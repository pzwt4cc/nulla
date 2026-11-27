#![forbid(unsafe_code)]

#[repr(u8)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TokenKind {
    Plain = 0,
    Keyword = 1,
    Type = 2,
    Number = 3,
    Key = 4,
    String = 5,
    Comment = 6,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct VisToken {
    pub rel_start: u32,
    pub rel_len: u32,
    pub kind: u8,
    pub _pad: [u8; 3],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct VisGuide {
    pub vis_col: u32,
    pub is_active: u8,
    pub _pad: [u8; 3],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct VisLine {
    pub line_idx: usize,
    pub text_offset: u32,
    pub text_len: u32,
    pub start_vis_col: usize,
    pub line_total_vis_cols: usize,
    pub token_offset: u32,
    pub token_count: u32,
    pub guide_offset: u32,
    pub guide_count: u32,
    pub sel_rel_start: u32,
    pub sel_rel_end: u32,
    pub has_selection: u8,
    pub sel_before_slice: u8,
    pub eol_selected: u8,
    pub _pad: u8,
}

#[repr(C)]
pub struct ViewportSnapshot {
    pub utf8_data: *const u8,
    pub utf8_len: usize,
    pub lines: *const VisLine,
    pub line_count: usize,
    pub tokens: *const VisToken,
    pub token_count: usize,
    pub guides: *const VisGuide,
    pub guide_count: usize,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MoveDir {
    Left = 0,
    Right = 1,
    Up = 2,
    Down = 3,
    WordLeft = 4,
    WordRight = 5,
    Home = 6,
    End = 7,
    DocStart = 8,
    DocEnd = 9,
    PageUp = 10,
    PageDown = 11,
}

impl MoveDir {
    pub fn from_i32(v: i32) -> Option<Self> {
        match v {
            0 => Some(Self::Left),
            1 => Some(Self::Right),
            2 => Some(Self::Up),
            3 => Some(Self::Down),
            4 => Some(Self::WordLeft),
            5 => Some(Self::WordRight),
            6 => Some(Self::Home),
            7 => Some(Self::End),
            8 => Some(Self::DocStart),
            9 => Some(Self::DocEnd),
            10 => Some(Self::PageUp),
            11 => Some(Self::PageDown),
            _ => None,
        }
    }
}

pub const TAB_STOP: usize = 4;

#[inline]
pub fn char_vis_width(ch: char, cur_vis_col: usize) -> usize {
    if ch == '\t' {
        TAB_STOP - (cur_vis_col % TAB_STOP)
    } else if ch.is_ascii() {
        if ch.is_ascii_control() { 0 } else { 1 }
    } else if is_wide_char(ch) {
        2
    } else {
        1
    }
}

#[inline]
pub fn is_wide_char(ch: char) -> bool {
    let u = ch as u32;
    (0x1100..=0x115F).contains(&u)
        || (0x2E80..=0x303E).contains(&u)
        || (0x3040..=0xA4CF).contains(&u)
        || (0xAC00..=0xD7A3).contains(&u)
        || (0xF900..=0xFAFF).contains(&u)
        || (0xFE10..=0xFE6F).contains(&u)
        || (0xFF00..=0xFF60).contains(&u)
        || (0x1F300..=0x1FAFF).contains(&u)
        || (0x20000..=0x3FFFF).contains(&u)
}
