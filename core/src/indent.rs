#![forbid(unsafe_code)]

use crate::buffer::PagedBuffer;
use crate::types::{VisGuide, TAB_STOP};

#[derive(Clone, Default)]
struct LineIndent {
    is_blank: bool,
    leading_cols: u32,
    guides: Vec<u32>,
}

pub struct IndentState {
    cached_rev: u64,
    indent_step: u32,
    lines: Vec<LineIndent>,
    prev_leading: Vec<i32>,
    prev_idx: Vec<isize>,
}

impl Default for IndentState {
    fn default() -> Self {
        Self::new()
    }
}

impl IndentState {
    pub fn new() -> Self {
        Self {
            cached_rev: u64::MAX,
            indent_step: 4,
            lines: Vec::new(),
            prev_leading: Vec::new(),
            prev_idx: Vec::new(),
        }
    }

    pub fn invalidate(&mut self) {
        self.cached_rev = u64::MAX;
    }

    pub fn indent_step(&mut self, buf: &PagedBuffer) -> u32 {
        if self.cached_rev != buf.revision() {
            self.cached_rev = buf.revision();
            self.indent_step = detect_indent_step(buf);
        }
        self.indent_step
    }

    #[allow(clippy::too_many_arguments)]
    pub fn compute_viewport_guides(
        &mut self,
        buf: &PagedBuffer,
        first_vis_line: usize,
        vis_line_count: usize,
        cursor_line: usize,
        per_line_offsets: &mut [(u32, u32)],
        out_guides: &mut Vec<VisGuide>,
    ) {
        out_guides.clear();
        for slot in per_line_offsets.iter_mut() {
            *slot = (0, 0);
        }
        let total_lines = buf.line_count();
        if total_lines == 0 || vis_line_count == 0 || first_vis_line >= total_lines {
            return;
        }

        let step = self.indent_step(buf);
        let last_vis_line = first_vis_line
            .saturating_add(vis_line_count - 1)
            .min(total_lines - 1);
        let cur_clamped = cursor_line.min(total_lines - 1);

        let include_cursor = cur_clamped.abs_diff(first_vis_line) <= 240
            || cur_clamped.abs_diff(last_vis_line) <= 240;

        let min_line = if include_cursor {
            first_vis_line.min(cur_clamped).saturating_sub(160)
        } else {
            first_vis_line.saturating_sub(160)
        };
        let max_line = if include_cursor {
            last_vis_line
                .max(cur_clamped)
                .saturating_add(160)
                .min(total_lines - 1)
        } else {
            last_vis_line.saturating_add(160).min(total_lines - 1)
        };

        let count = max_line.saturating_sub(min_line).saturating_add(1);
        self.lines.resize(count, LineIndent::default());
        for (i, slot) in self.lines.iter_mut().enumerate() {
            analyze_line_indent(buf, min_line + i, step, slot);
        }

        self.prev_leading.clear();
        self.prev_leading.resize(count, -1);
        self.prev_idx.clear();
        self.prev_idx.resize(count, -1);

        let mut last_non_blank: isize = -1;
        for i in 0..count {
            if !self.lines[i].is_blank {
                last_non_blank = i as isize;
            }
            self.prev_idx[i] = last_non_blank;
            if last_non_blank >= 0 {
                self.prev_leading[i] = self.lines[last_non_blank as usize].leading_cols as i32;
            }
        }

        let mut next_non_blank: isize = -1;
        for i in (0..count).rev() {
            if !self.lines[i].is_blank {
                next_non_blank = i as isize;
                continue;
            }
            let p = self.prev_idx[i];
            let n = next_non_blank;
            if p >= 0 && n >= 0 {
                let p_cols = self.prev_leading[i] as u32;
                let n_cols = self.lines[n as usize].leading_cols;
                let min_cols = p_cols.min(n_cols);
                if min_cols > 0 {
                    self.lines[i].leading_cols = min_cols;
                    let src_len = self.lines[p as usize].guides.len();
                    for gi in 0..src_len {
                        let gcol = self.lines[p as usize].guides[gi];
                        if gcol < min_cols {
                            self.lines[i].guides.push(gcol);
                        }
                    }
                }
            }
        }

        let mut active_col: Option<u32> = None;
        let mut active_start = 0usize;
        let mut active_end = 0usize;

        if include_cursor && cur_clamped >= min_line && cur_clamped <= max_line {
            let cur_rel = cur_clamped - min_line;
            let cur_cols = self.lines[cur_rel].leading_cols;
            let mut next_cols: Option<u32> = None;
            for i in (cur_rel + 1)..count {
                if !self.lines[i].is_blank {
                    next_cols = Some(self.lines[i].leading_cols);
                    break;
                }
            }

            if let Some(nc) = next_cols.filter(|&nc| nc > cur_cols) {
                let _ = nc;
                let target = cur_cols;
                let body_start = cur_rel + 1;
                if body_start < count && self.lines[body_start].guides.contains(&target) {
                    active_col = Some(target);
                    active_start = body_start;
                    active_end = body_start;
                    while active_end + 1 < count
                        && self.lines[active_end + 1].guides.contains(&target)
                    {
                        active_end += 1;
                    }
                }
            } else if let Some(&target) = self.lines[cur_rel].guides.last() {
                active_col = Some(target);
                active_start = cur_rel;
                active_end = cur_rel;
                while active_start > 0 && self.lines[active_start - 1].guides.contains(&target) {
                    active_start -= 1;
                }
                while active_end + 1 < count && self.lines[active_end + 1].guides.contains(&target)
                {
                    active_end += 1;
                }
            }
        }

        for vis_i in 0..vis_line_count.min(per_line_offsets.len()) {
            let doc_line = first_vis_line + vis_i;
            if doc_line < min_line || doc_line > max_line {
                continue;
            }
            let rel = doc_line - min_line;
            let guides = &self.lines[rel].guides;
            if guides.is_empty() {
                continue;
            }
            let start_off = out_guides.len() as u32;
            let in_active_scope = active_col.is_some() && rel >= active_start && rel <= active_end;
            for &gcol in guides {
                let is_active = u8::from(in_active_scope && active_col == Some(gcol));
                out_guides.push(VisGuide {
                    vis_col: gcol,
                    is_active,
                    _pad: [0; 3],
                });
            }
            per_line_offsets[vis_i] = (start_off, (out_guides.len() as u32) - start_off);
        }
    }
}

