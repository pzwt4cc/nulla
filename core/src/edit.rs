#![forbid(unsafe_code)]

use crate::buffer::PagedBuffer;
use crate::indent::IndentState;
use crate::search;
use crate::syntax::{detect_language, tokenize_window, LangId, SyntaxState};
use crate::types::{MoveDir, ViewportSnapshot, VisGuide, VisLine, VisToken, TAB_STOP};
use std::io;
use std::path::Path;

pub struct NullaDoc {
    pub buf: PagedBuffer,
    pub cursor: usize,
    pub anchor: usize,
    pub desired_vis_col: Option<usize>,
    pub lang: LangId,
    pub syntax_enabled: bool,
    pub syntax_state: SyntaxState,
    pub indent_state: IndentState,
    scratch_utf8: String,
    scratch_slice: String,
    scratch_lines: Vec<VisLine>,
    scratch_tokens: Vec<VisToken>,
    scratch_guides: Vec<VisGuide>,
    scratch_guide_offs: Vec<(u32, u32)>,
    pub last_string_result: String,
}

impl Default for NullaDoc {
    fn default() -> Self {
        Self::new()
    }
}

impl NullaDoc {
    pub fn new() -> Self {
        Self {
            buf: PagedBuffer::new(),
            cursor: 0,
            anchor: 0,
            desired_vis_col: None,
            lang: LangId::Plain,
            syntax_enabled: true,
            syntax_state: SyntaxState::new(),
            indent_state: IndentState::new(),
            scratch_utf8: String::new(),
            scratch_slice: String::new(),
            scratch_lines: Vec::new(),
            scratch_tokens: Vec::new(),
            scratch_guides: Vec::new(),
            scratch_guide_offs: Vec::new(),
            last_string_result: String::new(),
        }
    }

    pub fn load_file(&mut self, path: &str) -> io::Result<()> {
        self.buf.load_from_path(Path::new(path))?;
        self.cursor = 0;
        self.anchor = 0;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
        let sample_end = self.buf.clamp_char_boundary(4096.min(self.buf.byte_len()));
        let sample = self.buf.slice_to_string(0, sample_end);
        self.lang = detect_language(path, &sample);
        Ok(())
    }

    pub fn save_file(&mut self, path: &str) -> io::Result<()> {
        self.buf.save_to_path(Path::new(path))?;
        let sample_end = self.buf.clamp_char_boundary(4096.min(self.buf.byte_len()));
        let sample = self.buf.slice_to_string(0, sample_end);
        let detected = detect_language(path, &sample);
        if self.lang == LangId::Plain && detected != LangId::Plain {
            self.lang = detected;
            self.syntax_state.invalidate();
        }
        Ok(())
    }

    pub fn set_text(&mut self, raw_utf8: &[u8], file_path_hint: &str) {
        self.buf.load_from_raw_bytes(raw_utf8);
        self.cursor = 0;
        self.anchor = 0;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
        let sample_end = self.buf.clamp_char_boundary(4096.min(self.buf.byte_len()));
        let sample = self.buf.slice_to_string(0, sample_end);
        self.lang = detect_language(file_path_hint, &sample);
    }

    pub fn set_language(&mut self, lang_id: &str, file_path_hint: &str) {
        if lang_id.is_empty() || lang_id == "auto" {
            let sample_end = self.buf.clamp_char_boundary(4096.min(self.buf.byte_len()));
            let sample = self.buf.slice_to_string(0, sample_end);
            self.lang = detect_language(file_path_hint, &sample);
        } else {
            self.lang = LangId::from_id_str(lang_id);
        }
        self.syntax_state.invalidate();
    }

    pub fn refresh_auto_language(&mut self, file_path_hint: &str) -> bool {
        if !file_path_hint.is_empty() {
            return false;
        }
        let sample_end = self.buf.clamp_char_boundary(4096.min(self.buf.byte_len()));
        let sample = self.buf.slice_to_string(0, sample_end);
        let next = detect_language("", &sample);
        if next != self.lang {
            self.lang = next;
            self.syntax_state.invalidate();
            true
        } else {
            false
        }
    }

    #[inline]
    pub fn selection_range(&self) -> (usize, usize) {
        (self.cursor.min(self.anchor), self.cursor.max(self.anchor))
    }

    #[inline]
    pub fn has_selection(&self) -> bool {
        self.cursor != self.anchor
    }

    pub fn has_single_line_selection(&self) -> bool {
        if !self.has_selection() {
            return false;
        }
        let (s, e) = self.selection_range();
        if e - s > 4096 {
            return false;
        }
        let (sl, _) = self.buf.byte_to_line(s);
        let (el, _) = self.buf.byte_to_line(e);
        sl == el
    }

    pub fn selected_char_count(&self) -> usize {
        if !self.has_selection() {
            return 0;
        }
        let (s, e) = self.selection_range();
        self.buf.count_chars_in_range(s, e)
    }

    pub fn cursor_line_col(&self) -> (usize, usize, usize) {
        let (line_idx, line_start) = self.buf.byte_to_line(self.cursor);
        let (char_col, vis_col) = self.buf.cols_in_line(line_start, self.cursor);
        (line_idx + 1, char_col, vis_col)
    }

