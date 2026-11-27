#![forbid(unsafe_code)]

use crate::buffer::PagedBuffer;

fn fold_str_lower(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    for ch in s.chars() {
        for lc in ch.to_lowercase() {
            out.push(lc);
        }
    }
    out
}

fn match_at_char_ci(hay: &str, needle_lower: &str) -> Option<usize> {
    let mut n_chars = needle_lower.chars();
    let mut consumed = 0usize;
    let mut h_iter = hay.chars();

    loop {
        let mut n_peek = n_chars.clone();
        if n_peek.next().is_none() {
            return Some(consumed);
        }
        let hc = h_iter.next()?;
        for lc in hc.to_lowercase() {
            let nc = n_chars.next()?;
            if lc != nc {
                return None;
            }
        }
        consumed += hc.len_utf8();
    }
}

fn find_first_in_str_ci(
    hay: &str,
    query: &str,
    query_lower: &str,
    is_ascii: bool,
) -> Option<(usize, usize)> {
    if query.is_empty() || hay.is_empty() {
        return None;
    }
    if is_ascii {
        let hb = hay.as_bytes();
        let qb = query_lower.as_bytes();
        if hb.len() < qb.len() {
            return None;
        }
        let first_q = qb[0];
        let limit = hb.len() - qb.len();
        let mut i = 0usize;
        while i <= limit {
            if hb[i].to_ascii_lowercase() == first_q {
                let mut ok = true;
                for j in 1..qb.len() {
                    if hb[i + j].to_ascii_lowercase() != qb[j] {
                        ok = false;
                        break;
                    }
                }
                if ok {
                    return Some((i, qb.len()));
                }
            }
            i += 1;
        }
        None
    } else {
        let first_q = query_lower.chars().next()?;
        for (idx, ch) in hay.char_indices() {
            if ch.to_lowercase().next() == Some(first_q) {
                if let Some(m_len) = match_at_char_ci(&hay[idx..], query_lower) {
                    return Some((idx, m_len));
                }
            }
        }
        None
    }
}

fn find_last_in_str_ci(
    hay: &str,
    query: &str,
    query_lower: &str,
    is_ascii: bool,
) -> Option<(usize, usize)> {
    if query.is_empty() || hay.is_empty() {
        return None;
    }
    if is_ascii {
        let hb = hay.as_bytes();
        let qb = query_lower.as_bytes();
        if hb.len() < qb.len() {
            return None;
        }
        let first_q = qb[0];
        let mut i = hb.len() - qb.len();
        loop {
            if hb[i].to_ascii_lowercase() == first_q {
                let mut ok = true;
                for j in 1..qb.len() {
                    if hb[i + j].to_ascii_lowercase() != qb[j] {
                        ok = false;
                        break;
                    }
                }
                if ok {
                    return Some((i, qb.len()));
                }
            }
            if i == 0 {
                break;
            }
            i -= 1;
        }
        None
    } else {
        let first_q = query_lower.chars().next()?;
        for (idx, ch) in hay.char_indices().rev() {
            if ch.to_lowercase().next() == Some(first_q) {
                if let Some(m_len) = match_at_char_ci(&hay[idx..], query_lower) {
                    return Some((idx, m_len));
                }
            }
        }
        None
    }
}

