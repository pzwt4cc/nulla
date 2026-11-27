#![forbid(unsafe_code)]

use crate::types::char_vis_width;
use std::fs::{self, File};
use std::io::{self, BufWriter, Read, Write};
use std::path::Path;

const TARGET_PAGE_BYTES: usize = 8192;
const MAX_PAGE_BYTES: usize = 16384;
const MIN_MERGE_BYTES: usize = 2048;
const SUPER_STRIDE: usize = 64;
const MAX_UNDO_BYTES: usize = 64 * 1024 * 1024;

#[derive(Clone)]
pub(crate) struct Page {
    pub buf: String,
    pub newlines: Vec<u16>,
    pub chars: u32,
    pub ascii_simple: bool,
}

impl Page {
    fn from_string(buf: String) -> Self {
        debug_assert!(!buf.is_empty());
        debug_assert!(buf.len() <= MAX_PAGE_BYTES);
        let bytes = buf.as_bytes();
        let mut newlines = Vec::new();
        let mut ascii_simple = true;
        for (i, &b) in bytes.iter().enumerate() {
            if b == b'\n' {
                newlines.push(i as u16);
            } else if !(0x20..=0x7E).contains(&b) {
                ascii_simple = false;
            }
        }
        let chars = if ascii_simple {
            bytes.len() as u32
        } else {
            buf.chars().count() as u32
        };
        Self {
            buf,
            newlines,
            chars,
            ascii_simple,
        }
    }
}

#[derive(Clone, Copy, Default)]
struct SuperSummary {
    bytes: usize,
    newlines: usize,
    chars: usize,
}

#[derive(Clone)]
pub struct UndoEntry {
    pub pos: usize,
    pub deleted: String,
    pub inserted: String,
    pub cursor_before: usize,
    pub anchor_before: usize,
    pub cursor_after: usize,
    pub anchor_after: usize,
    pub rev_before: u64,
    pub rev_after: u64,
    pub timestamp_ms: u64,
    pub merge_kind: u8,
}

impl UndoEntry {
    fn mem_bytes(&self) -> usize {
        80 + self.deleted.len() + self.inserted.len()
    }
}

pub struct PagedBuffer {
    pages: Vec<Page>,
    supers: Vec<SuperSummary>,
    total_bytes: usize,
    total_newlines: usize,
    total_chars: usize,
    max_line_cols_hint: usize,
    use_crlf: bool,
    revision: u64,
    next_revision: u64,
    saved_revision: u64,
    undo_stack: Vec<UndoEntry>,
    redo_stack: Vec<UndoEntry>,
    undo_mem_bytes: usize,
}

pub struct LineWindow {
    pub slice_start_byte_in_line: usize,
    pub slice_end_byte_in_line: usize,
    pub start_vis_col: usize,
    pub total_vis_cols: usize,
}

fn floor_char_boundary_str(s: &str, mut idx: usize) -> usize {
    if idx >= s.len() {
        return s.len();
    }
    while idx > 0 && !s.is_char_boundary(idx) {
        idx -= 1;
    }
    idx
}

fn chunk_str_into_pages(s: &str, out: &mut Vec<Page>) {
    let mut pos = 0;
    while pos < s.len() {
        let rem = s.len() - pos;
        let step = if rem <= MAX_PAGE_BYTES {
            rem
        } else {
            let target = pos + TARGET_PAGE_BYTES;
            let split = floor_char_boundary_str(s, target);
            if split > pos {
                split - pos
            } else {
                let mut next = target + 1;
                while next < s.len() && !s.is_char_boundary(next) {
                    next += 1;
                }
                next - pos
            }
        };
        out.push(Page::from_string(s[pos..pos + step].to_string()));
        pos += step;
    }
}

impl Default for PagedBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl PagedBuffer {
    pub fn new() -> Self {
        Self {
            pages: Vec::new(),
            supers: Vec::new(),
            total_bytes: 0,
            total_newlines: 0,
            total_chars: 0,
            max_line_cols_hint: 0,
            use_crlf: false,
            revision: 0,
            next_revision: 1,
            saved_revision: 0,
            undo_stack: Vec::new(),
            redo_stack: Vec::new(),
            undo_mem_bytes: 0,
        }
    }

