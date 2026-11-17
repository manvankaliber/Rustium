//! Direct UTF-16 port of Blink's local line-break policy. ICU is a primitive,
//! lazy boundary/property provider; it does not supply a pre-normalized list.
//! Native ABI, loaded ICU data and physical iterator pools remain director debt.
use std::{cell::RefCell, rc::Rc};

mod break_all_data;
mod fast_pair_data;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BreakSpace {
    AfterRun,
    Every,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BreakType {
    Normal,
    BreakAll,
    BreakCharacter,
    KeepAll,
    Phrase,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Strictness {
    Normal = 0,
    Strict = 1,
    Loose = 2,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Category {
    Letter,
    Number,
    Mark,
    Other,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LineProperty {
    Numeric,
    Alphabetic,
    Complex,
    Ideographic,
    Combining,
    BreakAfter,
    Other,
}
/// ICU 78 ULineBreak values, copied in declaration order from the frozen
/// configured ICU uchar.h. Values outside 0..49 are rejected by the provider.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum IcuLineBreak {
    XX = 0,
    AI = 1,
    AL = 2,
    B2 = 3,
    BA = 4,
    BB = 5,
    BK = 6,
    CB = 7,
    CL = 8,
    CM = 9,
    CR = 10,
    EX = 11,
    GL = 12,
    HY = 13,
    ID = 14,
    IN = 15,
    IS = 16,
    LF = 17,
    NS = 18,
    NU = 19,
    OP = 20,
    PO = 21,
    PR = 22,
    QU = 23,
    SA = 24,
    SG = 25,
    SP = 26,
    SY = 27,
    ZW = 28,
    NL = 29,
    WJ = 30,
    H2 = 31,
    H3 = 32,
    JL = 33,
    JT = 34,
    JV = 35,
    CP = 36,
    CJ = 37,
    HL = 38,
    RI = 39,
    EB = 40,
    EM = 41,
    ZWJ = 42,
    AK = 43,
    AP = 44,
    AS = 45,
    VF = 46,
    VI = 47,
    HH = 48,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Following {
    Boundary(i32),
    Done,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum BoundaryError {
    Provider,
    InvalidRange,
    InvalidOffset,
    TooLong,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Fast {
    No,
    Yes,
    Unknown,
}

/// A session must retain stable backing text until `release_line`; operations
/// are thread-confined. Implementations validate ICU int32 indices. Values are
/// primitive so Rust retains all tailoring and boundary policy.
pub trait BoundaryProvider {
    fn acquire_line(&mut self, utf16: &[u16], locale: &str) -> Result<(), BoundaryError>;
    fn following(&mut self, query: i32) -> Result<Following, BoundaryError>;
    fn release_line(&mut self);
    fn acquire_character(&mut self, utf16: &[u16]) -> Result<(), BoundaryError>;
    fn character_following(&mut self, query: i32) -> Result<Following, BoundaryError>;
    fn release_character(&mut self);
    fn category(&mut self, codepoint: u32) -> Category;
    /// Exact u_getIntPropertyValue(..., UCHAR_LINE_BREAK), ICU-78 numeric ID.
    fn line_property(&mut self, codepoint: u32) -> Result<u8, BoundaryError>;
}

#[derive(Clone, Copy)]
struct Ctx {
    current: u16,
    last: u16,
    previous: u16,
    last_space: bool,
    current_space: bool,
}
impl Ctx {
    fn new(s: &[u16], start: usize, i: usize) -> Self {
        let last = if i > start { s[i - 1] } else { 0 };
        Self {
            current: 0,
            last,
            previous: if i > start + 1 { s[i - 2] } else { 0 },
            last_space: space(last),
            current_space: false,
        }
    }
    fn fetch(&mut self, s: &[u16], i: usize) -> bool {
        if i >= s.len() {
            return false;
        }
        self.current = s[i];
        self.current_space = space(self.current);
        true
    }
    fn advance(&mut self, i: &mut usize) {
        *i += 1;
        self.previous = self.last;
        self.last = self.current;
        self.last_space = self.current_space;
    }
    fn fast<P: BoundaryProvider>(&self, p: &mut P, disable_shy: bool, quote: bool) -> Fast {
        let (a, b) = (self.last, self.current);
        if a < 0x21 || b < 0x21 {
            return Fast::No;
        }
        if a == b'-' {
            if b <= 0x7f && b.is_ascii_digit() {
                return if self.previous.is_ascii_alphanumeric() { Fast::Yes } else { Fast::No };
            }
            if b > 0x7f {
                return Fast::Unknown;
            }
        }
        if a <= 0xff && b <= 0xff {
            let pair = FAST_PAIR_BITS[(a - 0x21) as usize][((b - 0x21) / 8) as usize]
                & (1 << ((b - 0x21) % 8))
                != 0;
            if !pair || (disable_shy && a == 0xad) {
                return Fast::No;
            }
            return Fast::Yes;
        }
        if quote {
            if b == 0x2019 && a <= 0xff && a != 0x85 {
                return Fast::No;
            }
            if a == 0x2019 && b <= 0xff {
                return Fast::No;
            }
        }
        Fast::Unknown
    }
}
fn space(c: u16) -> bool {
    matches!(c, 0x20 | 0x09 | 0x0a)
}
fn lead(c: u16) -> bool {
    (0xd800..=0xdbff).contains(&c)
}
fn trail(c: u16) -> bool {
    (0xdc00..=0xdfff).contains(&c)
}
fn cp(s: &[u16], i: usize) -> u32 {
    let u = s[i] as u32;
    if trail(s[i]) && i > 0 && lead(s[i - 1]) {
        0x10000 + (((s[i - 1] as u32 - 0xd800) << 10) | (u - 0xdc00))
    } else {
        u
    }
}
fn keep<P: BoundaryProvider>(
    p: &mut P,
    prev: u16,
    last: u16,
    next: u16,
) -> Result<bool, BoundaryError> {
    let c = if p.category(last as u32) == Category::Mark { prev } else { last };
    Ok(matches!(p.category(c as u32), Category::Letter | Category::Number)
        && p.line_property(c as u32)? != IcuLineBreak::SA as u8
        && matches!(p.category(next as u32), Category::Letter | Category::Number)
        && p.line_property(next as u32)? != IcuLineBreak::SA as u8)
}
fn after_all(last: u8, cur: u8, ch: u16, strict: Strictness) -> bool {
    if last >= 49 || cur >= 49 {
        return false;
    }
    if break_all_data::BREAK_ALL_TABLE[last as usize][(cur / 8) as usize] & (0x80 >> (cur % 8)) == 0
    {
        return false;
    }
    cur != IcuLineBreak::BA as u8 || ch == 0x7c || strict == Strictness::Loose
}

/// Owns settings and borrowed stable input; Rc/RefCell enforces !Send/!Sync.
pub struct LocalLinePolicy<'a, P: BoundaryProvider> {
    text: &'a [u16],
    start: usize,
    locale: String,
    kind: BreakType,
    space: BreakSpace,
    strict: Strictness,
    soft_hyphen: bool,
    quote: bool,
    session: bool,
    provider: Rc<RefCell<P>>,
}
impl<'a, P: BoundaryProvider> LocalLinePolicy<'a, P> {
    pub fn new(text: &'a [u16], provider: Rc<RefCell<P>>) -> Result<Self, BoundaryError> {
        if text.len() > i32::MAX as usize {
            return Err(BoundaryError::TooLong);
        }
        Ok(Self {
            text,
            start: 0,
            locale: String::new(),
            kind: BreakType::Normal,
            space: BreakSpace::AfterRun,
            strict: Strictness::Normal,
            soft_hyphen: true,
            quote: false,
            session: false,
            provider,
        })
    }
    fn invalidate(&mut self) {
        if self.session {
            self.provider.borrow_mut().release_line();
            self.session = false
        }
    }
    pub fn set_start(&mut self, n: usize) -> Result<(), BoundaryError> {
        if n > self.text.len() {
            return Err(BoundaryError::InvalidRange);
        }
        if n != self.start {
            self.invalidate();
            self.start = n
        }
        Ok(())
    }
    pub fn set_locale(&mut self, s: impl Into<String>) {
        let s = s.into();
        if s != self.locale {
            self.invalidate();
            self.locale = s
        }
    }
    pub fn set_type(&mut self, v: BreakType) {
        if v != self.kind {
            self.invalidate();
            self.kind = v
        }
    }
    pub fn set_space(&mut self, v: BreakSpace) {
        self.space = v
    }
    pub fn set_strictness(&mut self, v: Strictness) {
        self.strict = v;
        self.invalidate()
    }
    pub fn set_soft_hyphen(&mut self, v: bool) {
        self.soft_hyphen = v
    }
    pub fn set_right_quote(&mut self, v: bool) {
        self.quote = v
    }
    fn line_following(&mut self, q: usize) -> Result<usize, BoundaryError> {
        if !self.session {
            self.provider.borrow_mut().acquire_line(&self.text[self.start..], &self.locale)?;
            self.session = true
        }
        let q = i32::try_from(q).map_err(|_| BoundaryError::TooLong)?;
        match self.provider.borrow_mut().following(q)? {
            Following::Done => Ok(self.text.len()),
            Following::Boundary(n)
                if n > q && n >= 0 && (n as usize) <= self.text.len() - self.start =>
            {
                self.start.checked_add(n as usize).ok_or(BoundaryError::InvalidOffset)
            }
            _ => Err(BoundaryError::InvalidOffset),
        }
    }
    pub fn next(&mut self, pos: usize, limit: usize) -> Result<usize, BoundaryError> {
        if pos < self.start || pos > limit || limit > self.text.len() {
            return Err(BoundaryError::InvalidRange);
        }
        if pos == limit {
            return Ok(limit);
        }
        if self.kind == BreakType::BreakCharacter {
            self.provider.borrow_mut().acquire_character(self.text)?;
            let q = if pos > 0 { pos - 1 } else { 0 };
            let out = match self
                .provider
                .borrow_mut()
                .character_following(i32::try_from(q).map_err(|_| BoundaryError::TooLong)?)?
            {
                Following::Boundary(n)
                    if n > q as i32 && n >= 0 && (n as usize) <= self.text.len() =>
                {
                    n as usize
                }
                Following::Done => self.text.len(),
                _ => return Err(BoundaryError::InvalidOffset),
            };
            self.provider.borrow_mut().release_character();
            return Ok(out);
        }
        let mut i = pos;
        let mut c = Ctx::new(self.text, self.start, pos);
        let mut cached = 0usize;
        let mut last_prop = if self.kind == BreakType::BreakAll {
            let mut p = self.provider.borrow_mut();
            match p.line_property(if c.last == 0 {
                0
            } else {
                cp(self.text, pos.saturating_sub(1))
            }) {
                Ok(v) => v,
                Err(_) => return Err(BoundaryError::Provider),
            }
        } else {
            IcuLineBreak::XX as u8
        };
        while i < limit && c.fetch(self.text, i) {
            match self.space {
                BreakSpace::AfterRun => {
                    if c.current_space {
                        c.advance(&mut i);
                        continue;
                    }
                    if c.last_space {
                        return Ok(i);
                    }
                }
                BreakSpace::Every => {
                    if c.last_space || c.last == 0x3000 {
                        return Ok(i);
                    }
                    if (c.current_space || c.current == 0x3000) && i + 1 < limit {
                        return Ok(i + 1);
                    }
                }
            }
            let fast = c.fast(&mut *self.provider.borrow_mut(), !self.soft_hyphen, self.quote);
            if fast == Fast::Yes {
                return Ok(i);
            }
            if self.kind == BreakType::BreakAll && !lead(c.current) {
                let mut p = self.provider.borrow_mut();
                let prop = match p.line_property(if c.current == b'+' as u16 {
                    b'+' as u32
                } else {
                    cp(self.text, i)
                }) {
                    Ok(v) => v,
                    Err(_) => return Err(BoundaryError::Provider),
                };
                if self.strict == Strictness::Loose
                    && matches!(c.current, 0x2010 | 0x2013)
                    && matches!(last_prop, 19 | 2 | 24 | 14)
                {
                    return Ok(i);
                }
                if after_all(last_prop, prop, c.current, self.strict) {
                    return Ok(if i > pos && trail(c.current) { i - 1 } else { i });
                }
                if prop != IcuLineBreak::CM as u8 {
                    last_prop = prop
                }
            }
            if self.kind == BreakType::KeepAll
                && keep(&mut *self.provider.borrow_mut(), c.previous, c.last, c.current)?
            {
                c.advance(&mut i);
                continue;
            }
            if fast == Fast::Unknown {
                if i > self.start && (cached < i || cached == 0) {
                    let mut q = i - 1;
                    loop {
                        cached = self.line_following(q - self.start)?;
                        if cached == self.text.len() {
                            break;
                        }
                        if !self.soft_hyphen && cached > 0 && self.text[cached - 1] == 0xad {
                            q = cached;
                            continue;
                        }
                        break;
                    }
                }
                if i == cached && !c.last_space {
                    return Ok(i);
                }
            }
            c.advance(&mut i)
        }
        Ok(limit)
    }
    pub fn is_breakable(&mut self, at: usize) -> Result<bool, BoundaryError> {
        if at > self.text.len() {
            return Ok(false);
        }
        Ok(self.next(at, self.text.len())? == at)
    }
}
impl<P: BoundaryProvider> Drop for LocalLinePolicy<'_, P> {
    fn drop(&mut self) {
        self.invalidate()
    }
}
