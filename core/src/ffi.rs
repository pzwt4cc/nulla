use crate::edit::NullaDoc;
use crate::syntax::SUPPORTED_LANGUAGES;
use crate::types::{MoveDir, ViewportSnapshot};
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr;
use std::slice;

unsafe fn slice_to_lossy_str<'a>(ptr: *const u8, len: usize) -> std::borrow::Cow<'a, str> {
    if ptr.is_null() || len == 0 {
        return std::borrow::Cow::Borrowed("");
    }
    let raw = unsafe { slice::from_raw_parts(ptr, len) };
    String::from_utf8_lossy(raw)
}

unsafe fn write_str_out(s: &str, out_ptr: *mut *const u8, out_len: *mut usize) {
    if !out_ptr.is_null() {
        unsafe { *out_ptr = s.as_ptr() };
    }
    if !out_len.is_null() {
        unsafe { *out_len = s.len() };
    }
}

#[no_mangle]
pub extern "C" fn nulla_doc_new() -> *mut NullaDoc {
    catch_unwind(|| Box::into_raw(Box::new(NullaDoc::new()))).unwrap_or(ptr::null_mut())
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_free(doc: *mut NullaDoc) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        drop(Box::from_raw(doc));
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_clear(doc: *mut NullaDoc) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let d = &mut *doc;
        d.buf.clear();
        d.cursor = 0;
        d.anchor = 0;
        d.desired_vis_col = None;
        d.syntax_state.invalidate();
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_load_file(
    doc: *mut NullaDoc,
    path_ptr: *const u8,
    path_len: usize,
) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        let path = slice_to_lossy_str(path_ptr, path_len);
        (*doc).load_file(&path).is_ok()
    }))
    .unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_save_file(
    doc: *mut NullaDoc,
    path_ptr: *const u8,
    path_len: usize,
) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        let path = slice_to_lossy_str(path_ptr, path_len);
        (*doc).save_file(&path).is_ok()
    }))
    .unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_set_text(
    doc: *mut NullaDoc,
    text_ptr: *const u8,
    text_len: usize,
    path_ptr: *const u8,
    path_len: usize,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let raw = if text_ptr.is_null() || text_len == 0 {
            &[][..]
        } else {
            slice::from_raw_parts(text_ptr, text_len)
        };
        let path = slice_to_lossy_str(path_ptr, path_len);
        (*doc).set_text(raw, &path);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_get_text(
    doc: *mut NullaDoc,
    out_ptr: *mut *const u8,
    out_len: *mut usize,
) {
    if doc.is_null() {
        unsafe { write_str_out("", out_ptr, out_len) };
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let d = &mut *doc;
        let len = d.buf.byte_len();
        d.buf.copy_range_into(0, len, &mut d.last_string_result);
        write_str_out(&d.last_string_result, out_ptr, out_len);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_byte_len(doc: *const NullaDoc) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).buf.byte_len() })).unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_line_count(doc: *const NullaDoc) -> usize {
    if doc.is_null() {
        return 1;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).buf.line_count() })).unwrap_or(1)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_max_line_cols_hint(doc: *const NullaDoc) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).buf.max_line_cols_hint()
    }))
    .unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_revision(doc: *const NullaDoc) -> u64 {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).buf.revision() })).unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_is_modified(doc: *const NullaDoc) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).buf.is_modified() })).unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_mark_saved(doc: *mut NullaDoc) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).buf.mark_saved();
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_can_undo(doc: *const NullaDoc) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).buf.can_undo() })).unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_can_redo(doc: *const NullaDoc) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).buf.can_redo() })).unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_set_syntax_enabled(doc: *mut NullaDoc, enabled: bool) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).syntax_enabled = enabled;
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_set_language(
    doc: *mut NullaDoc,
    lang_ptr: *const u8,
    lang_len: usize,
    path_ptr: *const u8,
    path_len: usize,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let lang = slice_to_lossy_str(lang_ptr, lang_len);
        let path = slice_to_lossy_str(path_ptr, path_len);
        (*doc).set_language(&lang, &path);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_refresh_auto_language(
    doc: *mut NullaDoc,
    path_ptr: *const u8,
    path_len: usize,
) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        let path = slice_to_lossy_str(path_ptr, path_len);
        (*doc).refresh_auto_language(&path)
    }))
    .unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_language_id(
    doc: *const NullaDoc,
    out_ptr: *mut *const u8,
    out_len: *mut usize,
) {
    if doc.is_null() {
        unsafe { write_str_out("plain", out_ptr, out_len) };
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        write_str_out((*doc).lang.as_str(), out_ptr, out_len);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_language_name(
    doc: *const NullaDoc,
    out_ptr: *mut *const u8,
    out_len: *mut usize,
) {
    if doc.is_null() {
        unsafe { write_str_out("Plain Text", out_ptr, out_len) };
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        write_str_out((*doc).lang.display_name(), out_ptr, out_len);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_comment_prefix(
    doc: *const NullaDoc,
    out_ptr: *mut *const u8,
    out_len: *mut usize,
) {
    if doc.is_null() {
        unsafe { write_str_out("", out_ptr, out_len) };
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        write_str_out((*doc).lang.comment_prefix(), out_ptr, out_len);
    }));
}

#[no_mangle]
pub extern "C" fn nulla_supported_languages_count() -> usize {
    SUPPORTED_LANGUAGES.len()
}

#[no_mangle]
pub unsafe extern "C" fn nulla_supported_language_at(
    idx: usize,
    id_ptr: *mut *const u8,
    id_len: *mut usize,
    name_ptr: *mut *const u8,
    name_len: *mut usize,
) -> bool {
    let Some(&lang) = SUPPORTED_LANGUAGES.get(idx) else {
        return false;
    };
    unsafe {
        write_str_out(lang.as_str(), id_ptr, id_len);
        write_str_out(lang.display_name(), name_ptr, name_len);
    }
    true
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_cursor_byte(doc: *const NullaDoc) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).cursor })).unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_anchor_byte(doc: *const NullaDoc) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).anchor })).unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_cursor_pos(
    doc: *const NullaDoc,
    out_line_1: *mut usize,
    out_col_1: *mut usize,
    out_vis_col_0: *mut usize,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let (l, c, vc) = (*doc).cursor_line_col();
        if !out_line_1.is_null() {
            *out_line_1 = l;
        }
        if !out_col_1.is_null() {
            *out_col_1 = c;
        }
        if !out_vis_col_0.is_null() {
            *out_vis_col_0 = vc;
        }
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_byte_to_line_vis_col(
    doc: *const NullaDoc,
    byte_pos: usize,
    out_line_0: *mut usize,
    out_vis_col_0: *mut usize,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let (l, vc) = (*doc).byte_to_line_vis_col(byte_pos);
        if !out_line_0.is_null() {
            *out_line_0 = l;
        }
        if !out_vis_col_0.is_null() {
            *out_vis_col_0 = vc;
        }
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_line_start_byte(doc: *const NullaDoc, line_0: usize) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).buf.line_start_byte(line_0)
    }))
    .unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_has_selection(doc: *const NullaDoc) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).has_selection() })).unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_has_single_line_selection(doc: *const NullaDoc) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).has_single_line_selection()
    }))
    .unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_selected_chars(doc: *const NullaDoc) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).selected_char_count() })).unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_selected_text(
    doc: *mut NullaDoc,
    out_ptr: *mut *const u8,
    out_len: *mut usize,
) {
    if doc.is_null() {
        unsafe { write_str_out("", out_ptr, out_len) };
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let d = &mut *doc;
        let (s, e) = d.selection_range();
        d.buf.copy_range_into(s, e, &mut d.last_string_result);
        write_str_out(&d.last_string_result, out_ptr, out_len);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_set_selection(
    doc: *mut NullaDoc,
    anchor_byte: usize,
    cursor_byte: usize,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).set_selection(anchor_byte, cursor_byte);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_select_all(doc: *mut NullaDoc) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).select_all();
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_hit_test_byte(
    doc: *const NullaDoc,
    line_0: usize,
    vis_col_0: usize,
) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).hit_test_byte(line_0, vis_col_0)
    }))
    .unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_select_word_at(
    doc: *mut NullaDoc,
    line_0: usize,
    vis_col_0: usize,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).select_word_at(line_0, vis_col_0);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_select_line_at(doc: *mut NullaDoc, line_0: usize) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).select_line_at(line_0);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_select_current_line(doc: *mut NullaDoc) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).select_current_line();
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_move_cursor(
    doc: *mut NullaDoc,
    dir: i32,
    extend: bool,
    page_lines: usize,
) {
    if doc.is_null() {
        return;
    }
    let Some(d) = MoveDir::from_i32(dir) else {
        return;
    };
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).move_cursor(d, extend, page_lines);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_insert_text(
    doc: *mut NullaDoc,
    text_ptr: *const u8,
    text_len: usize,
    ts_ms: u64,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let s = slice_to_lossy_str(text_ptr, text_len);
        (*doc).insert_text(&s, ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_insert_newline(doc: *mut NullaDoc, ts_ms: u64) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).insert_newline_auto_indent(ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_backspace(doc: *mut NullaDoc, word: bool, ts_ms: u64) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).backspace(word, ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_delete_forward(doc: *mut NullaDoc, word: bool, ts_ms: u64) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).delete_forward(word, ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_indent(doc: *mut NullaDoc, unindent: bool, ts_ms: u64) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).indent_selection(unindent, ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_toggle_comment(doc: *mut NullaDoc, ts_ms: u64) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).toggle_comment(ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_duplicate(doc: *mut NullaDoc, ts_ms: u64) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).duplicate_selection_or_line(ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_delete_lines(doc: *mut NullaDoc, ts_ms: u64) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).delete_lines(ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_insert_line_sibling(
    doc: *mut NullaDoc,
    above: bool,
    ts_ms: u64,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        (*doc).insert_line_sibling(above, ts_ms);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_move_lines(doc: *mut NullaDoc, up: bool, ts_ms: u64) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).move_lines(up, ts_ms) })).unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_copy_or_line(
    doc: *mut NullaDoc,
    out_ptr: *mut *const u8,
    out_len: *mut usize,
) {
    if doc.is_null() {
        unsafe { write_str_out("", out_ptr, out_len) };
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let res = (*doc).copy_selection_or_line();
        write_str_out(res, out_ptr, out_len);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_cut_or_line(
    doc: *mut NullaDoc,
    ts_ms: u64,
    out_ptr: *mut *const u8,
    out_len: *mut usize,
) {
    if doc.is_null() {
        unsafe { write_str_out("", out_ptr, out_len) };
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let res = (*doc).cut_selection_or_line(ts_ms);
        write_str_out(res, out_ptr, out_len);
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_undo(doc: *mut NullaDoc) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).undo() })).unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_redo(doc: *mut NullaDoc) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe { (*doc).redo() })).unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_find(
    doc: *mut NullaDoc,
    q_ptr: *const u8,
    q_len: usize,
    forward: bool,
    wrap: bool,
) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        let q = slice_to_lossy_str(q_ptr, q_len);
        (*doc).find(&q, forward, wrap)
    }))
    .unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_match_stats(
    doc: *const NullaDoc,
    q_ptr: *const u8,
    q_len: usize,
    out_cur: *mut usize,
    out_total: *mut usize,
) {
    if doc.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| unsafe {
        let q = slice_to_lossy_str(q_ptr, q_len);
        let (cur, tot) = (*doc).match_stats(&q);
        if !out_cur.is_null() {
            *out_cur = cur;
        }
        if !out_total.is_null() {
            *out_total = tot;
        }
    }));
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_replace_current_and_find(
    doc: *mut NullaDoc,
    q_ptr: *const u8,
    q_len: usize,
    r_ptr: *const u8,
    r_len: usize,
    ts_ms: u64,
) -> bool {
    if doc.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        let q = slice_to_lossy_str(q_ptr, q_len);
        let r = slice_to_lossy_str(r_ptr, r_len);
        (*doc).replace_current_and_find(&q, &r, ts_ms)
    }))
    .unwrap_or(false)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_replace_all(
    doc: *mut NullaDoc,
    q_ptr: *const u8,
    q_len: usize,
    r_ptr: *const u8,
    r_len: usize,
    ts_ms: u64,
) -> usize {
    if doc.is_null() {
        return 0;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        let q = slice_to_lossy_str(q_ptr, q_len);
        let r = slice_to_lossy_str(r_ptr, r_len);
        (*doc).replace_all(&q, &r, ts_ms)
    }))
    .unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn nulla_doc_build_viewport(
    doc: *mut NullaDoc,
    first_line: usize,
    line_count: usize,
    first_vis_col: usize,
    max_vis_cols: usize,
    include_guides: bool,
    out_snap: *mut ViewportSnapshot,
) -> bool {
    if doc.is_null() || out_snap.is_null() {
        return false;
    }
    catch_unwind(AssertUnwindSafe(|| unsafe {
        *out_snap = (*doc).build_viewport(
            first_line,
            line_count,
            first_vis_col,
            max_vis_cols,
            include_guides,
        );
        true
    }))
    .unwrap_or(false)
}