    pub fn clear(&mut self) {
        self.pages.clear();
        self.supers.clear();
        self.total_bytes = 0;
        self.total_newlines = 0;
        self.total_chars = 0;
        self.max_line_cols_hint = 0;
        self.use_crlf = false;
        self.revision = 0;
        self.next_revision = 1;
        self.saved_revision = 0;
        self.undo_stack.clear();
        self.redo_stack.clear();
        self.undo_mem_bytes = 0;
    }

    pub fn load_from_path(&mut self, path: &Path) -> io::Result<()> {
        let mut file = File::open(path)?;
        let meta_len = file.metadata().map(|m| m.len() as usize).unwrap_or(0);
        let mut raw = Vec::with_capacity(meta_len.min(256 * 1024 * 1024));
        file.read_to_end(&mut raw)?;
        self.load_from_raw_bytes(&raw);
        Ok(())
    }

    pub fn load_from_raw_bytes(&mut self, raw: &[u8]) {
        self.clear();
        if raw.is_empty() {
            return;
        }

        let head = &raw[..raw.len().min(65536)];
        let mut crlf_count = 0usize;
        let mut lf_only_count = 0usize;
        for i in 0..head.len() {
            if head[i] == b'\n' {
                if i > 0 && head[i - 1] == b'\r' {
                    crlf_count += 1;
                } else {
                    lf_only_count += 1;
                }
            }
        }
        self.use_crlf = crlf_count > 0 && crlf_count >= lf_only_count;

        let mut page_buf = String::with_capacity(TARGET_PAGE_BYTES + 64);
        let mut cur_line_bytes = 0usize;
        let mut max_line_bytes = 0usize;

        let push_sanitized = |ch: char,
                              page_buf: &mut String,
                              pages: &mut Vec<Page>,
                              cur_line_bytes: &mut usize,
                              max_line_bytes: &mut usize| {
            let safe_ch = if ch == '\0' { '\u{FFFD}' } else { ch };
            if safe_ch == '\n' {
                if *cur_line_bytes > *max_line_bytes {
                    *max_line_bytes = *cur_line_bytes;
                }
                *cur_line_bytes = 0;
            } else {
                *cur_line_bytes += safe_ch.len_utf8();
            }
            page_buf.push(safe_ch);
            if page_buf.len() >= TARGET_PAGE_BYTES {
                let full =
                    std::mem::replace(page_buf, String::with_capacity(TARGET_PAGE_BYTES + 64));
                pages.push(Page::from_string(full));
            }
        };

        let chunk_size = 65536;
        let mut offset = 0usize;
        let mut prev_was_cr = false;

        while offset < raw.len() {
            let mut end = (offset + chunk_size).min(raw.len());
            if end < raw.len() {
                let mut back = 0usize;
                while back < 3 && end > offset && (raw[end] & 0b1100_0000) == 0b1000_0000 {
                    end -= 1;
                    back += 1;
                }
            }
            let slice = &raw[offset..end];
            let cow = String::from_utf8_lossy(slice);
            for ch in cow.chars() {
                if prev_was_cr {
                    prev_was_cr = false;
                    if ch == '\n' {
                        push_sanitized(
                            '\n',
                            &mut page_buf,
                            &mut self.pages,
                            &mut cur_line_bytes,
                            &mut max_line_bytes,
                        );
                        continue;
                    } else {
                        push_sanitized(
                            '\n',
                            &mut page_buf,
                            &mut self.pages,
                            &mut cur_line_bytes,
                            &mut max_line_bytes,
                        );
                    }
                }
                if ch == '\r' {
                    prev_was_cr = true;
                } else {
                    push_sanitized(
                        ch,
                        &mut page_buf,
                        &mut self.pages,
                        &mut cur_line_bytes,
                        &mut max_line_bytes,
                    );
                }
            }
            offset = end;
        }
        if prev_was_cr {
            push_sanitized(
                '\n',
                &mut page_buf,
                &mut self.pages,
                &mut cur_line_bytes,
                &mut max_line_bytes,
            );
        }
        if cur_line_bytes > max_line_bytes {
            max_line_bytes = cur_line_bytes;
        }
        if !page_buf.is_empty() {
            self.pages.push(Page::from_string(page_buf));
        }

        self.max_line_cols_hint = max_line_bytes;
        self.rebuild_summaries();
        self.revision = 0;
        self.next_revision = 1;
        self.saved_revision = 0;
    }

