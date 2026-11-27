#![forbid(unsafe_code)]

use crate::buffer::PagedBuffer;
use crate::types::{TokenKind, VisToken};

#[repr(u8)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LangId {
    Plain = 0,
    Bash = 1,
    C = 2,
    Css = 3,
    Go = 4,
    Html = 5,
    Ini = 6,
    Js = 7,
    Json = 8,
    Make = 9,
    Markdown = 10,
    Python = 11,
    Rust = 12,
    Toml = 13,
    Yaml = 14,
}

pub const SUPPORTED_LANGUAGES: &[LangId] = &[
    LangId::Plain,
    LangId::Bash,
    LangId::C,
    LangId::Css,
    LangId::Go,
    LangId::Html,
    LangId::Ini,
    LangId::Js,
    LangId::Json,
    LangId::Make,
    LangId::Markdown,
    LangId::Python,
    LangId::Rust,
    LangId::Toml,
    LangId::Yaml,
];

impl LangId {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Plain => "plain",
            Self::Bash => "bash",
            Self::C => "c",
            Self::Css => "css",
            Self::Go => "go",
            Self::Html => "html",
            Self::Ini => "ini",
            Self::Js => "js",
            Self::Json => "json",
            Self::Make => "make",
            Self::Markdown => "markdown",
            Self::Python => "python",
            Self::Rust => "rust",
            Self::Toml => "toml",
            Self::Yaml => "yaml",
        }
    }

    pub fn display_name(self) -> &'static str {
        match self {
            Self::Plain => "Plain Text",
            Self::Bash => "Bash / Shell",
            Self::C => "C / C++",
            Self::Css => "CSS",
            Self::Go => "Go",
            Self::Html => "HTML / XML",
            Self::Ini => "INI / Config",
            Self::Js => "JavaScript / TS",
            Self::Json => "JSON",
            Self::Make => "Makefile / PKGBUILD",
            Self::Markdown => "Markdown",
            Self::Python => "Python",
            Self::Rust => "Rust",
            Self::Toml => "TOML",
            Self::Yaml => "YAML",
        }
    }

    pub fn comment_prefix(self) -> &'static str {
        match self {
            Self::C | Self::Js | Self::Rust | Self::Go | Self::Css => "//",
            Self::Html | Self::Markdown | Self::Json => "",
            _ => "#",
        }
    }

    pub fn from_id_str(s: &str) -> Self {
        match s.trim() {
            "bash" => Self::Bash,
            "c" => Self::C,
            "css" => Self::Css,
            "go" => Self::Go,
            "html" => Self::Html,
            "ini" => Self::Ini,
            "js" => Self::Js,
            "json" => Self::Json,
            "make" => Self::Make,
            "markdown" => Self::Markdown,
            "python" => Self::Python,
            "rust" => Self::Rust,
            "toml" => Self::Toml,
            "yaml" => Self::Yaml,
            _ => Self::Plain,
        }
    }

    pub fn has_multiline_state(self) -> bool {
        matches!(
            self,
            Self::C | Self::Js | Self::Rust | Self::Go | Self::Css | Self::Python | Self::Html
        )
    }
}

pub fn detect_language(file_path: &str, sample: &str) -> LangId {
    if !file_path.is_empty() {
        let file_name = file_path
            .rsplit(['/', '\\'])
            .next()
            .unwrap_or(file_path)
            .to_ascii_lowercase();
        let ext = file_name.rsplit_once('.').map(|(_, e)| e).unwrap_or("");

        if matches!(file_name.as_str(), "makefile" | "gnumakefile" | "pkgbuild") || ext == "mk" {
            return LangId::Make;
        }
        if matches!(file_name.as_str(), ".bashrc" | ".zshrc" | ".profile")
            || matches!(ext, "sh" | "bash" | "zsh")
        {
            return LangId::Bash;
        }
        if matches!(ext, "c" | "h" | "cpp" | "hpp" | "cc" | "cxx") {
            return LangId::C;
        }
        if matches!(ext, "py" | "pyw") {
            return LangId::Python;
        }
        if matches!(ext, "json" | "jsonc") {
            return LangId::Json;
        }
        if matches!(ext, "ini" | "conf" | "cfg" | "desktop" | "service" | "env") {
            return LangId::Ini;
        }
        if matches!(ext, "yml" | "yaml") {
            return LangId::Yaml;
        }
        if ext == "toml" {
            return LangId::Toml;
        }
        if matches!(ext, "md" | "markdown") {
            return LangId::Markdown;
        }
        if matches!(ext, "js" | "mjs" | "ts" | "tsx" | "jsx" | "qml") {
            return LangId::Js;
        }
        if ext == "rs" {
            return LangId::Rust;
        }
        if ext == "go" {
            return LangId::Go;
        }
        if matches!(ext, "css" | "scss") {
            return LangId::Css;
        }
        if matches!(ext, "html" | "htm" | "xml" | "svg" | "qrc") {
            return LangId::Html;
        }
    }

    detect_language_from_content(sample)
}

