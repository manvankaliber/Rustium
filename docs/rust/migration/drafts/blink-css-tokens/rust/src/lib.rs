//! Safe, standalone candidate for Blink's CSS token input and lexical rules.
//!
//! Offsets are UTF-16 code-unit offsets, matching WTF StringView. Numeric
//! conversion and numeric serialization are injected because Blink delegates
//! them to WTF's decimal conversion/formatting providers.
use std::borrow::Cow;
use std::ops::Range;

mod css_tables;

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
    number: Option<f64>,
    number_bits: Option<u64>,
    /// Exact lexical spelling for numeric tokens, supplied to NumberProvider.
    pub numeric_source: Option<Cow<'a, [u16]>>,
    pub numeric_is_integer: bool,
    pub numeric_sign: i8,
    pub hash_is_id: bool,
    pub value_id: u16,
    pub unit_id: Option<u16>,
    pub unicode_range: Option<(u32, u32)>,
    pub block: i8,
}

impl Token<'_> {
    /// Numeric value owned by this token, derived from its authoritative bits.
    pub fn numeric_value(&self) -> Option<f64> {
        self.number_bits.map(f64::from_bits)
    }
    /// Token serialization policy. Numeric spelling itself is delegated only
    /// to the three approved decimal provider operations.
    pub fn serialize(&self, numbers: &impl NumberProvider) -> Option<Vec<u16>> {
        use Kind::*;
        let text = |s: &str| s.encode_utf16().collect::<Vec<_>>();
        let val = self.value.as_deref().unwrap_or(&[]);
        let out = match self.kind {
            Ident => serialize_identifier(val, false),
            Function => {
                let mut x = serialize_identifier(val, false);
                x.push(40);
                x
            }
            AtKeyword => {
                let mut x = vec![64];
                x.extend(serialize_identifier(val, false));
                x
            }
            Hash => {
                let mut x = vec![35];
                x.extend(serialize_identifier(val, !self.hash_is_id));
                x
            }
            Url => {
                let mut x = text("url(");
                x.extend(serialize_identifier(val, false));
                x.push(41);
                x
            }
            Delimiter => {
                let c = self.delimiter?;
                if c == 92 {
                    text("\\\n")
                } else {
                    vec![c]
                }
            }
            Number | Percentage | Dimension => serialize_numeric(self, numbers)?,
            UnicodeRange => {
                let (a, b) = self.unicode_range?;
                text(&format!("U+{a:X}-{b:X}"))
            }
            String => serialize_string(val),
            BadString => text("'\n"),
            BadUrl => text("url(()"),
            Whitespace => vec![32],
            IncludeMatch => text("~="),
            DashMatch => text("|="),
            PrefixMatch => text("^="),
            SuffixMatch => text("$="),
            SubstringMatch => text("*="),
            Column => text("||"),
            Cdo => text("<!--"),
            Cdc => text("-->"),
            Colon => vec![58],
            Semicolon => vec![59],
            Comma => vec![44],
            LParen => vec![40],
            RParen => vec![41],
            LBracket => vec![91],
            RBracket => vec![93],
            LBrace => vec![123],
            RBrace => vec![125],
            Eof | Comment => return None,
        };
        Some(out)
    }

    pub fn semantically_equal(&self, other: &Token<'_>) -> bool {
        if self.kind != other.kind {
            return false;
        }
        use Kind::*;
        match self.kind {
            Delimiter => self.delimiter == other.delimiter,
            Hash => self.hash_is_id == other.hash_is_id && self.value == other.value,
            Ident | Function | String | Url => self.value == other.value,
            Dimension => {
                self.value == other.value
                    && self.numeric_sign == other.numeric_sign
                    && self.number_bits == other.number_bits
                    && self.numeric_is_integer == other.numeric_is_integer
            }
            Number | Percentage => {
                self.numeric_sign == other.numeric_sign
                    && self.number_bits == other.number_bits
                    && self.numeric_is_integer == other.numeric_is_integer
            }
            UnicodeRange => self.unicode_range == other.unicode_range,
            _ => true,
        }
    }
}