fn find_forward_range(
    buf: &PagedBuffer,
    start_byte: usize,
    end_byte: usize,
    query: &str,
    query_lower: &str,
    is_ascii: bool,
) -> Option<(usize, usize)> {
    let s = buf.clamp_char_boundary(start_byte);
    let e = buf.clamp_char_boundary(end_byte);
    if s >= e || query.is_empty() || buf.pages().is_empty() {
        return None;
    }

    let max_match_bytes = if is_ascii {
        query_lower.len()
    } else {
        query_lower.chars().count().saturating_mul(4).max(4)
    };
    let overlap_cap = max_match_bytes.saturating_sub(1);

    let (mut p_idx, mut local) = buf.locate_byte(s);
    let mut rem = e - s;
    let mut abs_pos = s;
    let mut tail = String::new();
    let mut tail_abs_start = s;
    let mut window = String::new();

    while rem > 0 && p_idx < buf.pages().len() {
        let p = &buf.pages()[p_idx];
        let avail = p.buf.len() - local;
        let take = avail.min(rem);
        let seg = &p.buf[local..local + take];

        if !tail.is_empty() {
            window.clear();
            window.push_str(&tail);
            let head_take = seg
                .char_indices()
                .nth(query_lower.chars().count())
                .map(|(i, _)| i)
                .unwrap_or(seg.len());
            window.push_str(&seg[..head_take]);
            if let Some((m_off, m_len)) =
                find_first_in_str_ci(&window, query, query_lower, is_ascii)
            {
                if m_off < tail.len() {
                    let abs_m = tail_abs_start + m_off;
                    if abs_m + m_len <= e {
                        return Some((abs_m, abs_m + m_len));
                    }
                }
            }
        }

        if let Some((m_off, m_len)) = find_first_in_str_ci(seg, query, query_lower, is_ascii) {
            let abs_m = abs_pos + m_off;
            if abs_m + m_len <= e {
                return Some((abs_m, abs_m + m_len));
            }
        }

        if overlap_cap > 0 {
            let combined_len = tail.len() + seg.len();
            if combined_len <= overlap_cap {
                if tail.is_empty() {
                    tail_abs_start = abs_pos;
                }
                tail.push_str(seg);
            } else if seg.len() >= overlap_cap {
                let mut cut = seg.len() - overlap_cap;
                while cut < seg.len() && !seg.is_char_boundary(cut) {
                    cut += 1;
                }
                tail.clear();
                tail.push_str(&seg[cut..]);
                tail_abs_start = abs_pos + cut;
            } else {
                tail.push_str(seg);
                let mut cut = tail.len() - overlap_cap;
                while cut < tail.len() && !tail.is_char_boundary(cut) {
                    cut += 1;
                }
                tail.drain(..cut);
                tail_abs_start += cut;
            }
        }

        abs_pos += take;
        rem -= take;
        p_idx += 1;
        local = 0;
    }

    None
}

fn find_backward_range(
    buf: &PagedBuffer,
    start_byte: usize,
    end_byte: usize,
    query: &str,
    query_lower: &str,
    is_ascii: bool,
) -> Option<(usize, usize)> {
    let s = buf.clamp_char_boundary(start_byte);
    let e = buf.clamp_char_boundary(end_byte);
    if s >= e || query.is_empty() || buf.pages().is_empty() {
        return None;
    }

    let max_match_bytes = if is_ascii {
        query_lower.len()
    } else {
        query_lower.chars().count().saturating_mul(4).max(4)
    };
    let overlap_cap = max_match_bytes.saturating_sub(1);

    let (sp, sl) = buf.locate_byte(s);
    let (ep, el) = buf.locate_byte(e);

    if sp == ep {
        let seg = &buf.pages()[sp].buf[sl..el];
        return find_last_in_str_ci(seg, query, query_lower, is_ascii)
            .map(|(off, len)| (s + off, s + off + len));
    }

    let mut cur_p = ep;
    let mut cur_end_abs = e;
    let mut head_next = String::new();
    let mut window = String::new();

    loop {
        let seg_start_local = if cur_p == sp { sl } else { 0 };
        let seg_end_local = if cur_p == ep {
            el
        } else {
            buf.pages()[cur_p].buf.len()
        };
        let seg = &buf.pages()[cur_p].buf[seg_start_local..seg_end_local];
        let seg_start_abs = cur_end_abs - seg.len();

        let mut best: Option<(usize, usize)> = None;

        if !head_next.is_empty() && overlap_cap > 0 && !seg.is_empty() {
            let mut tail_cut = seg.len().saturating_sub(overlap_cap);
            while tail_cut < seg.len() && !seg.is_char_boundary(tail_cut) {
                tail_cut += 1;
            }
            window.clear();
            window.push_str(&seg[tail_cut..]);
            window.push_str(&head_next);
            if let Some((m_off, m_len)) = find_last_in_str_ci(&window, query, query_lower, is_ascii)
            {
                let abs_m = seg_start_abs + tail_cut + m_off;
                if abs_m >= s && abs_m + m_len <= e {
                    best = Some((abs_m, abs_m + m_len));
                }
            }
        }

        if best.is_none() {
            if let Some((m_off, m_len)) = find_last_in_str_ci(seg, query, query_lower, is_ascii) {
                let abs_m = seg_start_abs + m_off;
                if abs_m >= s && abs_m + m_len <= e {
                    best = Some((abs_m, abs_m + m_len));
                }
            }
        }

        if best.is_some() {
            return best;
        }

        if overlap_cap > 0 && !seg.is_empty() {
            let mut combined = String::with_capacity(seg.len().min(overlap_cap) + head_next.len());
            let take = seg
                .char_indices()
                .nth(query_lower.chars().count())
                .map(|(i, _)| i)
                .unwrap_or(seg.len());
            combined.push_str(&seg[..take]);
            if combined.len() < overlap_cap && !head_next.is_empty() {
                let rem_cap = overlap_cap - combined.len();
                let mut cut = rem_cap.min(head_next.len());
                while cut > 0 && !head_next.is_char_boundary(cut) {
                    cut -= 1;
                }
                combined.push_str(&head_next[..cut]);
            }
            head_next = combined;
        }

        if cur_p == sp || cur_p == 0 {
            break;
        }
        cur_end_abs = seg_start_abs;
        cur_p -= 1;
    }

    None
}