    pub fn byte_to_line_vis_col(&self, byte_pos: usize) -> (usize, usize) {
        let clamped = self.buf.clamp_char_boundary(byte_pos);
        let (line_idx, line_start) = self.buf.byte_to_line(clamped);
        let (_, vis_col) = self.buf.cols_in_line(line_start, clamped);
        (line_idx, vis_col)
    }

    pub fn set_selection(&mut self, anchor_byte: usize, cursor_byte: usize) {
        self.anchor = self.buf.clamp_char_boundary(anchor_byte);
        self.cursor = self.buf.clamp_char_boundary(cursor_byte);
        self.desired_vis_col = None;
    }

    pub fn select_all(&mut self) {
        self.anchor = 0;
        self.cursor = self.buf.byte_len();
        self.desired_vis_col = None;
    }

    pub fn hit_test_byte(&self, line_idx: usize, vis_col: usize) -> usize {
        if self.buf.line_count() == 0 {
            return 0;
        }
        let clamped_line = line_idx.min(self.buf.line_count() - 1);
        self.buf.line_vis_col_to_byte(clamped_line, vis_col)
    }

    pub fn select_word_at(&mut self, line_idx: usize, vis_col: usize) {
        let pos = self.hit_test_byte(line_idx, vis_col);
        let (line_start, line_end) = self
            .buf
            .line_byte_range(line_idx.min(self.buf.line_count().saturating_sub(1)));
        if line_start >= line_end {
            self.anchor = line_start;
            self.cursor = line_start;
            self.desired_vis_col = None;
            return;
        }
        let probe = if pos >= line_end {
            self.buf.prev_char_boundary(line_end).max(line_start)
        } else {
            pos
        };
        let Some(ch) = self.buf.char_at(probe) else {
            self.anchor = pos;
            self.cursor = pos;
            return;
        };
        let cls = char_word_class(ch);
        let mut s = probe;
        while s > line_start {
            let prev = self.buf.prev_char_boundary(s);
            if prev < line_start {
                break;
            }
            if self.buf.char_at(prev).map(char_word_class) == Some(cls) {
                s = prev;
            } else {
                break;
            }
        }
        let mut e = self.buf.next_char_boundary(probe).min(line_end);
        while e < line_end {
            if self.buf.char_at(e).map(char_word_class) == Some(cls) {
                e = self.buf.next_char_boundary(e).min(line_end);
            } else {
                break;
            }
        }
        self.anchor = s;
        self.cursor = e;
        self.desired_vis_col = None;
    }

    pub fn select_line_at(&mut self, line_idx: usize) {
        if self.buf.line_count() == 0 {
            self.anchor = 0;
            self.cursor = 0;
            return;
        }
        let clamped = line_idx.min(self.buf.line_count() - 1);
        let (s, e) = self.buf.line_byte_range(clamped);
        let end_with_nl = if e < self.buf.byte_len() && self.buf.byte_at(e) == Some(b'\n') {
            e + 1
        } else {
            e
        };
        self.anchor = s;
        self.cursor = end_with_nl;
        self.desired_vis_col = None;
    }

    pub fn select_current_line(&mut self) {
        let (line_idx, _) = self.buf.byte_to_line(self.cursor);
        self.select_line_at(line_idx);
    }

    pub fn move_cursor(&mut self, dir: MoveDir, extend: bool, page_lines: usize) {
        let step_pages = page_lines.max(1);

        if !extend && self.has_selection() {
            let (s, e) = self.selection_range();
            match dir {
                MoveDir::Left => {
                    self.cursor = s;
                    self.anchor = s;
                    self.desired_vis_col = None;
                    return;
                }
                MoveDir::Right => {
                    self.cursor = e;
                    self.anchor = e;
                    self.desired_vis_col = None;
                    return;
                }
                _ => {}
            }
        }

        let new_pos = match dir {
            MoveDir::Left => {
                self.desired_vis_col = None;
                self.buf.prev_char_boundary(self.cursor)
            }
            MoveDir::Right => {
                self.desired_vis_col = None;
                self.buf.next_char_boundary(self.cursor)
            }
            MoveDir::WordLeft => {
                self.desired_vis_col = None;
                self.word_left_pos(self.cursor)
            }
            MoveDir::WordRight => {
                self.desired_vis_col = None;
                self.word_right_pos(self.cursor)
            }
            MoveDir::Home => {
                self.desired_vis_col = None;
                let (line_idx, _) = self.buf.byte_to_line(self.cursor);
                let (ls, le) = self.buf.line_byte_range(line_idx);
                let mut first_non_ws = ls;
                while first_non_ws < le {
                    match self.buf.byte_at(first_non_ws) {
                        Some(b' ' | b'\t') => first_non_ws += 1,
                        _ => break,
                    }
                }
                if self.cursor == first_non_ws {
                    ls
                } else {
                    first_non_ws
                }
            }
            MoveDir::End => {
                self.desired_vis_col = None;
                let (line_idx, _) = self.buf.byte_to_line(self.cursor);
                self.buf.line_byte_range(line_idx).1
            }
            MoveDir::DocStart => {
                self.desired_vis_col = None;
                0
            }
            MoveDir::DocEnd => {
                self.desired_vis_col = None;
                self.buf.byte_len()
            }
            MoveDir::Up | MoveDir::Down | MoveDir::PageUp | MoveDir::PageDown => {
                let (cur_line, line_start) = self.buf.byte_to_line(self.cursor);
                let target_col = match self.desired_vis_col {
                    Some(c) => c,
                    None => {
                        let (_, vc) = self.buf.cols_in_line(line_start, self.cursor);
                        self.desired_vis_col = Some(vc);
                        vc
                    }
                };
                let max_line = self.buf.line_count().saturating_sub(1);
                let target_line = match dir {
                    MoveDir::Up => {
                        if cur_line == 0 {
                            self.desired_vis_col = None;
                            0
                        } else {
                            cur_line - 1
                        }
                    }
                    MoveDir::Down => {
                        if cur_line >= max_line {
                            self.desired_vis_col = None;
                            return self.finish_move(self.buf.byte_len(), extend);
                        }
                        cur_line + 1
                    }
                    MoveDir::PageUp => cur_line.saturating_sub(step_pages),
                    MoveDir::PageDown => (cur_line + step_pages).min(max_line),
                    _ => cur_line,
                };
                if dir == MoveDir::Up && cur_line == 0 {
                    0
                } else {
                    self.buf.line_vis_col_to_byte(target_line, target_col)
                }
            }
        };

        self.finish_move(new_pos, extend);
    }