pub fn detect_language_from_content(sample: &str) -> LangId {
    let trimmed_all = sample.trim();
    if trimmed_all.is_empty() {
        return LangId::Plain;
    }
    let head_end = floor_boundary(sample, 4096);
    let head = &sample[..head_end];
    let first_non_empty = head
        .lines()
        .map(str::trim)
        .find(|l| !l.is_empty())
        .unwrap_or("");

    if first_non_empty.starts_with("#!") {
        if first_non_empty.contains("python") {
            return LangId::Python;
        }
        if first_non_empty.contains("sh")
            || first_non_empty.contains("bash")
            || first_non_empty.contains("zsh")
        {
            return LangId::Bash;
        }
        if first_non_empty.contains("node")
            || first_non_empty.contains("deno")
            || first_non_empty.contains("bun")
        {
            return LangId::Js;
        }
    }

    if (first_non_empty.starts_with('{') || first_non_empty.starts_with('['))
        && has_json_key_pattern(head)
    {
        return LangId::Json;
    }

    let lower_first = first_non_empty.to_ascii_lowercase();
    if lower_first.starts_with("<!doctype")
        || lower_first.starts_with("<html")
        || lower_first.starts_with("<svg")
        || lower_first.starts_with("<?xml")
    {
        return LangId::Html;
    }

    for line in head.lines() {
        let t = line.trim_start();
        if let Some(rest) = t.strip_prefix('#') {
            let directive = rest.trim_start();
            if starts_with_word(directive, "include")
                || starts_with_word(directive, "define")
                || starts_with_word(directive, "pragma")
                || starts_with_word(directive, "ifndef")
                || starts_with_word(directive, "ifdef")
            {
                return LangId::C;
            }
        }
    }
    if head.contains("std::")
        || head.contains("QString")
        || head.contains("nullptr")
        || head.contains("int main(")
        || head.contains("printf(")
    {
        return LangId::C;
    }

    for line in head.lines() {
        let t = line.trim_start();
        let after_pub = t.strip_prefix("pub ").map(str::trim_start).unwrap_or(t);
        if (after_pub.starts_with("fn ") && after_pub.contains('('))
            || (after_pub.starts_with("use ") && after_pub.ends_with(';'))
            || starts_with_word(after_pub, "impl")
            || after_pub.starts_with("let mut ")
            || t.contains("println!(")
        {
            return LangId::Rust;
        }
    }

    for line in head.lines() {
        let t = line.trim_start();
        if starts_with_word(t, "package")
            || (starts_with_word(t, "func") && t.contains('(') && t.ends_with('{'))
            || t.contains(":=")
        {
            return LangId::Go;
        }
    }

    for line in head.lines() {
        let t = line.trim_start();
        if (starts_with_word(t, "def") && t.contains('(') && t.ends_with(':'))
            || (starts_with_word(t, "from") && t.contains(" import "))
            || starts_with_word(t, "import")
                && !t.contains('{')
                && !t.ends_with(';')
                && !t.starts_with("import QtQuick")
            || t.starts_with("if __name__")
        {
            return LangId::Python;
        }
    }

    if head.contains("import QtQuick")
        || head.contains("console.log(")
        || head.contains("export default ")
        || head.contains("export const ")
        || head.contains("export function ")
        || head.contains("=>") && (head.contains("const ") || head.contains("let "))
    {
        return LangId::Js;
    }

    if (head.contains("@media") || head.contains("@keyframes")) && head.contains('{') {
        return LangId::Css;
    }

    for line in head.lines() {
        let t = line.trim();
        if t.starts_with("export ") && t.contains('=')
            || t.starts_with("echo ")
            || matches!(t, "fi" | "done" | "esac")
            || t.starts_with("if [")
        {
            return LangId::Bash;
        }
    }

    if head.contains("pkgname=") || head.contains("pkgver=") || head.contains(":\n\t") {
        return LangId::Make;
    }

    let mut has_section = false;
    let mut has_toml_assign = false;
    for line in head.lines() {
        let t = line.trim();
        if t.starts_with('[') && t.ends_with(']') && t.len() > 2 {
            has_section = true;
        } else if let Some((_, rhs)) = t.split_once('=') {
            let rt = rhs.trim_start();
            if rt.starts_with('"') || rt.starts_with('[') {
                has_toml_assign = true;
            }
        }
    }
    if has_section {
        return if has_toml_assign {
            LangId::Toml
        } else {
            LangId::Ini
        };
    }

    for line in head.lines() {
        if line.starts_with("# ")
            || line.starts_with("## ")
            || line.starts_with("### ")
            || line.starts_with("```")
        {
            return LangId::Markdown;
        }
    }

    if !head.contains(';') && !head.contains('{') {
        for line in head.lines() {
            let t = line.trim();
            if t == "---" {
                return LangId::Yaml;
            }
            if let Some((k, v)) = t.split_once(':') {
                if !k.is_empty()
                    && k.bytes()
                        .all(|b| b.is_ascii_alphanumeric() || matches!(b, b'_' | b'.' | b'-'))
                    && (v.starts_with(' ') || v.is_empty())
                {
                    return LangId::Yaml;
                }
            }
        }
    }

    LangId::Plain
}

fn floor_boundary(s: &str, mut idx: usize) -> usize {
    if idx >= s.len() {
        return s.len();
    }
    while idx > 0 && !s.is_char_boundary(idx) {
        idx -= 1;
    }
    idx
}

fn starts_with_word(s: &str, word: &str) -> bool {
    if !s.starts_with(word) {
        return false;
    }
    match s.as_bytes().get(word.len()) {
        None => true,
        Some(&b) => !(b.is_ascii_alphanumeric() || b == b'_'),
    }
}

