//! Safe, standalone candidate for Blink's CSS token input and lexical rules.
//!
//! Offsets are UTF-16 code-unit offsets, matching WTF StringView. Numeric
//! conversion and numeric serialization are injected because Blink delegates
//! them to WTF's decimal conversion/formatting providers.
use std::borrow::Cow;
use std::ops::Range;

#[derive(Clone, Debug, PartialEq)]
pub enum Input<'a> {
    Bytes(&'a [u8]),
    Utf16(&'a [u16]),
}

impl<'a> Input<'a> {
    fn units(&self) -> Cow<'a, [u16]> {
        match self {
            Input::Bytes(bytes) => Cow::Owned(bytes.iter().map(|b| *b as u16).collect()),
            Input::Utf16(units) => Cow::Borrowed(units),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Kind {
    Ident,
    Function,
    AtKeyword,
    Hash,
    Url,
    BadUrl,
    Delimiter,
    Number,
    Percentage,
    Dimension,
    IncludeMatch,
    DashMatch,
    PrefixMatch,
    SuffixMatch,
    SubstringMatch,
    Column,
    UnicodeRange,
    Whitespace,
    Cdo,
    Cdc,
    Colon,
    Semicolon,
    Comma,
    LParen,
    RParen,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    String,
    BadString,
    Eof,
    Comment,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Token<'a> {
    pub kind: Kind,
    /// Original-input range in byte units for Bytes and UTF-16 code units for Utf16.
    pub span: Range<usize>,
    /// Decoded string payload. Unescaped input stays borrowed; escapes allocate.
    pub value: Option<Cow<'a, [u16]>>,
    pub delimiter: Option<u16>,
    pub number: Option<f64>,
    /// Exact lexical spelling for numeric tokens, supplied to NumberProvider.
    pub numeric_source: Option<Cow<'a, [u16]>>,
    pub numeric_is_integer: bool,
    pub numeric_sign: i8,
    pub hash_is_id: bool,
    pub unicode_range: Option<(u32, u32)>,
    pub block: i8,
}

pub trait NumberProvider {
    fn parse_blink_decimal(&self, source: &[u16]) -> f64;
    fn format_blink_number(&self, value: f64) -> String;
}

#[derive(Clone, Debug)]
pub struct Tokenizer<'a> {
    input: Input<'a>,
    units: Cow<'a, [u16]>,
    pos: usize,
    block_stack: Vec<Kind>,
    token_count: usize,
    emit_comments: bool,
    pub unicode_ranges_allowed: bool,
}

impl<'a> Tokenizer<'a> {
    pub fn new(input: Input<'a>, offset: usize) -> Self {
        let units = input.units();
        let pos = offset.min(units.len());
        Self {
            input,
            units,
            pos,
            block_stack: Vec::new(),
            token_count: 0,
            emit_comments: false,
            unicode_ranges_allowed: false,
        }
    }
    pub fn with_comments(mut self, yes: bool) -> Self {
        self.emit_comments = yes;
        self
    }
    pub fn token_count(&self) -> usize {
        self.token_count
    }
    pub fn source(&self) -> &Input<'a> {
        &self.input
    }
    fn peek(&self, n: usize) -> u16 {
        self.units.get(self.pos + n).copied().unwrap_or(0)
    }
    fn raw(&self, n: usize) -> u16 {
        self.peek(n)
    }
    fn eof(&self) -> bool {
        self.pos >= self.units.len()
    }
    fn consume(&mut self) -> u16 {
        if self.eof() {
            return 0;
        }
        let c = self.units[self.pos];
        self.pos += 1;
        if c == 0 || (0xd800..=0xdfff).contains(&c) {
            0xfffd
        } else {
            c
        }
    }
    fn html_space(c: u16) -> bool {
        matches!(c, 9 | 10 | 12 | 13 | 32)
    }
    fn newline(c: u16) -> bool {
        matches!(c, 10 | 12 | 13)
    }
    fn digit(c: u16) -> bool {
        (48..=57).contains(&c)
    }
    fn hex(c: u16) -> Option<u32> {
        match c {
            48..=57 => Some((c - 48) as u32),
            97..=102 => Some((c - 87) as u32),
            65..=70 => Some((c - 55) as u32),
            _ => None,
        }
    }
    fn name(c: u16) -> bool {
        c >= 0x80
            || c == 45
            || c == 95
            || Self::digit(c)
            || (65..=90).contains(&c)
            || (97..=122).contains(&c)
    }
    fn name_start(c: u16) -> bool {
        c >= 0x80 || c == 95 || (65..=90).contains(&c) || (97..=122).contains(&c)
    }
    fn valid_escape(a: u16, b: u16) -> bool {
        a == 92 && !Self::newline(b) && b != 0
    }
    fn starts_ident(&self, first: u16) -> bool {
        let second = self.peek(0);
        Self::name_start(first)
            || Self::valid_escape(first, second)
            || (first == 45
                && (Self::name_start(second)
                    || second == 45
                    || Self::valid_escape(second, self.peek(1))))
    }
    fn token(&self, kind: Kind, start: usize) -> Token<'a> {
        Token {
            kind,
            span: start..self.pos,
            value: None,
            delimiter: None,
            number: None,
            numeric_source: None,
            numeric_is_integer: true,
            numeric_sign: 0,
            hash_is_id: false,
            unicode_range: None,
            block: 0,
        }
    }
    fn text_token(&self, kind: Kind, start: usize, value: Cow<'a, [u16]>) -> Token<'a> {
        let mut t = self.token(kind, start);
        t.value = Some(value);
        t
    }
    fn consume_ws(&mut self) {
        while Self::html_space(self.peek(0)) && !self.eof() {
            self.pos += 1;
        }
    }
    fn consume_escape(&mut self) -> u16 {
        let mut c = self.consume();
        if let Some(mut n) = Self::hex(c) {
            for _ in 1..6 {
                if let Some(v) = Self::hex(self.peek(0)) {
                    self.pos += 1;
                    n = (n << 4) | v;
                } else {
                    break;
                }
            }
            if self.peek(0) == 13 && self.peek(1) == 10 {
                self.pos += 2;
            } else if Self::html_space(self.peek(0)) {
                self.pos += 1;
            }
            if n == 0 || n > 0x10ffff || (0xd800..=0xdfff).contains(&n) {
                return 0xfffd;
            }
            return n as u16;
        }
        if c == 0 {
            0xfffd
        } else {
            c
        }
    }
    fn consume_name(&mut self, start: usize) -> (Cow<'a, [u16]>, bool) {
        let mut escaped = false;
        while !self.eof() {
            if Self::name(self.peek(0)) {
                self.pos += 1;
            } else if Self::valid_escape(self.peek(0), self.peek(1)) {
                escaped = true;
                self.pos += 1;
                self.consume_escape();
            } else {
                break;
            }
        }
        if escaped {
            let mut out = Vec::new();
            let mut p = start;
            while p < self.pos {
                let c = self.units[p];
                if c == 92 && p + 1 < self.pos {
                    p += 1;
                    let save = self.pos;
                    self.pos = p;
                    out.push(self.consume_escape());
                    p = self.pos;
                    self.pos = save;
                } else {
                    out.push(if c == 0 || (0xd800..=0xdfff).contains(&c) { 0xfffd } else { c });
                    p += 1;
                }
            }
            (Cow::Owned(out), true)
        } else {
            (Cow::Borrowed(&self.units[start..self.pos]), false)
        }
    }
    fn would_number(&self, first: u16) -> bool {
        if first == 43 || first == 45 {
            Self::digit(self.peek(0)) || (self.peek(0) == 46 && Self::digit(self.peek(1)))
        } else if first == 46 {
            Self::digit(self.peek(0))
        } else {
            Self::digit(first)
        }
    }
    fn numeric_end(&self, start: usize) -> (usize, bool, i8) {
        let mut p = start;
        let sign = if self.units.get(p) == Some(&43) {
            p += 1;
            1
        } else if self.units.get(p) == Some(&45) {
            p += 1;
            -1
        } else {
            0
        };
        while self.units.get(p).is_some_and(|c| Self::digit(*c)) {
            p += 1;
        }
        let mut integer = true;
        if self.units.get(p) == Some(&46) && self.units.get(p + 1).is_some_and(|c| Self::digit(*c))
        {
            integer = false;
            p += 2;
            while self.units.get(p).is_some_and(|c| Self::digit(*c)) {
                p += 1;
            }
        }
        if matches!(self.units.get(p), Some(69 | 101)) {
            let q = if matches!(self.units.get(p + 1), Some(43 | 45)) { p + 2 } else { p + 1 };
            if self.units.get(q).is_some_and(|c| Self::digit(*c)) {
                integer = false;
                p = q + 1;
                while self.units.get(p).is_some_and(|c| Self::digit(*c)) {
                    p += 1;
                }
            }
        }
        (p, integer, sign)
    }
    pub fn next_token(&mut self, numbers: &impl NumberProvider) -> Token<'a> {
        loop {
            let start = self.pos;
            self.token_count += 1;
            if self.eof() {
                return self.token(Kind::Eof, start);
            }
            let c = self.consume();
            if c == 0 {
                return self.token(Kind::Eof, start);
            }
            if Self::html_space(c) {
                self.consume_ws();
                return self.token(Kind::Whitespace, start);
            }
            if c == 34 || c == 39 {
                return self.string(start, c);
            }
            if self.would_number(c) {
                self.pos = start;
                return self.number(start, numbers);
            }
            let one = match c {
                40 => Some(Kind::LParen),
                41 => Some(Kind::RParen),
                91 => Some(Kind::LBracket),
                93 => Some(Kind::RBracket),
                123 => Some(Kind::LBrace),
                125 => Some(Kind::RBrace),
                58 => Some(Kind::Colon),
                59 => Some(Kind::Semicolon),
                44 => Some(Kind::Comma),
                _ => None,
            };
            if let Some(k) = one {
                let mut t = self.token(k, start);
                if matches!(k, Kind::LParen | Kind::LBracket | Kind::LBrace) {
                    self.block_stack.push(k);
                    t.block = 1;
                } else if let Some(open) = self.block_stack.last().copied() {
                    let expected = match k {
                        Kind::RParen => Kind::LParen,
                        Kind::RBracket => Kind::LBracket,
                        _ => Kind::LBrace,
                    };
                    if open == expected {
                        self.block_stack.pop();
                        t.block = -1;
                    }
                }
                return t;
            }
            if c == 47 && self.peek(0) == 42 {
                self.pos += 1;
                while !self.eof() {
                    if self.peek(0) == 42 && self.peek(1) == 47 {
                        self.pos += 2;
                        break;
                    }
                    self.pos += 1;
                }
                if self.emit_comments {
                    return self.token(Kind::Comment, start);
                }
                continue;
            }
            if c == 92 && Self::valid_escape(c, self.peek(0)) {
                self.pos = start;
                return self.ident_like(start);
            }
            if self.starts_ident(c) {
                self.pos = start;
                return self.ident_like(start);
            }
            if c == 64 && self.starts_ident(self.peek(0)) {
                let s = self.pos;
                let (v, _) = self.consume_name(s);
                return self.text_token(Kind::AtKeyword, start, v);
            }
            if c == 35
                && (Self::name(self.peek(0)) || Self::valid_escape(self.peek(0), self.peek(1)))
            {
                let s = self.pos;
                let (v, _) = self.consume_name(s);
                let mut t = self.text_token(Kind::Hash, start, v);
                t.hash_is_id = self.starts_ident(self.peek(0));
                return t;
            }
            if c == 42 && self.peek(0) == 61 {
                self.pos += 1;
                return self.token(Kind::SubstringMatch, start);
            }
            if c == 94 && self.peek(0) == 61 {
                self.pos += 1;
                return self.token(Kind::PrefixMatch, start);
            }
            if c == 36 && self.peek(0) == 61 {
                self.pos += 1;
                return self.token(Kind::SuffixMatch, start);
            }
            if c == 126 && self.peek(0) == 61 {
                self.pos += 1;
                return self.token(Kind::IncludeMatch, start);
            }
            if c == 124 && self.peek(0) == 61 {
                self.pos += 1;
                return self.token(Kind::DashMatch, start);
            }
            if c == 124 && self.peek(0) == 124 {
                self.pos += 1;
                return self.token(Kind::Column, start);
            }
            if c == 60 && self.peek(0) == 33 && self.peek(1) == 45 && self.peek(2) == 45 {
                self.pos += 3;
                return self.token(Kind::Cdo, start);
            }
            if c == 45 && self.peek(0) == 45 && self.peek(1) == 62 {
                self.pos += 2;
                return self.token(Kind::Cdc, start);
            }
            let mut t = self.token(Kind::Delimiter, start);
            t.delimiter = Some(c);
            return t;
        }
    }
    fn ident_like(&mut self, start: usize) -> Token<'a> {
        let s = self.pos;
        let (v, _) = self.consume_name(s);
        if self.peek(0) == 40 {
            self.pos += 1;
            if ascii_eq_ignore_case(&v, &[117, 114, 108]) {
                self.consume_ws();
                if self.peek(0) != 34 && self.peek(0) != 39 {
                    return self.url(start);
                }
            }
            let mut t = self.text_token(Kind::Function, start, v);
            t.block = 1;
            self.block_stack.push(Kind::LParen);
            t
        } else {
            self.text_token(Kind::Ident, start, v)
        }
    }
    fn number(&mut self, start: usize, numbers: &impl NumberProvider) -> Token<'a> {
        let (end, integer, sign) = self.numeric_end(start);
        self.pos = end;
        let mut t = self.token(Kind::Number, start);
        t.numeric_is_integer = integer;
        t.numeric_sign = sign;
        let raw = &self.units[start..end];
        t.number = Some(numbers.parse_blink_decimal(raw));
        t.numeric_source = Some(Cow::Borrowed(raw));
        if self.starts_ident(self.peek(0)) {
            let s = self.pos;
            let (v, _) = self.consume_name(s);
            t.kind = Kind::Dimension;
            t.value = Some(v);
        } else if self.peek(0) == 37 {
            self.pos += 1;
            t.kind = Kind::Percentage;
        }
        t
    }
    fn string(&mut self, start: usize, q: u16) -> Token<'a> {
        let mut out = Vec::new();
        let mut escaped = false;
        loop {
            if self.eof() {
                let mut t = self.text_token(Kind::String, start, Cow::Owned(out));
                t.span = start..self.pos;
                return t;
            }
            let c = self.consume();
            if c == q {
                let mut t = self.text_token(
                    Kind::String,
                    start,
                    if escaped {
                        Cow::Owned(out)
                    } else {
                        Cow::Borrowed(&self.units[start + 1..self.pos - 1])
                    },
                );
                t.span = start..self.pos;
                return t;
            }
            if Self::newline(c) {
                self.pos -= 1;
                return self.token(Kind::BadString, start);
            }
            if c == 92 {
                escaped = true;
                if self.eof() {
                    continue;
                }
                if Self::newline(self.peek(0)) {
                    if self.peek(0) == 13 && self.peek(1) == 10 {
                        self.pos += 2
                    } else {
                        self.pos += 1
                    }
                } else {
                    out.push(self.consume_escape())
                }
            } else {
                out.push(c)
            }
        }
    }
    fn url(&mut self, start: usize) -> Token<'a> {
        let mut out = Vec::new();
        loop {
            if self.eof() || self.peek(0) == 41 {
                if !self.eof() {
                    self.pos += 1
                }
                return self.text_token(Kind::Url, start, Cow::Owned(out));
            }
            let c = self.consume();
            if Self::html_space(c) {
                self.consume_ws();
                if self.peek(0) == 41 {
                    self.pos += 1;
                    return self.text_token(Kind::Url, start, Cow::Owned(out));
                }
                if self.eof() {
                    return self.text_token(Kind::Url, start, Cow::Owned(out));
                }
                self.bad_url();
                return self.token(Kind::BadUrl, start);
            }
            if c == 34 || c == 39 || c == 40 || c == 127 {
                self.bad_url();
                return self.token(Kind::BadUrl, start);
            }
            if c == 92 {
                if Self::valid_escape(c, self.peek(0)) {
                    out.push(self.consume_escape())
                } else {
                    self.bad_url();
                    return self.token(Kind::BadUrl, start);
                }
            } else {
                out.push(c)
            }
        }
    }
    fn bad_url(&mut self) {
        while !self.eof() {
            let c = self.consume();
            if c == 41 {
                return;
            }
            if Self::valid_escape(c, self.peek(0)) {
                self.consume_escape();
            }
        }
    }
}

fn ascii_eq_ignore_case(a: &[u16], b: &[u16]) -> bool {
    a.len() == b.len()
        && a.iter().zip(b).all(|(x, y)| {
            let fold = |c: u16| if (65..=90).contains(&c) { c + 32 } else { c };
            fold(*x) == fold(*y)
        })
}