    pub fn save_to_path(&mut self, path: &Path) -> io::Result<()> {
        if path.as_os_str().is_empty() {
            return Err(io::Error::new(io::ErrorKind::InvalidInput, "Empty file path"));
        }
        let parent = path.parent().unwrap_or_else(|| Path::new("."));
        let file_name = path
            .file_name()
            .and_then(|s| s.to_str())
            .unwrap_or("untitled");
        let tmp_name = format!(".{}.nulla-tmp-{}", file_name, std::process::id());
        let tmp_path = parent.join(tmp_name);

        let write_result = (|| -> io::Result<()> {
            let f = File::create(&tmp_path)?;
            let mut w = BufWriter::with_capacity(65536, f);
            self.write_stream(&mut w)?;
            w.flush()?;
            if let Ok(inner) = w.into_inner() {
                let _ = inner.sync_all();
            }
            Ok(())
        })();

        if let Err(err) = write_result {
            let _ = fs::remove_file(&tmp_path);
            if !path.exists() {
                let f = File::create(path)?;
                let mut w = BufWriter::with_capacity(65536, f);
                self.write_stream(&mut w)?;
                w.flush()?;
                if let Ok(inner) = w.into_inner() {
                    let _ = inner.sync_all();
                }
                self.saved_revision = self.revision;
                return Ok(());
            }
            return Err(err);
        }

        if fs::rename(&tmp_path, path).is_ok() {
            self.saved_revision = self.revision;
            return Ok(());
        }
        let _ = fs::remove_file(&tmp_path);

        let f = File::create(path)?;
        let mut w = BufWriter::with_capacity(65536, f);
        self.write_stream(&mut w)?;
        w.flush()?;
        if let Ok(inner) = w.into_inner() {
            let _ = inner.sync_all();
        }
        self.saved_revision = self.revision;
        Ok(())
    }

    fn write_stream<W: Write>(&self, w: &mut W) -> io::Result<()> {
        if !self.use_crlf {
            for p in &self.pages {
                w.write_all(p.buf.as_bytes())?;
            }
        } else {
            for p in &self.pages {
                let bytes = p.buf.as_bytes();
                let mut last = 0usize;
                for &nl in &p.newlines {
                    let idx = nl as usize;
                    if idx > last {
                        w.write_all(&bytes[last..idx])?;
                    }
                    w.write_all(b"\r\n")?;
                    last = idx + 1;
                }
                if last < bytes.len() {
                    w.write_all(&bytes[last..])?;
                }
            }
        }
        Ok(())
    }

    #[inline]
    pub fn byte_len(&self) -> usize {
        self.total_bytes
    }

    #[inline]
    pub fn line_count(&self) -> usize {
        self.total_newlines + 1
    }

    #[inline]
    pub fn char_len(&self) -> usize {
        self.total_chars
    }

    #[inline]
    pub fn max_line_cols_hint(&self) -> usize {
        self.max_line_cols_hint
    }

    #[inline]
    pub fn revision(&self) -> u64 {
        self.revision
    }

    #[inline]
    pub fn is_modified(&self) -> bool {
        self.revision != self.saved_revision
    }

    pub fn mark_saved(&mut self) {
        self.saved_revision = self.revision;
    }

    #[inline]
    pub fn can_undo(&self) -> bool {
        !self.undo_stack.is_empty()
    }