fn has_json_key_pattern(s: &str) -> bool {
    let bytes = s.as_bytes();
    let mut i = 0usize;
    while i < bytes.len() {
        if bytes[i] == b'"' {
            i += 1;
            let start = i;
            while i < bytes.len() && bytes[i] != b'"' {
                if bytes[i] == b'\\' && i + 1 < bytes.len() {
                    i += 2;
                } else {
                    i += 1;
                }
            }
            if i > start && i < bytes.len() && bytes[i] == b'"' {
                i += 1;
                while i < bytes.len() && bytes[i].is_ascii_whitespace() {
                    i += 1;
                }
                if i < bytes.len() && bytes[i] == b':' {
                    return true;
                }
            }
        } else {
            i += 1;
        }
    }
    false
}

const CHECKPOINT_STRIDE: usize = 256;

pub struct SyntaxState {
    cached_rev: u64,
    cached_lang: LangId,
    checkpoints: Vec<u8>,
    scratch_line: String,
}

impl Default for SyntaxState {
    fn default() -> Self {
        Self::new()
    }
}

impl SyntaxState {
    pub fn new() -> Self {
        Self {
            cached_rev: u64::MAX,
            cached_lang: LangId::Plain,
            checkpoints: Vec::new(),
            scratch_line: String::new(),
        }
    }

    pub fn invalidate(&mut self) {
        self.cached_rev = u64::MAX;
        self.checkpoints.clear();
    }

    pub fn state_before_line(&mut self, buf: &PagedBuffer, lang: LangId, target_line: usize) -> u8 {
        if !lang.has_multiline_state() || target_line == 0 {
            return 0;
        }
        if self.cached_rev != buf.revision() || self.cached_lang != lang {
            self.cached_rev = buf.revision();
            self.cached_lang = lang;
            self.checkpoints.clear();
            self.checkpoints.push(0);
        }

        let target_cp = target_line / CHECKPOINT_STRIDE;
        let known_cp = self.checkpoints.len().saturating_sub(1);

        if target_cp > known_cp && (target_cp - known_cp) * CHECKPOINT_STRIDE <= 4096 {
            let mut state = self.checkpoints[known_cp];
            let mut line = known_cp * CHECKPOINT_STRIDE;
            let max_line = (target_cp * CHECKPOINT_STRIDE).min(buf.line_count());
            while line < max_line {
                state = self.advance_line_state(buf, lang, line, state);
                line += 1;
                if line % CHECKPOINT_STRIDE == 0 {
                    self.checkpoints.push(state);
                }
            }
        }

        if target_cp < self.checkpoints.len() {
            let mut state = self.checkpoints[target_cp];
            let start = target_cp * CHECKPOINT_STRIDE;
            for line in start..target_line.min(buf.line_count()) {
                state = self.advance_line_state(buf, lang, line, state);
            }
            state
        } else {
            let lookback_start = target_line.saturating_sub(512);
            let mut state = 0u8;
            for line in lookback_start..target_line.min(buf.line_count()) {
                state = self.advance_line_state(buf, lang, line, state);
            }
            state
        }
    }

    pub fn advance_line_state(
        &mut self,
        buf: &PagedBuffer,
        lang: LangId,
        line_idx: usize,
        in_state: u8,
    ) -> u8 {
        if !lang.has_multiline_state() {
            return 0;
        }
        let (s, e) = buf.line_byte_range(line_idx);
        if e - s > 16384 {
            return in_state;
        }
        buf.copy_range_into(s, e, &mut self.scratch_line);
        scan_multiline_only(lang, in_state, &self.scratch_line)
    }
}

fn scan_multiline_only(lang: LangId, mut state: u8, line: &str) -> u8 {
    let b = line.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    match lang {
        LangId::C | LangId::Js | LangId::Rust | LangId::Go | LangId::Css => {
            while i < len {
                if state == 1 {
                    if i + 1 < len && b[i] == b'*' && b[i + 1] == b'/' {
                        state = 0;
                        i += 2;
                    } else {
                        i += 1;
                    }
                } else if i + 1 < len && b[i] == b'/' && b[i + 1] == b'/' && lang != LangId::Css {
                    break;
                } else if i + 1 < len && b[i] == b'/' && b[i + 1] == b'*' {
                    state = 1;
                    i += 2;
                } else if b[i] == b'"'
                    || (b[i] == b'\'' && lang != LangId::Rust)
                    || (b[i] == b'`' && matches!(lang, LangId::Js | LangId::Go))
                {
                    let q = b[i];
                    i += 1;
                    while i < len && b[i] != q {
                        if b[i] == b'\\' && q != b'`' && i + 1 < len {
                            i += 2;
                        } else {
                            i += 1;
                        }
                    }
                    if i < len {
                        i += 1;
                    }
                } else {
                    i += 1;
                }
            }
        }
        LangId::Python => {
            while i < len {
                if state == 2 {
                    if i + 2 < len && &b[i..i + 3] == b"\"\"\"" {
                        state = 0;
                        i += 3;
                    } else if b[i] == b'\\' && i + 1 < len {
                        i += 2;
                    } else {
                        i += 1;
                    }
                } else if state == 3 {
                    if i + 2 < len && &b[i..i + 3] == b"'''" {
                        state = 0;
                        i += 3;
                    } else if b[i] == b'\\' && i + 1 < len {
                        i += 2;
                    } else {
                        i += 1;
                    }
                } else if b[i] == b'#' {
                    break;
                } else if i + 2 < len && &b[i..i + 3] == b"\"\"\"" {
                    state = 2;
                    i += 3;
                } else if i + 2 < len && &b[i..i + 3] == b"'''" {
                    state = 3;
                    i += 3;
                } else if b[i] == b'"' || b[i] == b'\'' {
                    let q = b[i];
                    i += 1;
                    while i < len && b[i] != q {
                        if b[i] == b'\\' && i + 1 < len {
                            i += 2;
                        } else {
                            i += 1;
                        }
                    }
                    if i < len {
                        i += 1;
                    }
                } else {
                    i += 1;
                }
            }
        }
        LangId::Html => {
            while i < len {
                if state == 4 {
                    if i + 2 < len && &b[i..i + 3] == b"-->" {
                        state = 0;
                        i += 3;
                    } else {
                        i += 1;
                    }
                } else if i + 3 < len && &b[i..i + 4] == b"<!--" {
                    state = 4;
                    i += 4;
                } else {
                    i += 1;
                }
            }
        }
        _ => {}
    }
    state
}