pub fn find_text(
    buf: &PagedBuffer,
    query: &str,
    from_byte: usize,
    forward: bool,
    wrap: bool,
) -> Option<(usize, usize)> {
    if query.is_empty() || buf.byte_len() == 0 {
        return None;
    }
    let is_ascii = query.is_ascii();
    let query_lower = fold_str_lower(query);
    if query_lower.is_empty() {
        return None;
    }

    let origin = buf.clamp_char_boundary(from_byte);
    if forward {
        if let Some(hit) =
            find_forward_range(buf, origin, buf.byte_len(), query, &query_lower, is_ascii)
        {
            return Some(hit);
        }
        if wrap && origin > 0 {
            let wrap_end = (origin + query.len().saturating_mul(4)).min(buf.byte_len());
            return find_forward_range(buf, 0, wrap_end, query, &query_lower, is_ascii);
        }
        None
    } else {
        if origin > 0 {
            if let Some(hit) = find_backward_range(buf, 0, origin, query, &query_lower, is_ascii) {
                return Some(hit);
            }
        }
        if wrap && origin < buf.byte_len() {
            let wrap_start = origin.saturating_sub(query.len().saturating_mul(4));
            return find_backward_range(
                buf,
                wrap_start,
                buf.byte_len(),
                query,
                &query_lower,
                is_ascii,
            );
        }
        None
    }
}

pub fn matches_exact_ci(
    buf: &PagedBuffer,
    start_byte: usize,
    end_byte: usize,
    query: &str,
) -> bool {
    if query.is_empty() || end_byte <= start_byte {
        return false;
    }
    if end_byte - start_byte > query.len().saturating_mul(4) + 4 {
        return false;
    }
    let sel = buf.slice_to_string(start_byte, end_byte);
    let q_lower = fold_str_lower(query);
    match_at_char_ci(&sel, &q_lower) == Some(sel.len())
}

pub fn match_stats(
    buf: &PagedBuffer,
    query: &str,
    sel_start: usize,
    sel_end: usize,
    max_cap: usize,
) -> (usize, usize) {
    if query.is_empty() || buf.byte_len() == 0 {
        return (0, 0);
    }
    let is_ascii = query.is_ascii();
    let query_lower = fold_str_lower(query);
    if query_lower.is_empty() {
        return (0, 0);
    }

    let s_min = sel_start.min(sel_end);
    let s_max = sel_start.max(sel_end);
    let mut total = 0usize;
    let mut current = 0usize;
    let mut pos = 0usize;

    while pos < buf.byte_len() {
        let Some((ms, me)) =
            find_forward_range(buf, pos, buf.byte_len(), query, &query_lower, is_ascii)
        else {
            break;
        };
        total += 1;
        if ms == s_min && me == s_max {
            current = total;
        }
        pos = if me > ms {
            me
        } else {
            buf.next_char_boundary(ms)
        };
        if total >= max_cap && pos > s_max {
            break;
        }
    }

    (current, total)
}

pub fn build_replace_all(
    buf: &PagedBuffer,
    query: &str,
    replacement: &str,
) -> Option<(String, usize)> {
    if query.is_empty() || buf.byte_len() == 0 {
        return None;
    }
    let is_ascii = query.is_ascii();
    let query_lower = fold_str_lower(query);
    if query_lower.is_empty() {
        return None;
    }

    let first_hit = find_forward_range(buf, 0, buf.byte_len(), query, &query_lower, is_ascii)?;
    let mut out = String::with_capacity(buf.byte_len());
    let mut scratch = String::new();
    let mut count = 0usize;
    let mut last = 0usize;
    let mut next_hit = Some(first_hit);

    while let Some((ms, me)) = next_hit {
        if ms > last {
            buf.copy_range_into(last, ms, &mut scratch);
            out.push_str(&scratch);
        }
        out.push_str(replacement);
        count += 1;
        last = if me > ms {
            me
        } else {
            buf.next_char_boundary(ms)
        };
        next_hit = if last < buf.byte_len() {
            find_forward_range(buf, last, buf.byte_len(), query, &query_lower, is_ascii)
        } else {
            None
        };
    }

    if last < buf.byte_len() {
        buf.copy_range_into(last, buf.byte_len(), &mut scratch);
        out.push_str(&scratch);
    }

    Some((out, count))
}