    #[inline]
    pub fn can_redo(&self) -> bool {
        !self.redo_stack.is_empty()
    }

    pub(crate) fn pages(&self) -> &[Page] {
        &self.pages
    }

    fn alloc_revision(&mut self) -> u64 {
        let r = self.next_revision;
        self.next_revision = self.next_revision.wrapping_add(1);
        if self.next_revision == 0 {
            self.next_revision = 1;
        }
        r
    }

    fn rebuild_summaries(&mut self) {
        self.supers.clear();
        self.total_bytes = 0;
        self.total_newlines = 0;
        self.total_chars = 0;

        for chunk in self.pages.chunks(SUPER_STRIDE) {
            let mut s = SuperSummary::default();
            for p in chunk {
                s.bytes += p.buf.len();
                s.newlines += p.newlines.len();
                s.chars += p.chars as usize;
            }
            self.total_bytes += s.bytes;
            self.total_newlines += s.newlines;
            self.total_chars += s.chars;
            self.supers.push(s);
        }
    }

    pub fn clamp_char_boundary(&self, pos: usize) -> usize {
        if pos >= self.total_bytes || self.pages.is_empty() {
            return self.total_bytes;
        }
        let (p_idx, local) = self.locate_byte(pos);
        let page = &self.pages[p_idx];
        let snapped = floor_char_boundary_str(&page.buf, local);
        pos - (local - snapped)
    }

    pub(crate) fn locate_byte(&self, pos: usize) -> (usize, usize) {
        if self.pages.is_empty() {
            return (0, 0);
        }
        let clamped = pos.min(self.total_bytes);
        if clamped == self.total_bytes {
            let last = self.pages.len() - 1;
            return (last, self.pages[last].buf.len());
        }
        let mut rem = clamped;
        let mut page_idx = 0usize;
        for s in &self.supers {
            if rem >= s.bytes {
                rem -= s.bytes;
                page_idx += SUPER_STRIDE;
            } else {
                break;
            }
        }
        while page_idx < self.pages.len() {
            let plen = self.pages[page_idx].buf.len();
            if rem >= plen {
                rem -= plen;
                page_idx += 1;
            } else {
                break;
            }
        }
        (page_idx, rem)
    }

    pub fn nth_newline_byte(&self, k: usize) -> usize {
        debug_assert!(k < self.total_newlines);
        let mut rem = k;
        let mut page_idx = 0usize;
        let mut byte_acc = 0usize;
        for s in &self.supers {
            if rem >= s.newlines {
                rem -= s.newlines;
                byte_acc += s.bytes;
                page_idx += SUPER_STRIDE;
            } else {
                break;
            }
        }
        while page_idx < self.pages.len() {
            let p = &self.pages[page_idx];
            if rem >= p.newlines.len() {
                rem -= p.newlines.len();
                byte_acc += p.buf.len();
                page_idx += 1;
            } else {
                return byte_acc + (p.newlines[rem] as usize);
            }
        }
        self.total_bytes
    }

    pub fn line_byte_range(&self, line_idx: usize) -> (usize, usize) {
        let count = self.line_count();
        if self.pages.is_empty() || count == 0 {
            return (0, 0);
        }
        let clamped = line_idx.min(count - 1);
        let start = if clamped == 0 {
            0
        } else {
            self.nth_newline_byte(clamped - 1) + 1
        };
        let end = if clamped >= self.total_newlines {
            self.total_bytes
        } else {
            self.nth_newline_byte(clamped)
        };
        (start, end)
    }

    pub fn line_start_byte(&self, line_idx: usize) -> usize {
        self.line_byte_range(line_idx).0
    }