fn ascii_u16(bytes: &[u8]) -> Vec<u16> {
    bytes.iter().map(|b| *b as u16).collect()
}
fn serialize_numeric(t: &Token<'_>, p: &impl NumberProvider) -> Option<Vec<u16>> {
    let bits = t.number_bits?;
    let value = f64::from_bits(bits);
    let raw = match t.kind {
        Kind::Number if t.numeric_is_integer => {
            let n = if value.is_nan() {
                0
            } else if value >= i64::MAX as f64 {
                i64::MAX
            } else if value <= i64::MIN as f64 {
                i64::MIN
            } else {
                value as i64
            };
            n.to_string().into_bytes()
        }
        Kind::Percentage => p.format_wtf_six_significant(bits),
        Kind::Number | Kind::Dimension => p.format_ecmascript_shortest(bits),
        _ => return None,
    };
    let mut out = ascii_u16(&raw);
    if t.kind == Kind::Number
        && !t.numeric_is_integer
        && !raw.contains(&b'.')
        && !raw.contains(&b'e')
    {
        out.extend([46, 48]);
    }
    if t.kind == Kind::Percentage {
        out.push(37);
    }
    if t.kind == Kind::Dimension {
        out.extend(serialize_identifier(t.value.as_deref()?, false));
    }
    Some(out)
}
fn serialize_string(v: &[u16]) -> Vec<u16> {
    let mut o = vec![34];
    let mut i = 0;
    while i < v.len() {
        let c = v[i];
        let cp = if (0xd800..=0xdbff).contains(&c)
            && v.get(i + 1).is_some_and(|x| (0xdc00..=0xdfff).contains(x))
        {
            let hi = (c as u32) - 0xd800;
            let lo = (v[i + 1] as u32) - 0xdc00;
            i += 2;
            0x10000 + ((hi << 10) | lo)
        } else {
            i += 1;
            c as u32
        };
        if cp <= 0x1f || cp == 0x7f {
            append_hex_escape(&mut o, cp)
        } else if cp == 34 || cp == 92 {
            o.push(92);
            o.push(cp as u16)
        } else {
            append_scalar(&mut o, cp)
        }
    }
    o.push(34);
    o
}
fn serialize_identifier(v: &[u16], skip_start: bool) -> Vec<u16> {
    let mut o = Vec::new();
    let mut i = 0;
    let mut first = !skip_start;
    let mut second = false;
    let mut first_hyphen = false;
    while i < v.len() {
        let c = v[i];
        let cp = if (0xd800..=0xdbff).contains(&c)
            && v.get(i + 1).is_some_and(|x| (0xdc00..=0xdfff).contains(x))
        {
            let hi = c as u32 - 0xd800;
            let lo = v[i + 1] as u32 - 0xdc00;
            i += 2;
            0x10000 + ((hi << 10) | lo)
        } else {
            i += 1;
            c as u32
        };
        if cp == 0 {
            o.push(0xfffd);
        } else if cp <= 0x1f
            || cp == 0x7f
            || ((48..=57).contains(&cp) && (first || (second && first_hyphen)))
        {
            append_hex_escape(&mut o, cp)
        } else if cp == 45 && first && i == v.len() {
            o.extend([92, 45])
        } else if cp >= 0x80
            || cp == 45
            || cp == 95
            || (48..=57).contains(&cp)
            || (65..=90).contains(&cp)
            || (97..=122).contains(&cp)
        {
            append_scalar(&mut o, cp)
        } else {
            o.push(92);
            append_scalar(&mut o, cp)
        }
        if first {
            first = false;
            second = true;
            first_hyphen = cp == 45
        } else if second {
            second = false
        }
    }
    o
}
fn append_hex_escape(o: &mut Vec<u16>, cp: u32) {
    let s = format!("\\{cp:x} ");
    o.extend(s.bytes().map(|b| b as u16));
}
fn append_scalar(o: &mut Vec<u16>, cp: u32) {
    if let Some(c) = char::from_u32(cp) {
        let mut b = [0u16; 2];
        o.extend_from_slice(c.encode_utf16(&mut b));
    } else {
        o.push(0xfffd)
    }
}

fn append_escaped_codepoint(o: &mut Vec<u16>, cp: u32) {
    if (0xd800..=0xdfff).contains(&cp) {
        o.push(cp as u16);
    } else {
        append_scalar(o, cp);
    }
}