fn push_tok(
    out: &mut Vec<VisToken>,
    base_offset: usize,
    start: usize,
    end: usize,
    kind: TokenKind,
) {
    if end <= start {
        return;
    }
    let Ok(rel_start) = u32::try_from(base_offset.saturating_add(start)) else {
        return;
    };
    let Ok(rel_len) = u32::try_from(end - start) else {
        return;
    };
    let k = kind as u8;
    if let Some(last) = out.last_mut() {
        if last.kind == k && last.rel_start.checked_add(last.rel_len) == Some(rel_start) {
            if let Some(sum) = last.rel_len.checked_add(rel_len) {
                last.rel_len = sum;
                return;
            }
        }
    }
    out.push(VisToken {
        rel_start,
        rel_len,
        kind: k,
        _pad: [0; 3],
    });
}

pub fn tokenize_window(
    enabled: bool,
    lang: LangId,
    in_state: u8,
    slice: &str,
    slice_is_line_start: bool,
    out: &mut Vec<VisToken>,
) {
    if slice.is_empty() {
        return;
    }
    if !enabled || lang == LangId::Plain {
        push_tok(out, 0, 0, slice.len(), TokenKind::Plain);
        return;
    }

    match lang {
        LangId::C | LangId::Js | LangId::Rust | LangId::Go => {
            lex_code_c_like(lang, in_state, slice, slice_is_line_start, out);
        }
        LangId::Python => {
            lex_python(in_state, slice, out);
        }
        LangId::Bash | LangId::Make => {
            lex_shell_make(slice, slice_is_line_start, out);
        }
        LangId::Json => {
            lex_json(slice, out);
        }
        LangId::Ini | LangId::Toml | LangId::Yaml => {
            lex_config(slice, slice_is_line_start, out);
        }
        LangId::Css => {
            lex_css(in_state, slice, out);
        }
        LangId::Html => {
            lex_html(in_state, slice, out);
        }
        LangId::Markdown => {
            lex_markdown(slice, slice_is_line_start, out);
        }
        LangId::Plain => {
            push_tok(out, 0, 0, slice.len(), TokenKind::Plain);
        }
    }
}

fn step_utf8(s: &str, i: usize) -> usize {
    if i >= s.len() {
        return s.len();
    }
    s[i..]
        .chars()
        .next()
        .map(|c| i + c.len_utf8())
        .unwrap_or(s.len())
}

fn is_ident_start(b: u8) -> bool {
    b.is_ascii_alphabetic() || b == b'_'
}

fn is_ident_cont(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_'
}

fn scan_number(b: &[u8], start: usize) -> usize {
    let len = b.len();
    let mut i = start;
    if i + 1 < len && b[i] == b'0' && matches!(b[i + 1], b'x' | b'X' | b'b' | b'B' | b'o' | b'O') {
        i += 2;
        while i < len && (b[i].is_ascii_hexdigit() || b[i] == b'_') {
            i += 1;
        }
        return i;
    }
    while i < len && (b[i].is_ascii_digit() || b[i] == b'_') {
        i += 1;
    }
    if i + 1 < len && b[i] == b'.' && b[i + 1].is_ascii_digit() {
        i += 1;
        while i < len && (b[i].is_ascii_digit() || b[i] == b'_') {
            i += 1;
        }
    }
    i
}