    pub fn byte_to_line(&self, byte_pos: usize) -> (usize, usize) {
        if self.pages.is_empty() || byte_pos == 0 {
            return (0, 0);
        }
        let clamped = byte_pos.min(self.total_bytes);
        let mut rem = clamped;
        let mut page_idx = 0usize;
        let mut nl_acc = 0usize;

        for s in &self.supers {
            if rem >= s.bytes {
                rem -= s.bytes;
                nl_acc += s.newlines;
                page_idx += SUPER_STRIDE;
            } else {
                break;
            }
        }
        while page_idx < self.pages.len() {
            let p = &self.pages[page_idx];
            if rem >= p.buf.len() {
                rem -= p.buf.len();
                nl_acc += p.newlines.len();
                page_idx += 1;
            } else {
                nl_acc += p.newlines.partition_point(|&nl| (nl as usize) < rem);
                break;
            }
        }
        let start = if nl_acc == 0 {
            0
        } else {
            self.nth_newline_byte(nl_acc - 1) + 1
        };
        (nl_acc, start)
    }

    pub fn copy_range_into(&self, start_byte: usize, end_byte: usize, out: &mut String) {
        out.clear();
        let s = self.clamp_char_boundary(start_byte.min(end_byte));
        let e = self.clamp_char_boundary(start_byte.max(end_byte));
        if s >= e || self.pages.is_empty() {
            return;
        }
        out.reserve(e - s);
        let (mut p_idx, mut local) = self.locate_byte(s);
        let mut rem = e - s;
        while rem > 0 && p_idx < self.pages.len() {
            let p = &self.pages[p_idx];
            let avail = p.buf.len() - local;
            let take = avail.min(rem);
            out.push_str(&p.buf[local..local + take]);
            rem -= take;
            p_idx += 1;
            local = 0;
        }
    }

    pub fn slice_to_string(&self, start_byte: usize, end_byte: usize) -> String {
        let mut s = String::new();
        self.copy_range_into(start_byte, end_byte, &mut s);
        s
    }

    pub fn byte_at(&self, pos: usize) -> Option<u8> {
        if pos >= self.total_bytes || self.pages.is_empty() {
            return None;
        }
        let (p_idx, local) = self.locate_byte(pos);
        self.pages.get(p_idx)?.buf.as_bytes().get(local).copied()
    }

    pub fn char_at(&self, pos: usize) -> Option<char> {
        if pos >= self.total_bytes || self.pages.is_empty() {
            return None;
        }
        let s = self.clamp_char_boundary(pos);
        let (p_idx, local) = self.locate_byte(s);
        self.pages.get(p_idx)?.buf[local..].chars().next()
    }

    pub fn prev_char_boundary(&self, pos: usize) -> usize {
        let s = self.clamp_char_boundary(pos);
        if s == 0 || self.pages.is_empty() {
            return 0;
        }
        let (mut p_idx, mut local) = self.locate_byte(s);
        if local == 0 {
            if p_idx == 0 {
                return 0;
            }
            p_idx -= 1;
            local = self.pages[p_idx].buf.len();
        }
        let p = &self.pages[p_idx].buf;
        let prev_local = p[..local]
            .char_indices()
            .next_back()
            .map(|(i, _)| i)
            .unwrap_or(0);
        s - (local - prev_local)
    }

    pub fn next_char_boundary(&self, pos: usize) -> usize {
        let s = self.clamp_char_boundary(pos);
        if s >= self.total_bytes {
            return self.total_bytes;
        }
        if let Some(ch) = self.char_at(s) {
            (s + ch.len_utf8()).min(self.total_bytes)
        } else {
            self.total_bytes
        }
    }

    pub fn count_chars_in_range(&self, start_byte: usize, end_byte: usize) -> usize {
        let s = self.clamp_char_boundary(start_byte.min(end_byte));
        let e = self.clamp_char_boundary(start_byte.max(end_byte));
        if s >= e || self.pages.is_empty() {
            return 0;
        }
        let (sp, sl) = self.locate_byte(s);
        let (ep, el) = self.locate_byte(e);
        if sp == ep {
            let p = &self.pages[sp];
            return if p.ascii_simple {
                el - sl
            } else {
                p.buf[sl..el].chars().count()
            };
        }

        let mut total = {
            let p = &self.pages[sp];
            if p.ascii_simple {
                p.buf.len() - sl
            } else {
                p.buf[sl..].chars().count()
            }
        };
        for idx in (sp + 1)..ep {
            total += self.pages[idx].chars as usize;
        }
        if ep < self.pages.len() && el > 0 {
            let p = &self.pages[ep];
            total += if p.ascii_simple {
                el
            } else {
                p.buf[..el].chars().count()
            };
        }
        total
    }