fn detect_indent_step(buf: &PagedBuffer) -> u32 {
    let total = buf.line_count();
    let mut mod2 = 0u32;
    let mut mod4 = 0u32;
    let mut tab_lines = 0u32;
    let mut scanned = 0u32;

    for line_idx in 0..total {
        if scanned >= 180 || line_idx >= 1000 {
            break;
        }
        let (s, e) = buf.line_byte_range(line_idx);
        if s >= e {
            continue;
        }
        let max_scan = (e - s).min(128);
        let mut spaces = 0u32;
        let mut has_tab = false;
        let mut non_space = false;
        for off in 0..max_scan {
            match buf.byte_at(s + off) {
                Some(b' ') => spaces += 1,
                Some(b'\t') => {
                    has_tab = true;
                    break;
                }
                Some(_) => {
                    non_space = true;
                    break;
                }
                None => break,
            }
        }
        if !non_space && !has_tab {
            continue;
        }
        scanned += 1;
        if has_tab {
            tab_lines += 1;
        } else if spaces > 0 {
            if spaces % 4 == 2 {
                mod2 += 1;
            } else if spaces % 4 == 0 {
                mod4 += 1;
            }
        }
    }

    if tab_lines == 0 && mod2 >= 2 && mod2 * 2 >= mod4 {
        2
    } else {
        4
    }
}

fn analyze_line_indent(buf: &PagedBuffer, line_idx: usize, step: u32, out: &mut LineIndent) {
    out.is_blank = true;
    out.leading_cols = 0;
    out.guides.clear();

    let (s, e) = buf.line_byte_range(line_idx);
    if s >= e {
        return;
    }

    let max_ws = (e - s).min(512);
    let mut col = 0u32;
    let mut ws_end = 0usize;
    let mut has_tab = false;
    let mut found_non_ws = false;

    for off in 0..max_ws {
        match buf.byte_at(s + off) {
            Some(b' ') => {
                col += 1;
                ws_end = off + 1;
            }
            Some(b'\t') => {
                has_tab = true;
                let ts = TAB_STOP as u32;
                col = ((col / ts) + 1) * ts;
                ws_end = off + 1;
            }
            Some(_) => {
                found_non_ws = true;
                break;
            }
            None => break,
        }
    }

    if !found_non_ws && (e - s) <= max_ws {
        return;
    }

    out.is_blank = false;
    out.leading_cols = col;
    if col == 0 {
        return;
    }

    out.guides.push(0);
    if !has_tab {
        let mut g = step.max(1);
        while g < col {
            out.guides.push(g);
            g += step.max(1);
        }
        return;
    }

    let ts = TAB_STOP as u32;
    let mut cur = 0u32;
    let mut space_run = 0u32;
    for off in 0..ws_end {
        if buf.byte_at(s + off) == Some(b'\t') {
            cur = ((cur / ts) + 1) * ts;
            space_run = 0;
            if cur < col {
                out.guides.push(cur);
            }
        } else {
            cur += 1;
            space_run += 1;
            if space_run >= step.max(1) {
                space_run = 0;
                if cur < col {
                    out.guides.push(cur);
                }
            }
        }
    }
}
