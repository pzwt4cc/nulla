pub mod buffer;
pub mod edit;
pub mod ffi;
pub mod indent;
pub mod search;
pub mod syntax;
pub mod types;

#[cfg(test)]
mod tests {
    use crate::edit::NullaDoc;
    use crate::syntax::LangId;
    use crate::types::MoveDir;

    #[test]
    fn paged_buffer_large_lines_and_utf8() {
        let mut doc = NullaDoc::new();
        let mut raw = String::new();
        for i in 0..50_000 {
            raw.push_str(&format!("fn строка_{i}(x: usize) -> usize {{ x + {i} }}\n"));
        }
        doc.set_text(raw.as_bytes(), "test.rs");
        assert_eq!(doc.lang, LangId::Rust);
        assert_eq!(doc.buf.line_count(), 50_001);

        let snap = doc.build_viewport(25_000, 40, 0, 120, true);
        assert_eq!(snap.line_count, 40);
        assert!(snap.token_count >= 40);

        doc.move_cursor(MoveDir::DocEnd, false, 30);
        let (l, _, _) = doc.cursor_line_col();
        assert_eq!(l, 50_001);

        doc.insert_text("let z = 42;", 100);
        assert!(doc.buf.is_modified());
        assert!(doc.undo());
        assert!(!doc.buf.is_modified());
        assert!(doc.redo());
        assert!(doc.buf.is_modified());
    }

    #[test]
    fn single_line_megabyte_horizontal_window() {
        let mut doc = NullaDoc::new();
        let huge = "a".repeat(1_200_000);
        doc.set_text(huge.as_bytes(), "min.json");
        assert_eq!(doc.buf.line_count(), 1);

        let snap = doc.build_viewport(0, 10, 500_000, 100, false);
        assert_eq!(snap.line_count, 1);
        assert_eq!(snap.utf8_len, 100);
    }

    #[test]
    fn search_and_replace_across_pages_ci() {
        let mut doc = NullaDoc::new();
        let repeated = "Hello Привет world\n".repeat(1500);
        doc.set_text(repeated.as_bytes(), "notes.txt");

        assert!(doc.find("привет", true, true));
        let (cur, total) = doc.match_stats("ПРИВЕТ");
        assert_eq!(cur, 1);
        assert_eq!(total, 1500);

        let replaced = doc.replace_all("привет", "Мир", 200);
        assert_eq!(replaced, 1500);
        let (_, after_total) = doc.match_stats("привет");
        assert_eq!(after_total, 0);
        assert!(doc.undo());
        let (_, restored_total) = doc.match_stats("привет");
        assert_eq!(restored_total, 1500);
    }

    #[test]
    fn indent_comment_and_move_lines() {
        let mut doc = NullaDoc::new();
        doc.set_text(b"int a = 1;\nint b = 2;\nint c = 3;", "main.cpp");
        assert_eq!(doc.lang, LangId::C);

        doc.toggle_comment(300);
        let t1 = doc.buf.slice_to_string(0, doc.buf.byte_len());
        assert!(t1.starts_with("// int a = 1;"));

        doc.toggle_comment(310);
        let t2 = doc.buf.slice_to_string(0, doc.buf.byte_len());
        assert_eq!(t2, "int a = 1;\nint b = 2;\nint c = 3;");

        assert!(doc.move_lines(false, 320));
        let t3 = doc.buf.slice_to_string(0, doc.buf.byte_len());
        assert_eq!(t3, "int b = 2;\nint a = 1;\nint c = 3;");
    }

    #[test]
    fn indent_cache_and_bounds_safety() {
        let mut doc = NullaDoc::new();
        doc.set_text(b"if a:\n  x = 1\n  y = 2\n", "a.py");
        let _ = doc.build_viewport(0, 10, 0, 80, true);
        assert_eq!(doc.indent_state.indent_step(&doc.buf), 2);

        doc.set_text(b"fn main() {\n    let x = 1;\n}\n", "b.rs");
        assert_eq!(doc.indent_state.indent_step(&doc.buf), 4);

        doc.move_cursor(MoveDir::PageDown, false, usize::MAX);
        assert_eq!(doc.cursor, doc.buf.byte_len());

        let mut offsets = vec![(0, 0); 2];
        let mut guides = Vec::new();
        doc.indent_state.compute_viewport_guides(
            &doc.buf,
            10_000,
            usize::MAX,
            0,
            &mut offsets,
            &mut guides,
        );

        doc.set_text(b"hello world", "t.txt");
        doc.cursor = 5;
        doc.anchor = 5;
        doc.replace_utf16_range(-3, 4, "X", 100);
        let s = doc.buf.slice_to_string(0, doc.buf.byte_len());
        assert_eq!(s, "heXworld");
    }
}
