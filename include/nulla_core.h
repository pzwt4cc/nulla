#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NullaDoc NullaDoc;

enum NullaTokenKind {
  NULLA_TOK_PLAIN = 0,
  NULLA_TOK_KEYWORD = 1,
  NULLA_TOK_TYPE = 2,
  NULLA_TOK_NUMBER = 3,
  NULLA_TOK_KEY = 4,
  NULLA_TOK_STRING = 5,
  NULLA_TOK_COMMENT = 6
};

enum NullaMoveDir {
  NULLA_MOVE_LEFT = 0,
  NULLA_MOVE_RIGHT = 1,
  NULLA_MOVE_UP = 2,
  NULLA_MOVE_DOWN = 3,
  NULLA_MOVE_WORD_LEFT = 4,
  NULLA_MOVE_WORD_RIGHT = 5,
  NULLA_MOVE_HOME = 6,
  NULLA_MOVE_END = 7,
  NULLA_MOVE_DOC_START = 8,
  NULLA_MOVE_DOC_END = 9,
  NULLA_MOVE_PAGE_UP = 10,
  NULLA_MOVE_PAGE_DOWN = 11
};

typedef struct NullaVisToken {
  uint32_t rel_start;
  uint32_t rel_len;
  uint8_t kind;
  uint8_t _pad[3];
} NullaVisToken;

typedef struct NullaVisGuide {
  uint32_t vis_col;
  uint8_t is_active;
  uint8_t _pad[3];
} NullaVisGuide;

typedef struct NullaVisLine {
  size_t line_idx;
  uint32_t text_offset;
  uint32_t text_len;
  size_t start_vis_col;
  size_t line_total_vis_cols;
  uint32_t token_offset;
  uint32_t token_count;
  uint32_t guide_offset;
  uint32_t guide_count;
  uint32_t sel_rel_start;
  uint32_t sel_rel_end;
  uint8_t has_selection;
  uint8_t sel_before_slice;
  uint8_t eol_selected;
  uint8_t _pad;
} NullaVisLine;

typedef struct NullaViewportSnapshot {
  const uint8_t *utf8_data;
  size_t utf8_len;
  const NullaVisLine *lines;
  size_t line_count;
  const NullaVisToken *tokens;
  size_t token_count;
  const NullaVisGuide *guides;
  size_t guide_count;
} NullaViewportSnapshot;

NullaDoc *nulla_doc_new(void);
void nulla_doc_free(NullaDoc *doc);
void nulla_doc_clear(NullaDoc *doc);

bool nulla_doc_load_file(NullaDoc *doc, const uint8_t *path_ptr,
                         size_t path_len);
bool nulla_doc_save_file(NullaDoc *doc, const uint8_t *path_ptr,
                         size_t path_len);
void nulla_doc_set_text(NullaDoc *doc, const uint8_t *text_ptr, size_t text_len,
                        const uint8_t *path_ptr, size_t path_len);
void nulla_doc_get_text(NullaDoc *doc, const uint8_t **out_ptr,
                        size_t *out_len);

size_t nulla_doc_byte_len(const NullaDoc *doc);
size_t nulla_doc_line_count(const NullaDoc *doc);
size_t nulla_doc_max_line_cols_hint(const NullaDoc *doc);
uint64_t nulla_doc_revision(const NullaDoc *doc);
bool nulla_doc_is_modified(const NullaDoc *doc);
void nulla_doc_mark_saved(NullaDoc *doc);
bool nulla_doc_can_undo(const NullaDoc *doc);
bool nulla_doc_can_redo(const NullaDoc *doc);

void nulla_doc_set_syntax_enabled(NullaDoc *doc, bool enabled);
void nulla_doc_set_language(NullaDoc *doc, const uint8_t *lang_ptr,
                            size_t lang_len, const uint8_t *path_ptr,
                            size_t path_len);
bool nulla_doc_refresh_auto_language(NullaDoc *doc, const uint8_t *path_ptr,
                                     size_t path_len);
void nulla_doc_language_id(const NullaDoc *doc, const uint8_t **out_ptr,
                           size_t *out_len);
void nulla_doc_language_name(const NullaDoc *doc, const uint8_t **out_ptr,
                             size_t *out_len);
void nulla_doc_comment_prefix(const NullaDoc *doc, const uint8_t **out_ptr,
                              size_t *out_len);