/// Candidate-only Rust provider API; this is not a shared ABI or FFI contract.
pub trait NumberProvider {
    fn parse_ascii_decimal(&self, source: &[u8]) -> DecimalResult;
    fn format_ecmascript_shortest(&self, binary64_bits: u64) -> Vec<u8>;
    fn format_wtf_six_significant(&self, binary64_bits: u64) -> Vec<u8>;
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum DecimalStatus {
    Ok,
    Invalid,
    Overflow,
    Underflow,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct DecimalResult {
    pub binary64_bits: u64,
    pub consumed: usize,
    pub status: DecimalStatus,
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
    fn effective(&self, p: usize) -> u16 {
        let Some(&c) = self.units.get(p) else { return 0 };
        if c == 0 {
            return 0xfffd;
        }
        if (0xd800..=0xdbff).contains(&c)
            && !self.units.get(p + 1).is_some_and(|x| (0xdc00..=0xdfff).contains(x))
        {
            return 0xfffd;
        }
        if (0xdc00..=0xdfff).contains(&c)
            && !(p > 0 && (0xd800..=0xdbff).contains(&self.units[p - 1]))
        {
            return 0xfffd;
        }
        c
    }
    fn source_value(&self, range: Range<usize>) -> Cow<'a, [u16]> {
        match &self.input {
            Input::Utf16(source) => Cow::Borrowed(&source[range]),
            Input::Bytes(_) => Cow::Owned(self.units[range].to_vec()),
        }
    }
    fn eof(&self) -> bool {
        self.pos >= self.units.len()
    }
    fn consume(&mut self) -> u16 {
        if self.eof() {
            return 0;
        }
        self.pos += 1;
        self.effective(self.pos - 1)
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
        a == 92 && !Self::newline(b)
    }
    /// CSS would-start-an-identifier lookahead at an unconsumed source position.
    fn starts_ident_at(&self, pos: usize) -> bool {
        let first = self.effective(pos);
        let second = self.effective(pos + 1);
        Self::name_start(first)
            || Self::valid_escape(first, second)
            || (first == 45
                && (Self::name_start(second)
                    || second == 45
                    || Self::valid_escape(second, self.effective(pos + 2))))
    }
    fn token(&self, kind: Kind, start: usize) -> Token<'a> {
        Token {
            kind,
            span: start..self.pos,
            value: None,
            delimiter: None,
            number: None,
            number_bits: None,
            numeric_source: None,
            numeric_is_integer: true,
            numeric_sign: 0,
            hash_is_id: false,
            value_id: 0,
            unit_id: None,
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
    fn consume_escape(&mut self) -> u32 {
        let c = self.consume();
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
            return n;
        }
        if c == 0 {
            0xfffd
        } else {
            u32::from(c)
        }
    }
    fn consume_name(&mut self, start: usize) -> (Cow<'a, [u16]>, bool) {
        let mut escaped = false;
        while !self.eof() {
            let normalized = self.effective(self.pos);
            if Self::name(normalized) {
                if normalized != self.peek(0) {
                    escaped = true;
                }
                self.pos += 1;
            } else if Self::valid_escape(normalized, self.effective(self.pos + 1)) {
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
                if c == 92 && p + 1 <= self.pos {
                    p += 1;
                    let save = self.pos;
                    self.pos = p;
                    append_escaped_codepoint(&mut out, self.consume_escape());
                    p = self.pos;
                    self.pos = save;
                } else {
                    out.push(self.effective(p));
                    p += 1;
                }
            }
            (Cow::Owned(out), true)
        } else {
            (self.source_value(start..self.pos), false)
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
            if (c == 117 || c == 85)
                && self.unicode_ranges_allowed
                && self.peek(0) == 43
                && (Self::hex(self.peek(1)).is_some() || self.peek(1) == 63)
            {
                self.pos += 1;
                return self.unicode_range(start);
            }
            if self.would_number(c) {
                self.pos = start;
                return self.number(start, numbers);
            }
            // HyphenMinus handles CDC before identifier consumption in Blink.
            if c == 45 && self.peek(0) == 45 && self.peek(1) == 62 {
                self.pos += 2;
                return self.token(Kind::Cdc, start);
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
            if self.starts_ident_at(start) {
                self.pos = start;
                return self.ident_like(start);
            }
            if c == 64 && self.starts_ident_at(self.pos) {
                let s = self.pos;
                let (v, _) = self.consume_name(s);
                return self.text_token(Kind::AtKeyword, start, v);
            }
            if c == 35
                && (Self::name(self.effective(self.pos))
                    || Self::valid_escape(self.effective(self.pos), self.effective(self.pos + 1)))
            {
                let is_id = self.starts_ident_at(self.pos);
                let s = self.pos;
                let (v, _) = self.consume_name(s);
                let mut t = self.text_token(Kind::Hash, start, v);
                t.hash_is_id = is_id;
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
            t.value_id = keyword_id(t.value.as_deref().unwrap_or(&[]));
            t.block = 1;
            self.block_stack.push(Kind::LParen);
            t
        } else {
            let mut t = self.text_token(Kind::Ident, start, v);
            t.value_id = keyword_id(t.value.as_deref().unwrap_or(&[]));
            t
        }
    }
    fn number(&mut self, start: usize, numbers: &impl NumberProvider) -> Token<'a> {
        let (end, integer, sign) = self.numeric_end(start);
        self.pos = end;
        let mut t = self.token(Kind::Number, start);
        t.numeric_is_integer = integer;
        t.numeric_sign = sign;
        let raw = &self.units[start..end];
        let unsigned_start = start + usize::from(sign != 0);
        let value =
            if integer && matches!(&self.input, Input::Bytes(_)) && end - unsigned_start <= 14 {
                let mut n = 0u64;
                for c in &self.units[unsigned_start..end] {
                    n = n * 10 + (*c - 48) as u64;
                }
                if sign < 0 {
                    -(n as f64)
                } else {
                    n as f64
                }
            } else {
                let ascii: Vec<u8> = raw.iter().map(|c| *c as u8).collect();
                let parsed = numbers.parse_ascii_decimal(&ascii);
                if parsed.consumed != ascii.len() || parsed.status == DecimalStatus::Invalid {
                    0.0
                } else {
                    f64::from_bits(parsed.binary64_bits)
                }
            };
        let limit = f32::MAX as f64;
        let value = if value > limit {
            limit
        } else if value < -limit {
            -limit
        } else {
            value
        };
        t.number_bits = Some(value.to_bits());
        t.number = Some(value);
        t.unit_id = Some(1); // CSSPrimitiveValue::UnitType::kNumber
        t.numeric_source = Some(self.source_value(start..end));
        if self.starts_ident_at(self.pos) {
            let s = self.pos;
            let (v, _) = self.consume_name(s);
            t.kind = Kind::Dimension;
            t.unit_id = Some(unit_id(&v));
            t.value = Some(v);
        } else if self.peek(0) == 37 {
            self.pos += 1;
            t.kind = Kind::Percentage;
            t.unit_id = Some(2); // CSSPrimitiveValue::UnitType::kPercentage
        }
        t.span.end = self.pos;
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
            let raw = self.peek(0);
            let c = self.consume();
            if c == q {
                let mut t = self.text_token(
                    Kind::String,
                    start,
                    if escaped {
                        Cow::Owned(out)
                    } else {
                        self.source_value(start + 1..self.pos - 1)
                    },
                );
                t.span = start..self.pos;
                return t;
            }
            if raw != c {
                escaped = true;
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
                    append_escaped_codepoint(&mut out, self.consume_escape())
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
            if c == 34 || c == 39 || c == 40 || is_non_printable(c) {
                self.bad_url();
                return self.token(Kind::BadUrl, start);
            }
            if c == 92 {
                if Self::valid_escape(c, self.peek(0)) {
                    append_escaped_codepoint(&mut out, self.consume_escape())
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

    fn unicode_range(&mut self, start: usize) -> Token<'a> {
        let mut remaining = 6usize;
        let mut first = 0u32;
        while remaining > 0 {
            if let Some(v) = Self::hex(self.peek(0)) {
                first = first * 16 + v;
                self.pos += 1;
                remaining -= 1
            } else {
                break;
            }
        }
        let mut last = first;
        if remaining > 0 && self.peek(0) == 63 {
            while remaining > 0 && self.peek(0) == 63 {
                first *= 16;
                last = last * 16 + 15;
                self.pos += 1;
                remaining -= 1
            }
        } else if self.peek(0) == 45 && Self::hex(self.peek(1)).is_some() {
            self.pos += 1;
            remaining = 6;
            last = 0;
            while remaining > 0 {
                if let Some(v) = Self::hex(self.peek(0)) {
                    last = last * 16 + v;
                    self.pos += 1;
                    remaining -= 1
                } else {
                    break;
                }
            }
        }
        let mut t = self.token(Kind::UnicodeRange, start);
        t.unicode_range = Some((first, last));
        t
    }
}

pub fn needs_inserted_comment(a: &Token<'_>, b: &Token<'_>) -> bool {
    use Kind::*;
    let at = a.kind;
    let bt = b.kind;
    let ad = a.delimiter.unwrap_or(0);
    let bd = b.delimiter.unwrap_or(0);
    if matches!(at, Ident | AtKeyword | Hash | Dimension | Number)
        || (at == Delimiter && (ad == 35 || ad == 45))
    {
        if at == Ident && bt == LParen {
            return true;
        }
        if at == Number && bt == Delimiter {
            if bd == 45 {
                return false;
            }
            if bd == 37 {
                return true;
            }
        }
        return matches!(
            bt,
            Ident | Function | Url | BadUrl | Number | Percentage | Dimension | Cdc
        ) || (bt == Delimiter && bd == 45);
    }
    if at == Delimiter && ad == 64 {
        return matches!(bt, Ident | Function | Url | BadUrl | Cdc)
            || (bt == Delimiter && bd == 45);
    }
    if at == Delimiter && (ad == 46 || ad == 43) {
        return matches!(bt, Number | Percentage | Dimension);
    }
    at == Delimiter && bt == Delimiter && ad == 47 && bd == 42
}

fn ascii_eq_ignore_case(a: &[u16], b: &[u16]) -> bool {
    a.len() == b.len()
        && a.iter().zip(b).all(|(x, y)| {
            let fold = |c: u16| if (65..=90).contains(&c) { c + 32 } else { c };
            fold(*x) == fold(*y)
        })
}
fn is_non_printable(c: u16) -> bool {
    c <= 8 || c == 11 || (14..=31).contains(&c) || c == 127
}

fn keyword_id(v: &[u16]) -> u16 {
    if v.iter().any(|c| *c > 127) {
        return 0;
    }
    let lower: Vec<u8> =
        v.iter().map(|c| if (65..=90).contains(c) { (*c + 32) as u8 } else { *c as u8 }).collect();
    css_tables::CSS_KEYWORDS
        .iter()
        .position(|s| s.as_bytes() == lower.as_slice())
        .map_or(0, |i| (i + 1) as u16)
}
fn unit_id(v: &[u16]) -> u16 {
    if v.iter().any(|c| *c > 127) {
        return 0;
    }
    let lower: Vec<u8> =
        v.iter().map(|c| if (65..=90).contains(c) { (*c + 32) as u8 } else { *c as u8 }).collect();
    css_tables::CSS_UNITS
        .iter()
        .find(|(s, _)| s.as_bytes() == lower.as_slice())
        .map_or(0, |(_, id)| *id)
}

#[cfg(test)]
mod tests {
    use super::*;
    struct FixedNumbers;
    impl NumberProvider for FixedNumbers {
        fn parse_ascii_decimal(&self, s: &[u8]) -> DecimalResult {
            let value: f64 = match s {
                b"2" => 2.0,
                b"2.0" | b"2e0" => 2.0,
                b"1.5" => 1.5,
                b"-0.0" => -0.0,
                b"2e3" => 2000.0,
                b"4e3" => 4000.0,
                b"5" => 5.0,
                b"-1" => -1.0,
                _ => panic!("missing explicit numeric fixture: {:?}", s),
            };
            DecimalResult {
                binary64_bits: value.to_bits(),
                consumed: s.len(),
                status: DecimalStatus::Ok,
            }
        }
        fn format_ecmascript_shortest(&self, b: u64) -> Vec<u8> {
            match f64::from_bits(b) {
                2.0 => b"2".to_vec(),
                1.5 => b"1.5".to_vec(),
                2000.0 => b"2000".to_vec(),
                4000.0 => b"4000".to_vec(),
                _ => panic!("missing explicit numeric formatting fixture: {b}"),
            }
        }
        fn format_wtf_six_significant(&self, b: u64) -> Vec<u8> {
            match f64::from_bits(b) {
                2.0 => b"2".to_vec(),
                1.5 => b"1.5".to_vec(),
                2000.0 => b"2000".to_vec(),
                4000.0 => b"4000".to_vec(),
                _ => panic!("missing explicit WTF6 formatting fixture: {b}"),
            }
        }
    }

    struct StrictCaseNumbers {
        fixtures: std::cell::RefCell<std::collections::VecDeque<(Vec<u8>, u64)>>,
    }
    impl StrictCaseNumbers {
        fn new(fixtures: &[(&[u8], u64)]) -> Self {
            Self {
                fixtures: std::cell::RefCell::new(
                    fixtures.iter().map(|(s, b)| (s.to_vec(), *b)).collect(),
                ),
            }
        }
        fn assert_consumed(&self, label: &str) {
            assert!(self.fixtures.borrow().is_empty(), "{label}: unused numeric fixtures");
        }
    }
    impl NumberProvider for StrictCaseNumbers {
        fn parse_ascii_decimal(&self, source: &[u8]) -> DecimalResult {
            let (expected, bits) = self
                .fixtures
                .borrow_mut()
                .pop_front()
                .expect("unexpected decimal provider call without fixture");
            assert_eq!(source, expected.as_slice(), "unexpected numeric source in case fixture");
            DecimalResult { binary64_bits: bits, consumed: source.len(), status: DecimalStatus::Ok }
        }
        fn format_ecmascript_shortest(&self, _: u64) -> Vec<u8> {
            panic!("formatter was not expected in tokenizer-only case")
        }
        fn format_wtf_six_significant(&self, _: u64) -> Vec<u8> {
            panic!("formatter was not expected in tokenizer-only case")
        }
    }
    fn kinds(s: &[u8], comments: bool) -> Vec<Kind> {
        let mut t = Tokenizer::new(Input::Bytes(s), 0).with_comments(comments);
        let mut v = Vec::new();
        for _ in 0..64 {
            let x = t.next_token(&FixedNumbers);
            v.push(x.kind);
            if x.kind == Kind::Eof {
                break;
            }
        }
        v
    }

    #[test]
    fn original_single_character_tokens() {
        assert_eq!(
            kinds(b"()[],;{}", false),
            vec![
                Kind::LParen,
                Kind::RParen,
                Kind::LBracket,
                Kind::RBracket,
                Kind::Comma,
                Kind::Semicolon,
                Kind::LBrace,
                Kind::RBrace,
                Kind::Eof
            ]
        );
    }
    #[test]
    fn original_multiple_character_tokens() {
        assert_eq!(
            kinds(b"~= |= ^= $= *= || <!-- -->", false),
            vec![
                Kind::IncludeMatch,
                Kind::Whitespace,
                Kind::DashMatch,
                Kind::Whitespace,
                Kind::PrefixMatch,
                Kind::Whitespace,
                Kind::SuffixMatch,
                Kind::Whitespace,
                Kind::SubstringMatch,
                Kind::Whitespace,
                Kind::Column,
                Kind::Whitespace,
                Kind::Cdo,
                Kind::Whitespace,
                Kind::Cdc,
                Kind::Eof
            ]
        );
    }
    #[test]
    fn original_delimiter_token() {
        assert_eq!(
            kinds(b"^*&~|&", false),
            vec![Kind::Delimiter; 6].into_iter().chain([Kind::Eof]).collect::<Vec<_>>()
        );
    }
    #[test]
    fn original_whitespace_tokens() {
        assert_eq!(
            kinds(b" \t\r\n\x0c*", false),
            vec![Kind::Whitespace, Kind::Delimiter, Kind::Eof]
        );
    }
    #[test]
    fn original_escapes() {
        let mut t = Tokenizer::new(Input::Bytes(b"hel\\6Co"), 0);
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.kind, Kind::Ident);
        assert_eq!(x.value.unwrap().as_ref(), &[104, 101, 108, 108, 111]);
    }
    #[test]
    fn original_ident_token() {
        assert_eq!(kinds(b"simple-ident", false), vec![Kind::Ident, Kind::Eof]);
    }
    #[test]
    fn original_function_token() {
        assert_eq!(
            kinds(b"scale(2)", false),
            vec![Kind::Function, Kind::Number, Kind::RParen, Kind::Eof]
        );
    }
    #[test]
    fn original_at_keyword_token() {
        assert_eq!(kinds(b"@media", false), vec![Kind::AtKeyword, Kind::Eof]);
    }
    #[test]
    fn original_url_token() {
        assert_eq!(kinds(b"url(foo)", false), vec![Kind::Url, Kind::Eof]);
    }
    #[test]
    fn original_string_token() {
        assert_eq!(kinds(b"'text'", false), vec![Kind::String, Kind::Eof]);
    }
    #[test]
    fn original_hash_token() {
        assert_eq!(kinds(b"#id", false), vec![Kind::Hash, Kind::Eof]);
    }
    #[test]
    fn original_number_token() {
        assert_eq!(kinds(b"2.0", false), vec![Kind::Number, Kind::Eof]);
    }
    #[test]
    fn original_dimension_token() {
        assert_eq!(kinds(b"2px", false), vec![Kind::Dimension, Kind::Eof]);
    }
    #[test]
    fn original_percentage_token() {
        assert_eq!(kinds(b"2%", false), vec![Kind::Percentage, Kind::Eof]);
    }
    #[test]
    fn original_unicode_range_token() {
        let mut t = Tokenizer::new(Input::Bytes(b"U+2??"), 0);
        t.unicode_ranges_allowed = true;
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.unicode_range, Some((0x200, 0x2ff)));
    }
    #[test]
    fn original_comment_token() {
        assert_eq!(kinds(b"/*x*/a", false), vec![Kind::Ident, Kind::Eof]);
        assert_eq!(kinds(b"/*x*/a", true), vec![Kind::Comment, Kind::Ident, Kind::Eof]);
    }

    #[test]
    fn preserves_valid_and_replaces_lone_surrogates() {
        let mut t = Tokenizer::new(Input::Utf16(&[0xd83d, 0xde00]), 0);
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.value.unwrap().as_ref(), &[0xd83d, 0xde00]);
        let mut t = Tokenizer::new(Input::Utf16(&[0xd800]), 0);
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.value.unwrap().as_ref(), &[0xfffd]);
        let mut t = Tokenizer::new(Input::Utf16(&[97, 0, 98]), 0);
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.value.unwrap().as_ref(), &[97, 0xfffd, 98]);
    }
    #[test]
    fn serializes_numeric_policy_and_dimension_compatibility() {
        let mut t = Tokenizer::new(Input::Bytes(b"2.0"), 0);
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.serialize(&FixedNumbers).unwrap(), vec![50, 46, 48]);
        let mut t = Tokenizer::new(Input::Bytes(b"4e3e2"), 0);
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.kind, Kind::Dimension);
        assert_eq!(x.number_bits, Some(4000.0f64.to_bits()));
        assert_eq!(x.value.as_deref(), Some(&[101, 50][..]));
        assert_eq!(x.serialize(&FixedNumbers).unwrap(), vec![52, 48, 48, 48, 101, 50]);

        let mut t = Tokenizer::new(Input::Bytes(b"4e3%"), 0);
        let percentage = t.next_token(&FixedNumbers);
        assert_eq!(percentage.kind, Kind::Percentage);
        assert_eq!(percentage.serialize(&FixedNumbers).unwrap(), vec![52, 48, 48, 48, 37]);
    }

    #[test]
    fn original_keyword_and_unit_ids() {
        let mut t = Tokenizer::new(Input::Bytes(b"inherit 2PX"), 0);
        let id = t.next_token(&FixedNumbers);
        assert_eq!(id.value_id, 1);
        let _ = t.next_token(&FixedNumbers);
        let d = t.next_token(&FixedNumbers);
        assert_eq!(d.kind, Kind::Dimension);
        assert_eq!(d.unit_id, Some(5));
    }
    #[test]
    fn preserves_storage_unit_offsets() {
        let mut b = Tokenizer::new(Input::Bytes(b"a b"), 0);
        let a = b.next_token(&FixedNumbers);
        let ws = b.next_token(&FixedNumbers);
        let z = b.next_token(&FixedNumbers);
        assert_eq!((a.span, ws.span, z.span), (0..1, 1..2, 2..3));
        let source = [0x61, 0x20, 0xd83d, 0xde00];
        let mut u = Tokenizer::new(Input::Utf16(&source), 0);
        let _ = u.next_token(&FixedNumbers);
        let _ = u.next_token(&FixedNumbers);
        let emoji = u.next_token(&FixedNumbers);
        assert_eq!(emoji.span, 2..4);
    }
    #[test]
    fn malformed_number_boundaries_preserve_unit_suffix() {
        let mut t = Tokenizer::new(Input::Bytes(b"2e+"), 0);
        let x = t.next_token(&FixedNumbers);
        assert_eq!(x.kind, Kind::Dimension);
        assert_eq!(x.value.unwrap().as_ref(), &[101]);
        assert_eq!(t.next_token(&FixedNumbers).delimiter, Some(43));
    }
    #[test]
    fn malformed_url_and_bad_string_reconsume_boundaries() {
        assert_eq!(kinds(b"url(a b)", false), vec![Kind::BadUrl, Kind::Eof]);
        assert_eq!(
            kinds(b"'bad\nnext", false),
            vec![Kind::BadString, Kind::Whitespace, Kind::Ident, Kind::Eof]
        );
    }
    #[test]
    fn comment_insertion_policy() {
        let a = Token {
            kind: Kind::Ident,
            span: 0..1,
            value: Some(Cow::Owned(vec![97])),
            delimiter: None,
            number: None,
            number_bits: None,
            numeric_source: None,
            numeric_is_integer: true,
            numeric_sign: 0,
            hash_is_id: false,
            value_id: 0,
            unit_id: None,
            unicode_range: None,
            block: 0,
        };
        let b = Token {
            kind: Kind::LParen,
            span: 0..1,
            value: None,
            delimiter: None,
            number: None,
            number_bits: None,
            numeric_source: None,
            numeric_is_integer: true,
            numeric_sign: 0,
            hash_is_id: false,
            value_id: 0,
            unit_id: None,
            unicode_range: None,
            block: 0,
        };
        assert!(needs_inserted_comment(&a, &b));
    }

    include!("basic_original_cases.rs");

    fn compare_original_token(
        label: &str,
        index: usize,
        actual: &Token<'_>,
        expected: Option<&BasicExpected>,
    ) {
        let expected_kind = expected.map_or(Kind::Eof, |e| e.kind);
        assert_eq!(actual.kind, expected_kind, "{label} token {index}");
        let Some(e) = expected else { return };
        match e.kind {
            Kind::Delimiter => {
                assert_eq!(actual.delimiter, e.delimiter, "{label} delimiter {index}")
            }
            Kind::Ident | Kind::AtKeyword | Kind::Function | Kind::String | Kind::Url => {
                assert_eq!(actual.value.as_deref(), e.value, "{label} value {index}")
            }
            Kind::Dimension => {
                assert_eq!(actual.value.as_deref(), e.value, "{label} dimension unit {index}");
                assert_eq!(actual.numeric_is_integer, e.integer, "{label} numeric kind {index}");
                assert_eq!(actual.number_bits, e.number_bits, "{label} numeric value {index}");
            }
            Kind::Number => {
                assert_eq!(actual.numeric_sign, e.sign, "{label} numeric sign {index}");
                assert_eq!(actual.numeric_is_integer, e.integer, "{label} numeric kind {index}");
                assert_eq!(actual.number_bits, e.number_bits, "{label} numeric value {index}");
            }
            Kind::Percentage => {
                assert_eq!(actual.numeric_is_integer, e.integer, "{label} numeric kind {index}");
                assert_eq!(actual.number_bits, e.number_bits, "{label} numeric value {index}");
            }
            // BasicExpected covers the first four non-hash/non-range rows.
            // Dedicated NextExpected/NumericExpected records test these kinds.
            Kind::UnicodeRange | Kind::Hash => panic!("{label}: wrong expected-record family"),
            _ => {}
        }
    }

    fn run_original_case(case: &BasicCase, wide: bool) {
        let bytes: Vec<u8> = case.input.iter().map(|u| *u as u8).collect();
        let mut tokenizer = if wide {
            Tokenizer::new(Input::Utf16(case.input), 0)
        } else {
            Tokenizer::new(Input::Bytes(&bytes), 0)
        };
        for index in 0..3 {
            let actual = tokenizer.next_token(&FixedNumbers);
            compare_original_token(case.label, index, &actual, case.expected.get(index));
            if actual.kind == Kind::Eof {
                break;
            }
        }
    }

    #[test]
    fn transcribed_single_character_multiple_character_delimiter_and_whitespace_rows() {
        assert_eq!(BASIC_ORIGINAL_CASES.len(), 37);
        for case in BASIC_ORIGINAL_CASES {
            assert!(
                case.input.iter().all(|u| *u <= 255),
                "{} expected byte-representable fixture",
                case.label
            );
            run_original_case(case, false);
            run_original_case(case, true);
        }
    }

    include!("extended_original_cases.rs");

    fn compare_extended(
        label: &str,
        index: usize,
        actual: &Token<'_>,
        expected: Option<&ExtendedExpected>,
    ) {
        let kind = expected.map_or(Kind::Eof, |e| e.kind);
        assert_eq!(actual.kind, kind, "{label} token {index}");
        let Some(e) = expected else { return };
        match e.kind {
            Kind::Ident | Kind::AtKeyword | Kind::Function | Kind::String | Kind::Url => {
                assert_eq!(actual.value.as_deref(), e.value, "{label} value {index}")
            }
            Kind::Delimiter => {
                assert_eq!(actual.delimiter, e.delimiter, "{label} delimiter {index}")
            }
            Kind::Number => {
                assert_eq!(actual.numeric_sign, e.sign, "{label} sign {index}");
                assert_eq!(actual.numeric_is_integer, e.integer, "{label} integer kind {index}");
                assert_eq!(actual.number_bits, e.number_bits, "{label} value {index}");
            }
            _ => {}
        }
    }
    fn run_extended_case(case: &ExtendedCase, wide: bool) {
        let bytes: Vec<u8> = case.input.iter().map(|u| *u as u8).collect();
        let mut tokenizer = if wide {
            Tokenizer::new(Input::Utf16(case.input), 0)
        } else {
            Tokenizer::new(Input::Bytes(&bytes), 0)
        };
        let numbers = StrictCaseNumbers::new(case.parse_fixtures);
        for i in 0..3 {
            let actual = tokenizer.next_token(&numbers);
            compare_extended(case.label, i, &actual, case.expected.get(i));
            if actual.kind == Kind::Eof {
                break;
            }
        }
        if wide {
            numbers.assert_consumed(case.label)
        }
    }
    #[test]
    fn transcribed_escapes_ident_function_and_at_keyword_rows() {
        assert_eq!(EXTENDED_ORIGINAL_CASES.len(), 105);
        for case in EXTENDED_ORIGINAL_CASES {
            if case.input.iter().all(|u| *u <= 255) {
                run_extended_case(case, false)
            }
            run_extended_case(case, true);
        }
    }
    include!("url_string_hash_comment_cases.rs");

    fn compare_next(
        label: &str,
        index: usize,
        actual: &Token<'_>,
        expected: Option<&NextExpected>,
    ) {
        let kind = expected.map_or(Kind::Eof, |e| e.kind);
        assert_eq!(actual.kind, kind, "{label} token {index}");
        let Some(e) = expected else { return };
        match e.kind {
            Kind::Ident | Kind::String | Kind::Url => {
                assert_eq!(actual.value.as_deref(), e.value, "{label} value {index}");
            }
            Kind::Hash => {
                assert_eq!(actual.value.as_deref(), e.value, "{label} hash value {index}");
                assert_eq!(actual.hash_is_id, e.hash_is_id, "{label} hash classification {index}");
            }
            Kind::Delimiter => {
                assert_eq!(actual.delimiter, e.delimiter, "{label} delimiter {index}")
            }
            _ => {}
        }
    }

    fn run_next_case(case: &NextCase, wide: bool) {
        let bytes: Vec<u8> = case.input.iter().map(|u| *u as u8).collect();
        let mut tokenizer = if wide {
            Tokenizer::new(Input::Utf16(case.input), 0)
        } else {
            Tokenizer::new(Input::Bytes(&bytes), 0)
        };
        // CSSParserTokenStream omits comments from Peek()/ConsumeRaw(). The
        // tokenizer's default mode has the same observable stream semantics.
        let numbers = StrictCaseNumbers::new(&[]);
        for i in 0..3 {
            let actual = tokenizer.next_token(&numbers);
            compare_next(case.label, i, &actual, case.expected.get(i));
            if actual.kind == Kind::Eof {
                break;
            }
        }
        if wide {
            numbers.assert_consumed(case.label);
        }
    }

    #[test]
    fn transcribed_url_string_hash_and_comment_rows() {
        assert_eq!(NEXT_ORIGINAL_CASES.len(), 56);
        for case in NEXT_ORIGINAL_CASES {
            if case.input.iter().all(|u| *u <= 255) {
                run_next_case(case, false);
            }
            run_next_case(case, true);
        }
    }
    struct OriginalDecimalFixtures {
        fixtures: std::cell::RefCell<Vec<(Vec<u8>, u64)>>,
    }
    impl OriginalDecimalFixtures {
        fn new(fixtures: &[(&[u8], u64)]) -> Self {
            Self {
                fixtures: std::cell::RefCell::new(
                    fixtures.iter().map(|(source, bits)| (source.to_vec(), *bits)).collect(),
                ),
            }
        }
        fn assert_consumed(&self, label: &str) {
            assert!(self.fixtures.borrow().is_empty(), "{label}: unused decimal fixtures");
        }
    }
    impl NumberProvider for OriginalDecimalFixtures {
        fn parse_ascii_decimal(&self, source: &[u8]) -> DecimalResult {
            let mut fixtures = self.fixtures.borrow_mut();
            let index = fixtures
                .iter()
                .position(|(expected, _)| expected.as_slice() == source)
                .unwrap_or_else(|| panic!("no frozen expected-value fixture for {source:?}"));
            let (expected, binary64_bits) = fixtures.remove(index);
            assert_eq!(source, expected.as_slice());
            DecimalResult { binary64_bits, consumed: source.len(), status: DecimalStatus::Ok }
        }
        fn format_ecmascript_shortest(&self, _: u64) -> Vec<u8> {
            panic!("numeric tokenizer rows must not invoke formatting")
        }
        fn format_wtf_six_significant(&self, _: u64) -> Vec<u8> {
            panic!("numeric tokenizer rows must not invoke formatting")
        }
    }

    include!("numeric_unicode_cases.rs");

    fn compare_numeric_unicode(
        label: &str,
        index: usize,
        actual: &Token<'_>,
        expected: Option<&NumericExpected>,
    ) {
        let kind = expected.map_or(Kind::Eof, |e| e.kind);
        assert_eq!(actual.kind, kind, "{label} token {index}");
        let Some(e) = expected else { return };
        match e.kind {
            Kind::Number | Kind::Percentage | Kind::Dimension => {
                assert_eq!(actual.number_bits, e.number_bits, "{label} numeric value {index}");
                assert_eq!(actual.numeric_is_integer, e.integer, "{label} numeric type {index}");
                assert_eq!(actual.numeric_sign, e.sign.unwrap(), "{label} numeric sign {index}");
                assert_eq!(actual.unit_id, e.unit_id, "{label} unit id {index}");
                if e.kind == Kind::Dimension {
                    assert_eq!(actual.value.as_deref(), e.value, "{label} dimension unit {index}");
                }
            }
            Kind::UnicodeRange => {
                assert_eq!(actual.unicode_range, e.range, "{label} Unicode bounds {index}");
            }
            Kind::Ident => {
                assert_eq!(actual.value.as_deref(), e.value, "{label} ident value {index}")
            }
            Kind::Delimiter => {
                assert_eq!(actual.delimiter, e.delimiter, "{label} delimiter {index}")
            }
            _ => {}
        }
    }

    fn run_numeric_unicode_case(case: &NumericCase, wide: bool) {
        let bytes: Vec<u8> = case.input.iter().map(|u| *u as u8).collect();
        let mut tokenizer = if wide {
            Tokenizer::new(Input::Utf16(case.input), 0)
        } else {
            Tokenizer::new(Input::Bytes(&bytes), 0)
        };
        tokenizer.unicode_ranges_allowed = case.unicode_ranges_allowed;
        let byte_fixtures: Vec<(&[u8], u64)> = if wide {
            case.parse_fixtures.to_vec()
        } else {
            case.parse_fixtures
                .iter()
                .zip(case.expected.iter().filter(|e| {
                    matches!(e.kind, Kind::Number | Kind::Dimension | Kind::Percentage)
                }))
                .filter_map(|(fixture, expected)| {
                    let digits = fixture.0.iter().filter(|b| b.is_ascii_digit()).count();
                    (!expected.integer || digits > 14).then_some(*fixture)
                })
                .collect()
        };
        let numbers = OriginalDecimalFixtures::new(&byte_fixtures);
        for i in 0..3 {
            let actual = tokenizer.next_token(&numbers);
            compare_numeric_unicode(case.label, i, &actual, case.expected.get(i));
            if actual.kind == Kind::Eof {
                break;
            }
        }
        numbers.assert_consumed(case.label);
    }

    #[test]
    fn transcribed_number_dimension_percentage_and_unicode_range_rows() {
        assert_eq!(NUMERIC_UNICODE_CASES.len(), 65);
        assert_eq!(NUMERIC_UNICODE_CASES.iter().filter(|c| c.unicode_ranges_allowed).count(), 17);
        for case in NUMERIC_UNICODE_CASES {
            assert!(
                case.input.iter().all(|u| *u <= 255),
                "{} byte-representable source",
                case.label
            );
            run_numeric_unicode_case(case, false);
            run_numeric_unicode_case(case, true);
        }
    }

    fn assert_clamp_token(token: Token<'_>) {
        assert_eq!(token.kind, Kind::Number);
        assert!(token.numeric_is_integer);
        assert_eq!(token.numeric_sign, 0);
        assert_eq!(token.number_bits, Some((f32::MAX as f64).to_bits()));
    }

    #[test]
    fn numeric_construction_clamps_to_f32_max_without_provider_fallback() {
        const TWO_POW_128_BITS: u64 = 0x47f0000000000000;
        let input = b"340282366920938463463374607431768211456";
        let byte_provider = OriginalDecimalFixtures::new(&[(input.as_slice(), TWO_POW_128_BITS)]);
        let mut byte_tokenizer = Tokenizer::new(Input::Bytes(input), 0);
        assert_clamp_token(byte_tokenizer.next_token(&byte_provider));
        byte_provider.assert_consumed("byte f32 clamp boundary");

        let units: Vec<u16> = input.iter().map(|b| *b as u16).collect();
        let wide_provider = OriginalDecimalFixtures::new(&[(input.as_slice(), TWO_POW_128_BITS)]);
        let mut wide_tokenizer = Tokenizer::new(Input::Utf16(&units), 0);
        assert_clamp_token(wide_tokenizer.next_token(&wide_provider));
        wide_provider.assert_consumed("wide f32 clamp boundary");
    }

    #[test]
    fn escaped_name_stream_assertions_match_original_expected_values_and_count() {
        let mut input = Vec::new();
        for _ in 0..3000 {
            input.extend_from_slice(b".w-\\[24px\\] ");
        }
        let mut tokenizer = Tokenizer::new(Input::Bytes(&input), 0);
        let numbers = FixedNumbers;
        let expected = [119, 45, 91, 50, 52, 112, 120, 93];
        let mut count = 0;
        loop {
            let token = tokenizer.next_token(&numbers);
            if token.kind == Kind::Ident {
                assert_eq!(token.value.as_deref(), Some(expected.as_slice()));
                count += 1;
            }
            if token.kind == Kind::Eof {
                break;
            }
        }
        assert_eq!(count, 3000);
    }
}