fn lex_code_c_like(
    lang: LangId,
    mut state: u8,
    s: &str,
    is_line_start: bool,
    out: &mut Vec<VisToken>,
) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    if state == 0 && is_line_start && lang == LangId::C {
        let mut ws = 0usize;
        while ws < len && matches!(b[ws], b' ' | b'\t') {
            ws += 1;
        }
        if ws < len && b[ws] == b'#' {
            let mut end = ws + 1;
            while end < len && matches!(b[end], b' ' | b'\t') {
                end += 1;
            }
            let d_start = end;
            while end < len && is_ident_cont(b[end]) {
                end += 1;
            }
            if end > d_start {
                if ws > 0 {
                    push_tok(out, 0, 0, ws, TokenKind::Plain);
                }
                push_tok(out, 0, ws, end, TokenKind::Type);
                i = end;
            }
        }
    }

    while i < len {
        if state == 1 {
            let start = i;
            while i < len {
                if i + 1 < len && b[i] == b'*' && b[i + 1] == b'/' {
                    i += 2;
                    state = 0;
                    break;
                }
                i = step_utf8(s, i);
            }
            push_tok(out, 0, start, i, TokenKind::Comment);
            continue;
        }

        if i + 1 < len && b[i] == b'/' && b[i + 1] == b'/' {
            push_tok(out, 0, i, len, TokenKind::Comment);
            break;
        }
        if i + 1 < len && b[i] == b'/' && b[i + 1] == b'*' {
            state = 1;
            let start = i;
            i += 2;
            while i < len {
                if i + 1 < len && b[i] == b'*' && b[i + 1] == b'/' {
                    i += 2;
                    state = 0;
                    break;
                }
                i = step_utf8(s, i);
            }
            push_tok(out, 0, start, i, TokenKind::Comment);
            continue;
        }

        if b[i] == b'"'
            || (b[i] == b'\'' && lang != LangId::Rust)
            || (b[i] == b'`' && matches!(lang, LangId::Js | LangId::Go))
        {
            let q = b[i];
            let start = i;
            i += 1;
            while i < len && b[i] != q {
                if b[i] == b'\\' && q != b'`' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            if i < len {
                i += 1;
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }

        if lang == LangId::Rust && b[i] == b'\'' {
            if i + 2 < len && b[i + 1] != b'\\' && s.is_char_boundary(i + 1) {
                let next_ch = step_utf8(s, i + 1);
                if next_ch < len && b[next_ch] == b'\'' {
                    push_tok(out, 0, i, next_ch + 1, TokenKind::String);
                    i = next_ch + 1;
                    continue;
                }
            } else if i + 3 < len && b[i + 1] == b'\\' {
                let next_ch = step_utf8(s, i + 2);
                if next_ch < len && b[next_ch] == b'\'' {
                    push_tok(out, 0, i, next_ch + 1, TokenKind::String);
                    i = next_ch + 1;
                    continue;
                }
            }
        }

        if b[i].is_ascii_digit() && (i == 0 || !is_ident_cont(b[i - 1])) {
            let end = scan_number(b, i);
            push_tok(out, 0, i, end, TokenKind::Number);
            i = end;
            continue;
        }

        if is_ident_start(b[i]) || (lang == LangId::Js && b[i] == b'$') {
            let start = i;
            i += 1;
            while i < len && (is_ident_cont(b[i]) || (lang == LangId::Js && b[i] == b'$')) {
                i += 1;
            }
            let word = &s[start..i];
            let kind = classify_word(lang, word);
            push_tok(out, 0, start, i, kind);
            continue;
        }

        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn lex_python(mut state: u8, s: &str, out: &mut Vec<VisToken>) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    while i < len {
        if state == 2 || state == 3 {
            let delim: &[u8; 3] = if state == 2 { b"\"\"\"" } else { b"'''" };
            let start = i;
            while i < len {
                if i + 2 < len && &b[i..i + 3] == delim {
                    i += 3;
                    state = 0;
                    break;
                } else if b[i] == b'\\' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }

        if b[i] == b'#' {
            push_tok(out, 0, i, len, TokenKind::Comment);
            break;
        }

        if i + 2 < len && (&b[i..i + 3] == b"\"\"\"" || &b[i..i + 3] == b"'''") {
            state = if &b[i..i + 3] == b"\"\"\"" { 2 } else { 3 };
            let start = i;
            let delim: &[u8; 3] = if state == 2 { b"\"\"\"" } else { b"'''" };
            i += 3;
            while i < len {
                if i + 2 < len && &b[i..i + 3] == delim {
                    i += 3;
                    state = 0;
                    break;
                } else if b[i] == b'\\' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }

        if b[i] == b'"' || b[i] == b'\'' {
            let q = b[i];
            let start = i;
            i += 1;
            while i < len && b[i] != q {
                if b[i] == b'\\' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            if i < len {
                i += 1;
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }

        if b[i] == b'@' && i + 1 < len && is_ident_start(b[i + 1]) {
            let start = i;
            i += 2;
            while i < len && (is_ident_cont(b[i]) || b[i] == b'.') {
                i += 1;
            }
            push_tok(out, 0, start, i, TokenKind::Type);
            continue;
        }

        if b[i].is_ascii_digit() && (i == 0 || !is_ident_cont(b[i - 1])) {
            let end = scan_number(b, i);
            push_tok(out, 0, i, end, TokenKind::Number);
            i = end;
            continue;
        }

        if is_ident_start(b[i]) {
            let start = i;
            i += 1;
            while i < len && is_ident_cont(b[i]) {
                i += 1;
            }
            let kind = classify_word(LangId::Python, &s[start..i]);
            push_tok(out, 0, start, i, kind);
            continue;
        }

        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn lex_shell_make(s: &str, is_line_start: bool, out: &mut Vec<VisToken>) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    if is_line_start && !b.is_empty() && !matches!(b[0], b' ' | b'\t' | b'#') {
        let mut k = 0usize;
        while k < len && (b[k].is_ascii_alphanumeric() || matches!(b[k], b'_' | b'.' | b'-' | b'/'))
        {
            k += 1;
        }
        if k > 0 {
            let mut ws = k;
            while ws < len && matches!(b[ws], b' ' | b'\t') {
                ws += 1;
            }
            if ws < len
                && (b[ws] == b'='
                    || (ws + 1 < len && matches!(b[ws], b'+' | b':' | b'?') && b[ws + 1] == b'='))
            {
                push_tok(out, 0, 0, k, TokenKind::Key);
                i = k;
            } else if k < len && b[k] == b':' {
                push_tok(out, 0, 0, k, TokenKind::Key);
                i = k;
            }
        }
    }

    while i < len {
        if b[i] == b'#' {
            push_tok(out, 0, i, len, TokenKind::Comment);
            break;
        }
        if b[i] == b'"' || b[i] == b'\'' {
            let q = b[i];
            let start = i;
            i += 1;
            while i < len && b[i] != q {
                if b[i] == b'\\' && q == b'"' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            if i < len {
                i += 1;
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }
        if b[i] == b'$' && i + 1 < len {
            let start = i;
            if b[i + 1] == b'{' {
                let mut end = i + 2;
                while end < len && is_ident_cont(b[end]) {
                    end += 1;
                }
                if end < len && b[end] == b'}' && end > i + 2 {
                    push_tok(out, 0, start, end + 1, TokenKind::Type);
                    i = end + 1;
                    continue;
                }
            } else if is_ident_start(b[i + 1]) {
                let mut end = i + 2;
                while end < len && is_ident_cont(b[end]) {
                    end += 1;
                }
                push_tok(out, 0, start, end, TokenKind::Type);
                i = end;
                continue;
            }
        }
        if b[i].is_ascii_digit() && (i == 0 || !is_ident_cont(b[i - 1])) {
            let end = scan_number(b, i);
            push_tok(out, 0, i, end, TokenKind::Number);
            i = end;
            continue;
        }
        if is_ident_start(b[i]) {
            let start = i;
            i += 1;
            while i < len && is_ident_cont(b[i]) {
                i += 1;
            }
            let kind = classify_word(LangId::Bash, &s[start..i]);
            push_tok(out, 0, start, i, kind);
            continue;
        }
        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn lex_json(s: &str, out: &mut Vec<VisToken>) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    while i < len {
        if b[i] == b'"' {
            let start = i;
            i += 1;
            while i < len && b[i] != b'"' {
                if b[i] == b'\\' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            let quote_end = if i < len {
                i += 1;
                i
            } else {
                len
            };
            let mut ws = quote_end;
            while ws < len && b[ws].is_ascii_whitespace() {
                ws += 1;
            }
            if ws < len && b[ws] == b':' && quote_end >= start + 2 {
                push_tok(out, 0, start, start + 1, TokenKind::Plain);
                push_tok(out, 0, start + 1, quote_end - 1, TokenKind::Key);
                push_tok(out, 0, quote_end - 1, quote_end, TokenKind::Plain);
            } else {
                push_tok(out, 0, start, quote_end, TokenKind::String);
            }
            continue;
        }
        if (b[i] == b'-' && i + 1 < len && b[i + 1].is_ascii_digit())
            || (b[i].is_ascii_digit() && (i == 0 || !is_ident_cont(b[i - 1])))
        {
            let start = i;
            if b[i] == b'-' {
                i += 1;
            }
            i = scan_number(b, i);
            push_tok(out, 0, start, i, TokenKind::Number);
            continue;
        }
        if is_ident_start(b[i]) {
            let start = i;
            i += 1;
            while i < len && is_ident_cont(b[i]) {
                i += 1;
            }
            let kind = match &s[start..i] {
                "true" | "false" | "null" => TokenKind::Keyword,
                _ => TokenKind::Plain,
            };
            push_tok(out, 0, start, i, kind);
            continue;
        }
        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn lex_config(s: &str, is_line_start: bool, out: &mut Vec<VisToken>) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    if is_line_start {
        let mut ws = 0usize;
        while ws < len && matches!(b[ws], b' ' | b'\t') {
            ws += 1;
        }
        if ws < len && b[ws] == b'[' {
            if let Some(rel_close) = s[ws + 1..].find(']') {
                let end = ws + 1 + rel_close + 1;
                if ws > 0 {
                    push_tok(out, 0, 0, ws, TokenKind::Plain);
                }
                push_tok(out, 0, ws, end, TokenKind::Keyword);
                i = end;
            }
        } else if ws < len && (b[ws].is_ascii_alphanumeric() || matches!(b[ws], b'_' | b'.' | b'-'))
        {
            let mut k = ws;
            while k < len && (b[k].is_ascii_alphanumeric() || matches!(b[k], b'_' | b'.' | b'-')) {
                k += 1;
            }
            let mut after = k;
            while after < len && matches!(b[after], b' ' | b'\t') {
                after += 1;
            }
            if after < len && matches!(b[after], b':' | b'=') {
                if ws > 0 {
                    push_tok(out, 0, 0, ws, TokenKind::Plain);
                }
                push_tok(out, 0, ws, k, TokenKind::Key);
                i = k;
            }
        }
    }

    while i < len {
        if matches!(b[i], b'#' | b';') && (i == 0 || b[i - 1].is_ascii_whitespace()) {
            push_tok(out, 0, i, len, TokenKind::Comment);
            break;
        }
        if b[i] == b'"' || b[i] == b'\'' {
            let q = b[i];
            let start = i;
            i += 1;
            while i < len && b[i] != q {
                if b[i] == b'\\' && q == b'"' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            if i < len {
                i += 1;
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }
        if b[i].is_ascii_digit() && (i == 0 || !is_ident_cont(b[i - 1])) {
            let end = scan_number(b, i);
            push_tok(out, 0, i, end, TokenKind::Number);
            i = end;
            continue;
        }
        if is_ident_start(b[i]) {
            let start = i;
            i += 1;
            while i < len && is_ident_cont(b[i]) {
                i += 1;
            }
            let kind = match &s[start..i] {
                "true" | "false" | "yes" | "no" | "null" => TokenKind::Keyword,
                _ => TokenKind::Plain,
            };
            push_tok(out, 0, start, i, kind);
            continue;
        }
        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn lex_css(mut state: u8, s: &str, out: &mut Vec<VisToken>) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    while i < len {
        if state == 1 {
            let start = i;
            while i < len {
                if i + 1 < len && b[i] == b'*' && b[i + 1] == b'/' {
                    i += 2;
                    state = 0;
                    break;
                }
                i = step_utf8(s, i);
            }
            push_tok(out, 0, start, i, TokenKind::Comment);
            continue;
        }
        if i + 1 < len && b[i] == b'/' && b[i + 1] == b'*' {
            state = 1;
            let start = i;
            i += 2;
            while i < len {
                if i + 1 < len && b[i] == b'*' && b[i + 1] == b'/' {
                    i += 2;
                    state = 0;
                    break;
                }
                i = step_utf8(s, i);
            }
            push_tok(out, 0, start, i, TokenKind::Comment);
            continue;
        }
        if b[i] == b'"' || b[i] == b'\'' {
            let q = b[i];
            let start = i;
            i += 1;
            while i < len && b[i] != q {
                if b[i] == b'\\' && i + 1 < len {
                    i = step_utf8(s, i + 1);
                } else {
                    i = step_utf8(s, i);
                }
            }
            if i < len {
                i += 1;
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }
        if b[i] == b'#' {
            let start = i;
            let mut end = i + 1;
            while end < len && b[end].is_ascii_hexdigit() {
                end += 1;
            }
            let hex_len = end - (i + 1);
            if (3..=8).contains(&hex_len) && (end == len || !is_ident_cont(b[end])) {
                push_tok(out, 0, start, end, TokenKind::Number);
                i = end;
                continue;
            }
        }
        if b[i].is_ascii_digit() {
            let start = i;
            i = scan_number(b, i);
            for unit in ["rem", "px", "em", "vh", "vw", "ms", "%", "s"] {
                if s[i..].starts_with(unit) {
                    let after = i + unit.len();
                    if after == len || !b[after].is_ascii_alphabetic() {
                        i = after;
                        break;
                    }
                }
            }
            push_tok(out, 0, start, i, TokenKind::Number);
            continue;
        }
        if b[i].is_ascii_alphabetic() || b[i] == b'-' {
            let start = i;
            i += 1;
            while i < len && (b[i].is_ascii_alphabetic() || b[i] == b'-') {
                i += 1;
            }
            let mut ws = i;
            while ws < len && matches!(b[ws], b' ' | b'\t') {
                ws += 1;
            }
            if ws < len && b[ws] == b':' {
                push_tok(out, 0, start, i, TokenKind::Key);
            } else {
                push_tok(out, 0, start, i, TokenKind::Plain);
            }
            continue;
        }
        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn lex_html(mut state: u8, s: &str, out: &mut Vec<VisToken>) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    while i < len {
        if state == 4 {
            let start = i;
            while i < len {
                if i + 2 < len && &b[i..i + 3] == b"-->" {
                    i += 3;
                    state = 0;
                    break;
                }
                i = step_utf8(s, i);
            }
            push_tok(out, 0, start, i, TokenKind::Comment);
            continue;
        }
        if i + 3 < len && &b[i..i + 4] == b"<!--" {
            state = 4;
            let start = i;
            i += 4;
            while i < len {
                if i + 2 < len && &b[i..i + 3] == b"-->" {
                    i += 3;
                    state = 0;
                    break;
                }
                i = step_utf8(s, i);
            }
            push_tok(out, 0, start, i, TokenKind::Comment);
            continue;
        }
        if b[i] == b'<' {
            let mut tag_start = i + 1;
            if tag_start < len && b[tag_start] == b'/' {
                tag_start += 1;
            }
            let mut tag_end = tag_start;
            while tag_end < len
                && (b[tag_end].is_ascii_alphanumeric() || matches!(b[tag_end], b':' | b'-'))
            {
                tag_end += 1;
            }
            if tag_end > tag_start {
                push_tok(out, 0, i, tag_start, TokenKind::Plain);
                push_tok(out, 0, tag_start, tag_end, TokenKind::Keyword);
                i = tag_end;
                continue;
            }
        }
        if b[i] == b'"' {
            let start = i;
            i += 1;
            while i < len && b[i] != b'"' {
                i = step_utf8(s, i);
            }
            if i < len {
                i += 1;
            }
            push_tok(out, 0, start, i, TokenKind::String);
            continue;
        }
        if (b[i].is_ascii_alphanumeric() || matches!(b[i], b':' | b'-'))
            && i > 0
            && b[i - 1].is_ascii_whitespace()
        {
            let start = i;
            while i < len && (b[i].is_ascii_alphanumeric() || matches!(b[i], b':' | b'-')) {
                i += 1;
            }
            if i < len && b[i] == b'=' {
                push_tok(out, 0, start, i, TokenKind::Key);
            } else {
                push_tok(out, 0, start, i, TokenKind::Plain);
            }
            continue;
        }
        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn lex_markdown(s: &str, is_line_start: bool, out: &mut Vec<VisToken>) {
    let b = s.as_bytes();
    let len = b.len();
    let mut i = 0usize;

    if is_line_start {
        let mut hashes = 0usize;
        while hashes < len && hashes < 6 && b[hashes] == b'#' {
            hashes += 1;
        }
        if hashes > 0 && hashes < len && b[hashes] == b' ' {
            push_tok(out, 0, 0, len, TokenKind::Keyword);
            return;
        }

        let mut ws = 0usize;
        while ws < len && matches!(b[ws], b' ' | b'\t') {
            ws += 1;
        }
        if ws + 1 < len && matches!(b[ws], b'-' | b'*' | b'+') && b[ws + 1] == b' ' {
            if ws > 0 {
                push_tok(out, 0, 0, ws, TokenKind::Plain);
            }
            push_tok(out, 0, ws, ws + 2, TokenKind::Key);
            i = ws + 2;
        } else if ws < len && b[ws].is_ascii_digit() {
            let mut d = ws;
            while d < len && b[d].is_ascii_digit() {
                d += 1;
            }
            if d + 1 < len && b[d] == b'.' && b[d + 1] == b' ' {
                if ws > 0 {
                    push_tok(out, 0, 0, ws, TokenKind::Plain);
                }
                push_tok(out, 0, ws, d + 2, TokenKind::Key);
                i = d + 2;
            }
        }
    }

    while i < len {
        if b[i] == b'`' {
            if let Some(rel_end) = s[i + 1..].find('`') {
                let end = i + 1 + rel_end + 1;
                push_tok(out, 0, i, end, TokenKind::Type);
                i = end;
                continue;
            }
        }
        if b[i] == b'[' {
            if let Some(close_br) = s[i + 1..].find(']') {
                let paren_idx = i + 1 + close_br + 1;
                if paren_idx < len && b[paren_idx] == b'(' {
                    if let Some(close_p) = s[paren_idx + 1..].find(')') {
                        let end = paren_idx + 1 + close_p + 1;
                        push_tok(out, 0, i, end, TokenKind::String);
                        i = end;
                        continue;
                    }
                }
            }
        }
        let next = step_utf8(s, i);
        push_tok(out, 0, i, next, TokenKind::Plain);
        i = next;
    }
}

fn classify_word(lang: LangId, word: &str) -> TokenKind {
    match lang {
        LangId::C => match word {
            "if" | "else" | "for" | "while" | "do" | "switch" | "case" | "default" | "break"
            | "continue" | "return" | "sizeof" | "typedef" | "struct" | "union" | "enum"
            | "static" | "const" | "volatile" | "extern" | "inline" | "class" | "public"
            | "private" | "protected" | "virtual" | "override" | "namespace" | "using"
            | "template" | "typename" | "auto" | "nullptr" | "true" | "false" | "new"
            | "delete" | "explicit" | "this" | "goto" => TokenKind::Keyword,
            "void" | "int" | "char" | "short" | "long" | "float" | "double" | "bool" | "signed"
            | "unsigned" | "size_t" | "uint8_t" | "uint16_t" | "uint32_t" | "uint64_t"
            | "int32_t" | "int64_t" | "QString" | "QObject" | "QVector" => TokenKind::Type,
            _ => TokenKind::Plain,
        },
        LangId::Python => match word {
            "and" | "as" | "assert" | "async" | "await" | "break" | "class" | "continue"
            | "def" | "del" | "elif" | "else" | "except" | "False" | "finally" | "for" | "from"
            | "global" | "if" | "import" | "in" | "is" | "lambda" | "None" | "nonlocal" | "not"
            | "or" | "pass" | "raise" | "return" | "True" | "try" | "while" | "with" | "yield" => {
                TokenKind::Keyword
            }
            "int" | "str" | "float" | "bool" | "list" | "dict" | "set" | "tuple" | "self"
            | "cls" | "print" | "len" | "range" => TokenKind::Type,
            _ => TokenKind::Plain,
        },
        LangId::Bash | LangId::Make => match word {
            "if" | "then" | "else" | "elif" | "fi" | "for" | "while" | "do" | "done" | "case"
            | "esac" | "in" | "function" | "local" | "export" | "return" | "exit" | "sudo"
            | "echo" | "printf" | "cd" | "rm" | "mkdir" | "install" | "make" | "true" | "false" => {
                TokenKind::Keyword
            }
            _ => TokenKind::Plain,
        },
        LangId::Js => match word {
            "import" | "from" | "export" | "default" | "function" | "const" | "let" | "var"
            | "if" | "else" | "for" | "while" | "switch" | "case" | "break" | "continue"
            | "return" | "class" | "new" | "async" | "await" | "try" | "catch" | "throw"
            | "true" | "false" | "null" | "undefined" | "property" | "readonly" | "signal" => {
                TokenKind::Keyword
            }
            _ => {
                if word.as_bytes().first().is_some_and(u8::is_ascii_uppercase) {
                    TokenKind::Type
                } else {
                    TokenKind::Plain
                }
            }
        },
        LangId::Rust => match word {
            "fn" | "let" | "mut" | "const" | "pub" | "mod" | "use" | "struct" | "enum" | "impl"
            | "trait" | "for" | "while" | "loop" | "if" | "else" | "match" | "return" | "self"
            | "Self" | "crate" | "where" | "async" | "await" | "true" | "false" => {
                TokenKind::Keyword
            }
            "i32" | "i64" | "u32" | "u64" | "usize" | "isize" | "f64" | "bool" | "str"
            | "String" | "Vec" | "Option" | "Result" => TokenKind::Type,
            _ => TokenKind::Plain,
        },
        LangId::Go => match word {
            "package" | "import" | "func" | "var" | "const" | "type" | "struct" | "interface"
            | "if" | "else" | "for" | "range" | "switch" | "case" | "return" | "go" | "defer"
            | "select" | "true" | "false" | "nil" => TokenKind::Keyword,
            "int" | "int64" | "uint" | "string" | "bool" | "byte" | "error" | "float64" => {
                TokenKind::Type
            }
            _ => TokenKind::Plain,
        },
        _ => TokenKind::Plain,
    }
}