    pub fn cols_in_line(&self, line_start: usize, byte_pos: usize) -> (usize, usize) {
        let s = self.clamp_char_boundary(line_start);
        let e = self.clamp_char_boundary(byte_pos.max(s));
        if s >= e || self.pages.is_empty() {
            return (1, 0);
        }
        let (mut p_idx, mut local) = self.locate_byte(s);
        let mut rem = e - s;
        let mut char_col = 1usize;
        let mut vis_col = 0usize;

        while rem > 0 && p_idx < self.pages.len() {
            let p = &self.pages[p_idx];
            let avail = p.buf.len() - local;
            let take = avail.min(rem);
            if p.ascii_simple {
                char_col += take;
                vis_col += take;
            } else {
                for ch in p.buf[local..local + take].chars() {
                    char_col += 1;
                    vis_col += char_vis_width(ch, vis_col);
                }
            }
            rem -= take;
            p_idx += 1;
            local = 0;
        }
        (char_col, vis_col)
    }

    pub fn line_vis_col_to_byte(&self, line_idx: usize, target_vis_col: usize) -> usize {
        let (line_start, line_end) = self.line_byte_range(line_idx);
        if line_start >= line_end || target_vis_col == 0 {
            return line_start;
        }
        let (mut p_idx, mut local) = self.locate_byte(line_start);
        let mut rem = line_end - line_start;
        let mut cur_byte = line_start;
        let mut vis_col = 0usize;

        while rem > 0 && p_idx < self.pages.len() {
            let p = &self.pages[p_idx];
            let avail = p.buf.len() - local;
            let take = avail.min(rem);
            if p.ascii_simple && vis_col + take <= target_vis_col {
                vis_col += take;
                cur_byte += take;
                if vis_col == target_vis_col {
                    return cur_byte;
                }
            } else {
                for (rel, ch) in p.buf[local..local + take].char_indices() {
                    let w = char_vis_width(ch, vis_col);
                    if vis_col + w > target_vis_col {
                        return cur_byte + rel;
                    }
                    vis_col += w;
                    if vis_col == target_vis_col {
                        return cur_byte + rel + ch.len_utf8();
                    }
                }
                cur_byte += take;
            }
            rem -= take;
            p_idx += 1;
            local = 0;
        }
        line_end
    }