size_t nulla_supported_languages_count(void);
bool nulla_supported_language_at(size_t idx, const uint8_t **id_ptr,
                                 size_t *id_len, const uint8_t **name_ptr,
                                 size_t *name_len);

size_t nulla_doc_cursor_byte(const NullaDoc *doc);
size_t nulla_doc_anchor_byte(const NullaDoc *doc);
void nulla_doc_cursor_pos(const NullaDoc *doc, size_t *out_line_1,
                          size_t *out_col_1, size_t *out_vis_col_0);
void nulla_doc_byte_to_line_vis_col(const NullaDoc *doc, size_t byte_pos,
                                    size_t *out_line_0, size_t *out_vis_col_0);
size_t nulla_doc_line_start_byte(const NullaDoc *doc, size_t line_0);

bool nulla_doc_has_selection(const NullaDoc *doc);
bool nulla_doc_has_single_line_selection(const NullaDoc *doc);
size_t nulla_doc_selected_chars(const NullaDoc *doc);
void nulla_doc_selected_text(NullaDoc *doc, const uint8_t **out_ptr,
                             size_t *out_len);
void nulla_doc_set_selection(NullaDoc *doc, size_t anchor_byte,
                             size_t cursor_byte);
void nulla_doc_select_all(NullaDoc *doc);
size_t nulla_doc_hit_test_byte(const NullaDoc *doc, size_t line_0,
                               size_t vis_col_0);
void nulla_doc_select_word_at(NullaDoc *doc, size_t line_0, size_t vis_col_0);
void nulla_doc_select_line_at(NullaDoc *doc, size_t line_0);
void nulla_doc_select_current_line(NullaDoc *doc);
void nulla_doc_move_cursor(NullaDoc *doc, int32_t dir, bool extend,
                           size_t page_lines);

void nulla_doc_insert_text(NullaDoc *doc, const uint8_t *text_ptr,
                           size_t text_len, uint64_t ts_ms);
void nulla_doc_insert_newline(NullaDoc *doc, uint64_t ts_ms);
void nulla_doc_backspace(NullaDoc *doc, bool word, uint64_t ts_ms);
void nulla_doc_delete_forward(NullaDoc *doc, bool word, uint64_t ts_ms);
void nulla_doc_indent(NullaDoc *doc, bool unindent, uint64_t ts_ms);
void nulla_doc_toggle_comment(NullaDoc *doc, uint64_t ts_ms);
void nulla_doc_duplicate(NullaDoc *doc, uint64_t ts_ms);
void nulla_doc_delete_lines(NullaDoc *doc, uint64_t ts_ms);
void nulla_doc_insert_line_sibling(NullaDoc *doc, bool above, uint64_t ts_ms);
bool nulla_doc_move_lines(NullaDoc *doc, bool up, uint64_t ts_ms);
void nulla_doc_copy_or_line(NullaDoc *doc, const uint8_t **out_ptr,
                            size_t *out_len);
void nulla_doc_cut_or_line(NullaDoc *doc, uint64_t ts_ms,
                           const uint8_t **out_ptr, size_t *out_len);
bool nulla_doc_undo(NullaDoc *doc);
bool nulla_doc_redo(NullaDoc *doc);

bool nulla_doc_find(NullaDoc *doc, const uint8_t *q_ptr, size_t q_len,
                    bool forward, bool wrap);
void nulla_doc_match_stats(const NullaDoc *doc, const uint8_t *q_ptr,
                           size_t q_len, size_t *out_cur, size_t *out_total);
bool nulla_doc_replace_current_and_find(NullaDoc *doc, const uint8_t *q_ptr,
                                        size_t q_len, const uint8_t *r_ptr,
                                        size_t r_len, uint64_t ts_ms);
size_t nulla_doc_replace_all(NullaDoc *doc, const uint8_t *q_ptr, size_t q_len,
                             const uint8_t *r_ptr, size_t r_len,
                             uint64_t ts_ms);

bool nulla_doc_build_viewport(NullaDoc *doc, size_t first_line,
                              size_t line_count, size_t first_vis_col,
                              size_t max_vis_cols, bool include_guides,
                              NullaViewportSnapshot *out_snap);

#ifdef __cplusplus
}
#endif