    fn finish_move(&mut self, new_pos: usize, extend: bool) {
        self.cursor = self.buf.clamp_char_boundary(new_pos);
        if !extend {
            self.anchor = self.cursor;
        }
    }

    fn word_left_pos(&self, from: usize) -> usize {
        let mut p = self.buf.clamp_char_boundary(from);
        if p == 0 {
            return 0;
        }
        let prev = self.buf.prev_char_boundary(p);
        if self.buf.byte_at(prev) == Some(b'\n') {
            return prev;
        }
        while p > 0 {
            let np = self.buf.prev_char_boundary(p);
            match self.buf.char_at(np) {
                Some(' ' | '\t') => p = np,
                _ => break,
            }
        }
        if p == 0 {
            return 0;
        }
        let first_prev = self.buf.prev_char_boundary(p);
        let Some(ch) = self.buf.char_at(first_prev) else {
            return 0;
        };
        if ch == '\n' {
            return p;
        }
        let cls = char_word_class(ch);
        p = first_prev;
        while p > 0 {
            let np = self.buf.prev_char_boundary(p);
            match self.buf.char_at(np) {
                Some(c) if c != '\n' && char_word_class(c) == cls => p = np,
                _ => break,
            }
        }
        p
    }

    fn word_right_pos(&self, from: usize) -> usize {
        let len = self.buf.byte_len();
        let mut p = self.buf.clamp_char_boundary(from);
        if p >= len {
            return len;
        }
        if self.buf.byte_at(p) == Some(b'\n') {
            return p + 1;
        }
        if let Some(first_ch) = self.buf.char_at(p) {
            if matches!(first_ch, ' ' | '\t') {
                while p < len {
                    match self.buf.char_at(p) {
                        Some(' ' | '\t') => p = self.buf.next_char_boundary(p),
                        _ => break,
                    }
                }
                return p;
            }
            let cls = char_word_class(first_ch);
            while p < len {
                match self.buf.char_at(p) {
                    Some(c) if c != '\n' && char_word_class(c) == cls => {
                        p = self.buf.next_char_boundary(p);
                    }
                    _ => break,
                }
            }
        }
        p
    }