    pub fn extract_line_window(
        &self,
        line_idx: usize,
        first_vis_col: usize,
        max_vis_cols: usize,
        out_slice: &mut String,
    ) -> LineWindow {
        out_slice.clear();
        let (line_start, line_end) = self.line_byte_range(line_idx);
        let line_len = line_end - line_start;
        if line_len == 0 || self.pages.is_empty() {
            return LineWindow {
                slice_start_byte_in_line: 0,
                slice_end_byte_in_line: 0,
                start_vis_col: 0,
                total_vis_cols: 0,
            };
        }

        let end_vis_target = first_vis_col.saturating_add(max_vis_cols.max(1));
        let (mut p_idx, mut local) = self.locate_byte(line_start);
        let mut rem = line_len;
        let mut rel_byte = 0usize;
        let mut vis_col = 0usize;

        let mut started = false;
        let mut finished_slice = false;
        let mut slice_start_rel = line_len;
        let mut slice_end_rel = line_len;
        let mut start_vis_col = 0usize;

        while rem > 0 && p_idx < self.pages.len() {
            let p = &self.pages[p_idx];
            let avail = p.buf.len() - local;
            let take = avail.min(rem);
            let seg = &p.buf[local..local + take];

            if !started && p.ascii_simple && vis_col + take <= first_vis_col {
                vis_col += take;
                rel_byte += take;
            } else if finished_slice {
                if p.ascii_simple {
                    vis_col += take;
                    rel_byte += take;
                } else if rem > 262_144 {
                    vis_col += rem;
                    break;
                } else {
                    for ch in seg.chars() {
                        vis_col += char_vis_width(ch, vis_col);
                    }
                    rel_byte += take;
                }
            } else {
                for (ci, ch) in seg.char_indices() {
                    let w = char_vis_width(ch, vis_col);
                    if !started && vis_col + w > first_vis_col {
                        started = true;
                        slice_start_rel = rel_byte + ci;
                        start_vis_col = vis_col;
                    }
                    if started && !finished_slice {
                        if vis_col >= end_vis_target {
                            finished_slice = true;
                            slice_end_rel = rel_byte + ci;
                        } else {
                            out_slice.push(ch);
                        }
                    }
                    vis_col += w;
                }
                rel_byte += take;
            }

            rem -= take;
            p_idx += 1;
            local = 0;
        }

        if !started {
            slice_start_rel = line_len;
            slice_end_rel = line_len;
            start_vis_col = vis_col;
        } else if !finished_slice {
            slice_end_rel = line_len;
        }

        LineWindow {
            slice_start_byte_in_line: slice_start_rel,
            slice_end_byte_in_line: slice_end_rel,
            start_vis_col,
            total_vis_cols: vis_col,
        }
    }

    pub fn apply_raw_edit(&mut self, start_byte: usize, end_byte: usize, insert: &str) {
        let s = self.clamp_char_boundary(start_byte.min(end_byte));
        let e = self.clamp_char_boundary(start_byte.max(end_byte));
        if s == e && insert.is_empty() {
            return;
        }

        if self.pages.is_empty() {
            chunk_str_into_pages(insert, &mut self.pages);
            self.max_line_cols_hint = self.max_line_cols_hint.max(max_line_len_in_str(insert));
            self.rebuild_summaries();
            self.revision = self.alloc_revision();
            return;
        }

        let (sp, sl) = self.locate_byte(s);
        let (ep, el) = self.locate_byte(e);

        let prefix = self.pages[sp].buf[..sl].to_string();
        let suffix = self.pages[ep].buf[el..].to_string();

        let combined_len = prefix.len() + insert.len() + suffix.len();
        let mut replacement_pages = Vec::new();

        if combined_len == 0 {
            self.pages.drain(sp..=ep);
        } else if combined_len <= MAX_PAGE_BYTES {
            let mut merged = String::with_capacity(combined_len);
            merged.push_str(&prefix);
            merged.push_str(insert);
            merged.push_str(&suffix);
            replacement_pages.push(Page::from_string(merged));
            self.pages.splice(sp..=ep, replacement_pages);
        } else {
            if !prefix.is_empty() {
                chunk_str_into_pages(&prefix, &mut replacement_pages);
            }
            if !insert.is_empty() {
                chunk_str_into_pages(insert, &mut replacement_pages);
            }
            if !suffix.is_empty() {
                chunk_str_into_pages(&suffix, &mut replacement_pages);
            }
            self.pages.splice(sp..=ep, replacement_pages);
        }

        let check_from = sp.saturating_sub(1);
        let mut i = check_from;
        while i + 1 < self.pages.len() && i <= sp + 2 {
            if self.pages[i].buf.len() + self.pages[i + 1].buf.len() <= MIN_MERGE_BYTES * 2 {
                let next_buf = self.pages.remove(i + 1).buf;
                let mut combined = std::mem::take(&mut self.pages[i].buf);
                combined.push_str(&next_buf);
                self.pages[i] = Page::from_string(combined);
            } else {
                i += 1;
            }
        }

        self.max_line_cols_hint = self.max_line_cols_hint.max(max_line_len_in_str(insert));
        self.rebuild_summaries();
        self.revision = self.alloc_revision();
    }

    #[allow(clippy::too_many_arguments)]
    pub fn edit_with_undo(
        &mut self,
        start_byte: usize,
        end_byte: usize,
        insert: &str,
        cursor_before: usize,
        anchor_before: usize,
        cursor_after: usize,
        anchor_after: usize,
        timestamp_ms: u64,
        merge_kind: u8,
    ) {
        let s = self.clamp_char_boundary(start_byte.min(end_byte));
        let e = self.clamp_char_boundary(start_byte.max(end_byte));
        if s == e && insert.is_empty() {
            return;
        }

        let rev_before = self.revision;
        let deleted = self.slice_to_string(s, e);
        self.apply_raw_edit(s, e, insert);
        let rev_after = self.revision;
        self.redo_stack.clear();

        let can_merge = if merge_kind != 0 && timestamp_ms > 0 {
            if let Some(last) = self.undo_stack.last() {
                last.merge_kind == merge_kind
                    && last.rev_after != self.saved_revision
                    && timestamp_ms.saturating_sub(last.timestamp_ms) <= 650
                    && match merge_kind {
                        1 => {
                            deleted.is_empty()
                                && !insert.contains('\n')
                                && s == last.pos + last.inserted.len()
                                && last.inserted.len() < 128
                        }
                        2 => {
                            insert.is_empty()
                                && !deleted.contains('\n')
                                && last.inserted.is_empty()
                                && e == last.pos
                                && last.deleted.len() < 128
                        }
                        _ => false,
                    }
            } else {
                false
            }
        } else {
            false
        };

        if can_merge {
            if let Some(last) = self.undo_stack.last_mut() {
                if merge_kind == 1 {
                    self.undo_mem_bytes += insert.len();
                    last.inserted.push_str(insert);
                } else if merge_kind == 2 {
                    self.undo_mem_bytes += deleted.len();
                    let mut new_del = deleted;
                    new_del.push_str(&last.deleted);
                    last.deleted = new_del;
                    last.pos = s;
                }
                last.cursor_after = cursor_after;
                last.anchor_after = anchor_after;
                last.rev_after = rev_after;
                last.timestamp_ms = timestamp_ms;
                return;
            }
        }

        let entry = UndoEntry {
            pos: s,
            deleted,
            inserted: insert.to_string(),
            cursor_before,
            anchor_before,
            cursor_after,
            anchor_after,
            rev_before,
            rev_after,
            timestamp_ms,
            merge_kind,
        };
        self.undo_mem_bytes += entry.mem_bytes();
        self.undo_stack.push(entry);

        while self.undo_mem_bytes > MAX_UNDO_BYTES && self.undo_stack.len() > 1 {
            let removed = self.undo_stack.remove(0);
            self.undo_mem_bytes = self.undo_mem_bytes.saturating_sub(removed.mem_bytes());
        }
    }

    pub fn undo(&mut self) -> Option<(usize, usize)> {
        let entry = self.undo_stack.pop()?;
        self.undo_mem_bytes = self.undo_mem_bytes.saturating_sub(entry.mem_bytes());
        self.apply_raw_edit(entry.pos, entry.pos + entry.inserted.len(), &entry.deleted);
        self.revision = entry.rev_before;
        let out = (
            self.clamp_char_boundary(entry.cursor_before),
            self.clamp_char_boundary(entry.anchor_before),
        );
        self.redo_stack.push(entry);
        Some(out)
    }

    pub fn redo(&mut self) -> Option<(usize, usize)> {
        let entry = self.redo_stack.pop()?;
        self.apply_raw_edit(entry.pos, entry.pos + entry.deleted.len(), &entry.inserted);
        self.revision = entry.rev_after;
        let out = (
            self.clamp_char_boundary(entry.cursor_after),
            self.clamp_char_boundary(entry.anchor_after),
        );
        self.undo_mem_bytes += entry.mem_bytes();
        self.undo_stack.push(entry);
        Some(out)
    }
}

fn max_line_len_in_str(s: &str) -> usize {
    let mut best = 0usize;
    for part in s.split('\n') {
        if part.len() > best {
            best = part.len();
        }
    }
    best
}