    pub fn insert_text(&mut self, raw_text: &str, timestamp_ms: u64) {
        if raw_text.is_empty() && !self.has_selection() {
            return;
        }
        let sanitized = sanitize_insert(raw_text);
        let (s, e) = self.selection_range();
        let new_cursor = s + sanitized.len();
        let merge_kind = if !self.has_selection()
            && !sanitized.is_empty()
            && sanitized.len() <= 8
            && !sanitized.contains('\n')
        {
            1
        } else {
            0
        };
        self.buf.edit_with_undo(
            s,
            e,
            &sanitized,
            self.cursor,
            self.anchor,
            new_cursor,
            new_cursor,
            timestamp_ms,
            merge_kind,
        );
        self.cursor = new_cursor;
        self.anchor = new_cursor;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    pub fn insert_newline_auto_indent(&mut self, timestamp_ms: u64) {
        let (s, e) = self.selection_range();
        let (_, line_start) = self.buf.byte_to_line(s);
        let mut indent = String::from("\n");
        let max_indent = s.min(line_start + 256);
        let mut p = line_start;
        while p < max_indent {
            match self.buf.byte_at(p) {
                Some(b' ') => {
                    indent.push(' ');
                    p += 1;
                }
                Some(b'\t') => {
                    indent.push('\t');
                    p += 1;
                }
                _ => break,
            }
        }
        let new_cursor = s + indent.len();
        self.buf.edit_with_undo(
            s,
            e,
            &indent,
            self.cursor,
            self.anchor,
            new_cursor,
            new_cursor,
            timestamp_ms,
            0,
        );
        self.cursor = new_cursor;
        self.anchor = new_cursor;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    pub fn backspace(&mut self, word: bool, timestamp_ms: u64) {
        if self.has_selection() {
            let (s, e) = self.selection_range();
            self.buf
                .edit_with_undo(s, e, "", self.cursor, self.anchor, s, s, timestamp_ms, 0);
            self.cursor = s;
            self.anchor = s;
            self.desired_vis_col = None;
            self.syntax_state.invalidate();
            return;
        }
        if self.cursor == 0 {
            return;
        }
        let del_start = if word {
            self.word_left_pos(self.cursor)
        } else {
            self.smart_backspace_start(self.cursor)
        };
        let merge_kind = if !word && self.cursor - del_start <= 4 {
            2
        } else {
            0
        };
        let end = self.cursor;
        self.buf.edit_with_undo(
            del_start,
            end,
            "",
            self.cursor,
            self.anchor,
            del_start,
            del_start,
            timestamp_ms,
            merge_kind,
        );
        self.cursor = del_start;
        self.anchor = del_start;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    fn smart_backspace_start(&self, pos: usize) -> usize {
        let (_, line_start) = self.buf.byte_to_line(pos);
        let rel = pos - line_start;
        if rel >= 2 && rel <= 256 {
            let mut all_spaces = true;
            for off in 0..rel {
                if self.buf.byte_at(line_start + off) != Some(b' ') {
                    all_spaces = false;
                    break;
                }
            }
            if all_spaces {
                let step = TAB_STOP;
                let rem = rel % step;
                let drop_n = if rem == 0 { step } else { rem };
                return pos - drop_n.min(rel);
            }
        }
        self.buf.prev_char_boundary(pos)
    }

    pub fn delete_forward(&mut self, word: bool, timestamp_ms: u64) {
        if self.has_selection() {
            let (s, e) = self.selection_range();
            self.buf
                .edit_with_undo(s, e, "", self.cursor, self.anchor, s, s, timestamp_ms, 0);
            self.cursor = s;
            self.anchor = s;
            self.desired_vis_col = None;
            self.syntax_state.invalidate();
            return;
        }
        if self.cursor >= self.buf.byte_len() {
            return;
        }
        let del_end = if word {
            self.word_right_pos(self.cursor)
        } else {
            self.buf.next_char_boundary(self.cursor)
        };
        let s = self.cursor;
        self.buf.edit_with_undo(
            s,
            del_end,
            "",
            self.cursor,
            self.anchor,
            s,
            s,
            timestamp_ms,
            0,
        );
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    fn touched_line_span(&self) -> (usize, usize, usize, usize) {
        let (sel_s, sel_e) = self.selection_range();
        let (start_line, line_s_byte) = self.buf.byte_to_line(sel_s);
        let effective_end = if sel_e > sel_s && self.buf.byte_at(sel_e - 1) == Some(b'\n') {
            sel_e - 1
        } else {
            sel_e
        };
        let (end_line, _) = self.buf.byte_to_line(effective_end);
        let (_, line_e_byte) = self.buf.line_byte_range(end_line);
        (start_line, end_line, line_s_byte, line_e_byte)
    }

    pub fn indent_selection(&mut self, unindent: bool, timestamp_ms: u64) {
        if !unindent && !self.has_selection() {
            self.insert_text("    ", timestamp_ms);
            return;
        }

        let had_sel = self.has_selection();
        let (_, _, block_s, block_e) = self.touched_line_span();
        if block_e.saturating_sub(block_s) > 16 * 1024 * 1024 {
            return;
        }
        let block = self.buf.slice_to_string(block_s, block_e);
        let mut updated = String::with_capacity(block.len() + 64);
        let mut first_delta: isize = 0;

        for (idx, line) in block.split('\n').enumerate() {
            if idx > 0 {
                updated.push('\n');
            }
            if !unindent {
                updated.push_str("    ");
                if idx == 0 {
                    first_delta = 4;
                }
                updated.push_str(line);
            } else {
                let lb = line.as_bytes();
                let removed = if lb.first() == Some(&b'\t') {
                    1
                } else {
                    lb.iter().take(4).take_while(|&&c| c == b' ').count()
                };
                if idx == 0 {
                    first_delta = -(removed as isize);
                }
                updated.push_str(&line[removed..]);
            }
        }

        if updated == block {
            return;
        }

        let (new_anchor, new_cursor) = if had_sel {
            (block_s, block_s + updated.len())
        } else {
            let shifted = (self.cursor as isize + first_delta).max(block_s as isize) as usize;
            let clamped = shifted.min(block_s + updated.len());
            (clamped, clamped)
        };

        self.buf.edit_with_undo(
            block_s,
            block_e,
            &updated,
            self.cursor,
            self.anchor,
            new_cursor,
            new_anchor,
            timestamp_ms,
            0,
        );
        self.anchor = new_anchor;
        self.cursor = new_cursor;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    pub fn toggle_comment(&mut self, timestamp_ms: u64) {
        let prefix = self.lang.comment_prefix();
        if prefix.is_empty() {
            return;
        }

        let had_sel = self.has_selection();
        let (_, _, block_s, block_e) = self.touched_line_span();
        if block_e.saturating_sub(block_s) > 16 * 1024 * 1024 {
            return;
        }
        let block = self.buf.slice_to_string(block_s, block_e);
        let lines: Vec<&str> = block.split('\n').collect();

        let mut all_commented = true;
        let mut has_non_empty = false;
        let mut min_indent = usize::MAX;

        for &line in &lines {
            let ws = line
                .bytes()
                .take_while(|&b| matches!(b, b' ' | b'\t'))
                .count();
            if ws == line.len() {
                continue;
            }
            has_non_empty = true;
            min_indent = min_indent.min(ws);
            if !line[ws..].starts_with(prefix) {
                all_commented = false;
            }
        }

        let mut updated = String::with_capacity(block.len() + lines.len() * (prefix.len() + 1));
        let mut first_delta: isize = 0;

        if !has_non_empty {
            for (idx, &line) in lines.iter().enumerate() {
                if idx > 0 {
                    updated.push('\n');
                }
                updated.push_str(line);
                updated.push_str(prefix);
                updated.push(' ');
                if idx == 0 {
                    first_delta = (prefix.len() + 1) as isize;
                }
            }
        } else if all_commented {
            for (idx, &line) in lines.iter().enumerate() {
                if idx > 0 {
                    updated.push('\n');
                }
                let ws = line
                    .bytes()
                    .take_while(|&b| matches!(b, b' ' | b'\t'))
                    .count();
                if ws == line.len() {
                    updated.push_str(line);
                    continue;
                }
                let after_prefix = ws + prefix.len();
                let drop_end = if line.as_bytes().get(after_prefix) == Some(&b' ') {
                    after_prefix + 1
                } else {
                    after_prefix
                };
                if idx == 0 {
                    first_delta = -((drop_end - ws) as isize);
                }
                updated.push_str(&line[..ws]);
                updated.push_str(&line[drop_end..]);
            }
        } else {
            let ins_col = if min_indent == usize::MAX {
                0
            } else {
                min_indent
            };
            for (idx, &line) in lines.iter().enumerate() {
                if idx > 0 {
                    updated.push('\n');
                }
                let ws = line
                    .bytes()
                    .take_while(|&b| matches!(b, b' ' | b'\t'))
                    .count();
                if ws == line.len() {
                    updated.push_str(line);
                    continue;
                }
                let at = ins_col.min(line.len());
                updated.push_str(&line[..at]);
                updated.push_str(prefix);
                updated.push(' ');
                updated.push_str(&line[at..]);
                if idx == 0 {
                    first_delta = (prefix.len() + 1) as isize;
                }
            }
        }

        let (new_anchor, new_cursor) = if had_sel {
            (block_s, block_s + updated.len())
        } else {
            let shifted = (self.cursor as isize + first_delta).max(block_s as isize) as usize;
            let clamped = shifted.min(block_s + updated.len());
            (clamped, clamped)
        };

        self.buf.edit_with_undo(
            block_s,
            block_e,
            &updated,
            self.cursor,
            self.anchor,
            new_cursor,
            new_anchor,
            timestamp_ms,
            0,
        );
        self.anchor = new_anchor;
        self.cursor = new_cursor;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    pub fn duplicate_selection_or_line(&mut self, timestamp_ms: u64) {
        if self.has_selection() {
            let (s, e) = self.selection_range();
            if e - s > 16 * 1024 * 1024 {
                return;
            }
            let sel = self.buf.slice_to_string(s, e);
            let new_anchor = e;
            let new_cursor = e + sel.len();
            self.buf.edit_with_undo(
                e,
                e,
                &sel,
                self.cursor,
                self.anchor,
                new_cursor,
                new_anchor,
                timestamp_ms,
                0,
            );
            self.anchor = new_anchor;
            self.cursor = new_cursor;
            self.desired_vis_col = None;
            self.syntax_state.invalidate();
            return;
        }

        let (line_idx, _) = self.buf.byte_to_line(self.cursor);
        let (ls, le) = self.buf.line_byte_range(line_idx);
        if le - ls > 16 * 1024 * 1024 {
            return;
        }
        let col_bytes = self.cursor - ls;
        let line_text = self.buf.slice_to_string(ls, le);
        let mut ins = String::with_capacity(line_text.len() + 1);
        ins.push('\n');
        ins.push_str(&line_text);
        let new_cursor = le + 1 + col_bytes;
        self.buf.edit_with_undo(
            le,
            le,
            &ins,
            self.cursor,
            self.anchor,
            new_cursor,
            new_cursor,
            timestamp_ms,
            0,
        );
        self.cursor = new_cursor;
        self.anchor = new_cursor;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    pub fn delete_lines(&mut self, timestamp_ms: u64) {
        if self.buf.byte_len() == 0 {
            return;
        }
        let (_, _, block_s, block_e) = self.touched_line_span();
        let (del_s, del_e) = if block_e < self.buf.byte_len() {
            (block_s, block_e + 1)
        } else if block_s > 0 {
            (block_s - 1, block_e)
        } else {
            (0, block_e)
        };
        let new_cursor = del_s.min(self.buf.byte_len().saturating_sub(del_e - del_s));
        self.buf.edit_with_undo(
            del_s,
            del_e,
            "",
            self.cursor,
            self.anchor,
            new_cursor,
            new_cursor,
            timestamp_ms,
            0,
        );
        self.cursor = self.buf.clamp_char_boundary(new_cursor);
        self.anchor = self.cursor;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    pub fn insert_line_sibling(&mut self, above: bool, timestamp_ms: u64) {
        let (line_idx, _) = self.buf.byte_to_line(self.cursor);
        let (ls, le) = self.buf.line_byte_range(line_idx);
        let mut indent = String::new();
        let max_ws = le.min(ls + 256);
        let mut p = ls;
        while p < max_ws {
            match self.buf.byte_at(p) {
                Some(b' ') => {
                    indent.push(' ');
                    p += 1;
                }
                Some(b'\t') => {
                    indent.push('\t');
                    p += 1;
                }
                _ => break,
            }
        }
        if above {
            let mut ins = String::with_capacity(indent.len() + 1);
            ins.push_str(&indent);
            ins.push('\n');
            let new_cursor = ls + indent.len();
            self.buf.edit_with_undo(
                ls,
                ls,
                &ins,
                self.cursor,
                self.anchor,
                new_cursor,
                new_cursor,
                timestamp_ms,
                0,
            );
            self.cursor = new_cursor;
            self.anchor = new_cursor;
        } else {
            let mut ins = String::with_capacity(indent.len() + 1);
            ins.push('\n');
            ins.push_str(&indent);
            let new_cursor = le + ins.len();
            self.buf.edit_with_undo(
                le,
                le,
                &ins,
                self.cursor,
                self.anchor,
                new_cursor,
                new_cursor,
                timestamp_ms,
                0,
            );
            self.cursor = new_cursor;
            self.anchor = new_cursor;
        }
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
    }

    pub fn move_lines(&mut self, up: bool, timestamp_ms: u64) -> bool {
        let total_lines = self.buf.line_count();
        if total_lines <= 1 {
            return false;
        }
        let (start_line, end_line, block_s, block_e) = self.touched_line_span();
        if block_e.saturating_sub(block_s) > 16 * 1024 * 1024 {
            return false;
        }

        if up {
            if start_line == 0 {
                return false;
            }
            let (prev_s, prev_e) = self.buf.line_byte_range(start_line - 1);
            let prev_line = self.buf.slice_to_string(prev_s, prev_e);
            let block = self.buf.slice_to_string(block_s, block_e);
            let mut swapped = String::with_capacity(prev_line.len() + 1 + block.len());
            swapped.push_str(&block);
            swapped.push('\n');
            swapped.push_str(&prev_line);

            let shift = prev_line.len() + 1;
            let new_anchor = self.anchor.saturating_sub(shift);
            let new_cursor = self.cursor.saturating_sub(shift);

            self.buf.edit_with_undo(
                prev_s,
                block_e,
                &swapped,
                self.cursor,
                self.anchor,
                new_cursor,
                new_anchor,
                timestamp_ms,
                0,
            );
            self.anchor = self.buf.clamp_char_boundary(new_anchor);
            self.cursor = self.buf.clamp_char_boundary(new_cursor);
        } else {
            if end_line + 1 >= total_lines {
                return false;
            }
            let (next_s, next_e) = self.buf.line_byte_range(end_line + 1);
            let next_line = self.buf.slice_to_string(next_s, next_e);
            let block = self.buf.slice_to_string(block_s, block_e);
            let mut swapped = String::with_capacity(next_line.len() + 1 + block.len());
            swapped.push_str(&next_line);
            swapped.push('\n');
            swapped.push_str(&block);

            let shift = next_line.len() + 1;
            let new_anchor = self.anchor + shift;
            let new_cursor = self.cursor + shift;

            self.buf.edit_with_undo(
                block_s,
                next_e,
                &swapped,
                self.cursor,
                self.anchor,
                new_cursor,
                new_anchor,
                timestamp_ms,
                0,
            );
            self.anchor = self.buf.clamp_char_boundary(new_anchor);
            self.cursor = self.buf.clamp_char_boundary(new_cursor);
        }

        self.desired_vis_col = None;
        self.syntax_state.invalidate();
        true
    }

    pub fn copy_selection_or_line(&mut self) -> &str {
        self.last_string_result.clear();
        if self.has_selection() {
            let (s, e) = self.selection_range();
            self.buf.copy_range_into(s, e, &mut self.last_string_result);
        } else {
            let (line_idx, _) = self.buf.byte_to_line(self.cursor);
            let (ls, le) = self.buf.line_byte_range(line_idx);
            self.buf
                .copy_range_into(ls, le, &mut self.last_string_result);
            self.last_string_result.push('\n');
        }
        &self.last_string_result
    }

    pub fn cut_selection_or_line(&mut self, timestamp_ms: u64) -> &str {
        self.last_string_result.clear();
        if self.has_selection() {
            let (s, e) = self.selection_range();
            self.buf.copy_range_into(s, e, &mut self.last_string_result);
            self.buf
                .edit_with_undo(s, e, "", self.cursor, self.anchor, s, s, timestamp_ms, 0);
            self.cursor = s;
            self.anchor = s;
        } else if self.buf.byte_len() > 0 {
            let (line_idx, _) = self.buf.byte_to_line(self.cursor);
            let (ls, le) = self.buf.line_byte_range(line_idx);
            self.buf
                .copy_range_into(ls, le, &mut self.last_string_result);
            self.last_string_result.push('\n');
            let (del_s, del_e) = if le < self.buf.byte_len() {
                (ls, le + 1)
            } else if ls > 0 {
                (ls - 1, le)
            } else {
                (0, le)
            };
            self.buf.edit_with_undo(
                del_s,
                del_e,
                "",
                self.cursor,
                self.anchor,
                del_s,
                del_s,
                timestamp_ms,
                0,
            );
            self.cursor = self.buf.clamp_char_boundary(del_s);
            self.anchor = self.cursor;
        }
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
        &self.last_string_result
    }

    pub fn undo(&mut self) -> bool {
        if let Some((c, a)) = self.buf.undo() {
            self.cursor = c;
            self.anchor = a;
            self.desired_vis_col = None;
            self.syntax_state.invalidate();
            true
        } else {
            false
        }
    }

    pub fn redo(&mut self) -> bool {
        if let Some((c, a)) = self.buf.redo() {
            self.cursor = c;
            self.anchor = a;
            self.desired_vis_col = None;
            self.syntax_state.invalidate();
            true
        } else {
            false
        }
    }

    pub fn find(&mut self, query: &str, forward: bool, wrap: bool) -> bool {
        if query.is_empty() {
            return false;
        }
        let (sel_s, sel_e) = self.selection_range();
        let origin = if forward {
            if self.has_selection() {
                sel_e
            } else {
                self.cursor
            }
        } else if self.has_selection() {
            sel_s
        } else {
            self.cursor
        };
        if let Some((ms, me)) = search::find_text(&self.buf, query, origin, forward, wrap) {
            self.anchor = ms;
            self.cursor = me;
            self.desired_vis_col = None;
            true
        } else {
            false
        }
    }

    pub fn match_stats(&self, query: &str) -> (usize, usize) {
        let (s, e) = self.selection_range();
        search::match_stats(&self.buf, query, s, e, 10_000)
    }

    pub fn replace_current_and_find(
        &mut self,
        query: &str,
        replacement: &str,
        timestamp_ms: u64,
    ) -> bool {
        if query.is_empty() {
            return false;
        }
        let (s, e) = self.selection_range();
        if !self.has_selection() || !search::matches_exact_ci(&self.buf, s, e, query) {
            return self.find(query, true, true);
        }

        let safe_rep = sanitize_insert(replacement);
        let after_rep = s + safe_rep.len();
        self.buf.edit_with_undo(
            s,
            e,
            &safe_rep,
            self.cursor,
            self.anchor,
            after_rep,
            after_rep,
            timestamp_ms,
            0,
        );
        self.cursor = after_rep;
        self.anchor = after_rep;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();

        let _ = self.find(query, true, true);
        true
    }

    pub fn replace_all(&mut self, query: &str, replacement: &str, timestamp_ms: u64) -> usize {
        if query.is_empty() {
            return 0;
        }
        let safe_rep = sanitize_insert(replacement);
        let Some((new_doc, count)) = search::build_replace_all(&self.buf, query, &safe_rep) else {
            return 0;
        };
        let old_len = self.buf.byte_len();
        let clamped_after = self.cursor.min(new_doc.len());
        let mut valid_cursor = clamped_after;
        while valid_cursor > 0 && !new_doc.is_char_boundary(valid_cursor) {
            valid_cursor -= 1;
        }
        self.buf.edit_with_undo(
            0,
            old_len,
            &new_doc,
            self.cursor,
            self.anchor,
            valid_cursor,
            valid_cursor,
            timestamp_ms,
            0,
        );
        self.cursor = valid_cursor;
        self.anchor = valid_cursor;
        self.desired_vis_col = None;
        self.syntax_state.invalidate();
        count
    }

    pub fn build_viewport(
        &mut self,
        first_line: usize,
        req_line_count: usize,
        first_vis_col: usize,
        max_vis_cols: usize,
        include_guides: bool,
    ) -> ViewportSnapshot {
        self.scratch_utf8.clear();
        self.scratch_lines.clear();
        self.scratch_tokens.clear();
        self.scratch_guides.clear();

        let total_lines = self.buf.line_count();
        if total_lines == 0 || req_line_count == 0 || first_line >= total_lines {
            return ViewportSnapshot {
                utf8_data: self.scratch_utf8.as_ptr(),
                utf8_len: 0,
                lines: self.scratch_lines.as_ptr(),
                line_count: 0,
                tokens: self.scratch_tokens.as_ptr(),
                token_count: 0,
                guides: self.scratch_guides.as_ptr(),
                guide_count: 0,
            };
        }

        let actual_count = req_line_count.min(total_lines - first_line);
        self.scratch_guide_offs.clear();
        self.scratch_guide_offs.resize(actual_count, (0, 0));

        if include_guides {
            let (cur_line_0, _) = self.buf.byte_to_line(self.cursor);
            self.indent_state.compute_viewport_guides(
                &self.buf,
                first_line,
                actual_count,
                cur_line_0,
                &mut self.scratch_guide_offs,
                &mut self.scratch_guides,
            );
        }

        let (sel_s, sel_e) = self.selection_range();
        let has_sel = sel_s < sel_e;

        let mut ml_state = if self.syntax_enabled {
            self.syntax_state
                .state_before_line(&self.buf, self.lang, first_line)
        } else {
            0
        };

        for i in 0..actual_count {
            let line_idx = first_line + i;
            let (line_start, line_end) = self.buf.line_byte_range(line_idx);
            let win = self.buf.extract_line_window(
                line_idx,
                first_vis_col,
                max_vis_cols,
                &mut self.scratch_slice,
            );

            let text_offset = self.scratch_utf8.len() as u32;
            let text_len = self.scratch_slice.len() as u32;
            self.scratch_utf8.push_str(&self.scratch_slice);

            let token_offset = self.scratch_tokens.len() as u32;
            let slice_is_line_start = win.slice_start_byte_in_line == 0;
            tokenize_window(
                self.syntax_enabled,
                self.lang,
                ml_state,
                &self.scratch_slice,
                slice_is_line_start,
                &mut self.scratch_tokens,
            );
            let token_count = (self.scratch_tokens.len() as u32) - token_offset;

            if self.syntax_enabled && self.lang.has_multiline_state() {
                ml_state = self
                    .syntax_state
                    .advance_line_state(&self.buf, self.lang, line_idx, ml_state);
            }

            let slice_abs_s = line_start + win.slice_start_byte_in_line;
            let slice_abs_e = line_start + win.slice_end_byte_in_line;
            let mut line_has_sel = 0u8;
            let mut sel_rel_start = 0u32;
            let mut sel_rel_end = 0u32;
            let mut sel_before_slice = 0u8;
            let mut eol_selected = 0u8;

            if has_sel && sel_e > line_start && sel_s <= line_end {
                if sel_s < slice_abs_s && sel_e > line_start {
                    sel_before_slice = 1;
                }
                let ov_s = sel_s.max(slice_abs_s);
                let ov_e = sel_e.min(slice_abs_e);
                if ov_e > ov_s {
                    line_has_sel = 1;
                    sel_rel_start = (ov_s - slice_abs_s) as u32;
                    sel_rel_end = (ov_e - slice_abs_s) as u32;
                }
                if sel_s <= line_end && sel_e > line_end {
                    line_has_sel = 1;
                    eol_selected = 1;
                }
            }

            let (guide_offset, guide_count) = self.scratch_guide_offs[i];

            self.scratch_lines.push(VisLine {
                line_idx,
                text_offset,
                text_len,
                start_vis_col: win.start_vis_col,
                line_total_vis_cols: win.total_vis_cols,
                token_offset,
                token_count,
                guide_offset,
                guide_count,
                sel_rel_start,
                sel_rel_end,
                has_selection: line_has_sel,
                sel_before_slice,
                eol_selected,
                _pad: 0,
            });
        }

        ViewportSnapshot {
            utf8_data: self.scratch_utf8.as_ptr(),
            utf8_len: self.scratch_utf8.len(),
            lines: self.scratch_lines.as_ptr(),
            line_count: self.scratch_lines.len(),
            tokens: self.scratch_tokens.as_ptr(),
            token_count: self.scratch_tokens.len(),
            guides: self.scratch_guides.as_ptr(),
            guide_count: self.scratch_guides.len(),
        }
    }
}

fn char_word_class(ch: char) -> u8 {
    if ch.is_whitespace() {
        0
    } else if ch.is_alphanumeric() || ch == '_' {
        1
    } else {
        2
    }
}

fn sanitize_insert(raw: &str) -> String {
    if !raw.contains(['\r', '\0']) {
        return raw.to_string();
    }
    let mut out = String::with_capacity(raw.len());
    let mut prev_cr = false;
    for ch in raw.chars() {
        if prev_cr {
            prev_cr = false;
            out.push('\n');
            if ch == '\n' {
                continue;
            }
        }
        if ch == '\r' {
            prev_cr = true;
        } else if ch == '\0' {
            out.push('\u{FFFD}');
        } else {
            out.push(ch);
        }
    }
    if prev_cr {
        out.push('\n');
    }
    out
}
