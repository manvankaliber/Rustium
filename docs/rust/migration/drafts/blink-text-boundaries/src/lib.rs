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
    /// Opaque, non-transferable-to-the-user session token. Adapter must bind
    /// each token to exactly one cursor and retain the supplied stable buffer.
    type Session;
    fn acquire_line(
        &mut self,
        utf16: Rc<[u16]>,
        locale: &str,
    ) -> Result<Self::Session, BoundaryError>;
    fn following(
        &mut self,
        session: &Self::Session,
        query: i32,
    ) -> Result<Following, BoundaryError>;
    fn release_line(&mut self, session: Self::Session);
    fn acquire_character(&mut self, utf16: Rc<[u16]>) -> Result<Self::Session, BoundaryError>;
    fn character_following(
        &mut self,
        session: &Self::Session,
        query: i32,
    ) -> Result<Following, BoundaryError>;
    fn release_character(&mut self, session: Self::Session);
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
/// Mirrors LineBreakPropertyValue(last_ch, ch): `+` tailoring precedes ICU,
/// and a valid UTF-16 pair is classified at its trail code unit.
fn line_property<P: BoundaryProvider>(
    p: &mut P,
    left: u16,
    right: u16,
) -> Result<u8, BoundaryError> {
    if right == b'+' as u16 {
        return Ok(IcuLineBreak::AL as u8);
    }
    let value = if lead(left) && trail(right) {
        0x10000 + (((left as u32 - 0xd800) << 10) | (right as u32 - 0xdc00))
    } else {
        right as u32
    };
    p.line_property(value)
}
fn keep<P: BoundaryProvider>(
    p: &mut P,
    prev: u16,
    last: u16,
    next: u16,
) -> Result<bool, BoundaryError> {
    let c = if p.category(last as u32) == Category::Mark { prev } else { last };
    if !matches!(p.category(c as u32), Category::Letter | Category::Number) {
        return Ok(false);
    }
    if p.line_property(c as u32)? == IcuLineBreak::SA as u8 {
        return Ok(false);
    }
    if !matches!(p.category(next as u32), Category::Letter | Category::Number) {
        return Ok(false);
    }
    Ok(p.line_property(next as u32)? != IcuLineBreak::SA as u8)
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
pub struct LocalLinePolicy<P: BoundaryProvider> {
    text: Rc<[u16]>,
    start: usize,
    locale: String,
    kind: BreakType,
    space: BreakSpace,
    strict: Strictness,
    soft_hyphen: bool,
    quote: bool,
    line_session: Option<(P::Session, Rc<[u16]>)>,
    character_session: Option<(P::Session, Rc<[u16]>)>,
    provider: Rc<RefCell<P>>,
}
impl<P: BoundaryProvider> LocalLinePolicy<P> {
    pub fn new(
        text: impl Into<Rc<[u16]>>,
        provider: Rc<RefCell<P>>,
    ) -> Result<Self, BoundaryError> {
        let text = text.into();
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
            line_session: None,
            character_session: None,
            provider,
        })
    }
    fn invalidate(&mut self) {
        if let Some((session, _backing)) = self.line_session.take() {
            self.provider.borrow_mut().release_line(session);
        }
        if let Some((session, _backing)) = self.character_session.take() {
            self.provider.borrow_mut().release_character(session);
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
        if self.line_session.is_none() {
            let backing: Rc<[u16]> = Rc::from(&self.text[self.start..]);
            let session = self.provider.borrow_mut().acquire_line(backing.clone(), &self.locale)?;
            self.line_session = Some((session, backing));
        }
        let q = i32::try_from(q).map_err(|_| BoundaryError::TooLong)?;
        let (session, _backing) = self.line_session.as_ref().expect("acquired above");
        match self.provider.borrow_mut().following(session, q)? {
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
        if self.kind == BreakType::BreakCharacter {
            if self.character_session.is_none() {
                let backing: Rc<[u16]> = Rc::from(&self.text[self.start..]);
                let session = self.provider.borrow_mut().acquire_character(backing.clone())?;
                self.character_session = Some((session, backing));
            }
            let relative = pos - self.start;
            let q = if relative > 0 { relative - 1 } else { 0 };
            let query = i32::try_from(q).map_err(|_| BoundaryError::TooLong);
            let (session, _backing) = self.character_session.as_ref().expect("acquired above");
            let result = query.and_then(|q| {
                self.provider.borrow_mut().character_following(session, q).map(|v| (q, v))
            });
            let out = match result.and_then(|(q, following)| match following {
                Following::Boundary(n)
                    if n > q as i32 && n >= 0 && (n as usize) <= self.text.len() - self.start =>
                {
                    Ok(self.start + n as usize)
                }
                Following::Done => Ok(self.text.len()),
                _ => Err(BoundaryError::InvalidOffset),
            }) {
                Ok(v) => v,
                Err(e) => {
                    self.invalidate();
                    return Err(e);
                }
            };
            if let Some((session, _backing)) = self.character_session.take() {
                self.provider.borrow_mut().release_character(session);
            }
            return Ok(out);
        }
        if pos == limit {
            return Ok(limit);
        }
        let mut i = pos;
        let mut c = Ctx::new(self.text, self.start, pos);
        let mut cached = 0usize;
        let mut last_prop = if self.kind == BreakType::BreakAll {
            let mut p = self.provider.borrow_mut();
            line_property(&mut *p, c.previous, c.last)?
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
                let prop = line_property(&mut *p, c.last, c.current)?;
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
        let limit = at.saturating_add(1).min(self.text.len());
        Ok(self.next(at, limit)? == at)
    }
    pub fn next_opportunity(&mut self, offset: usize) -> Result<usize, BoundaryError> {
        if offset > self.text.len() {
            return Err(BoundaryError::InvalidRange);
        }
        self.next(offset, self.text.len())
    }
    pub fn next_opportunity_limited(
        &mut self,
        offset: usize,
        len: usize,
    ) -> Result<usize, BoundaryError> {
        if offset > len || len > self.text.len() {
            return Err(BoundaryError::InvalidRange);
        }
        self.next(offset, len)
    }
    /// Frozen native loop: probe forward at pos with at least one code point
    /// in the span, then step backward by one UTF-16 scalar (or one Latin-1
    /// unit), returning min when no boundary is found above it.
    pub fn previous_opportunity(
        &mut self,
        offset: usize,
        min: usize,
    ) -> Result<usize, BoundaryError> {
        let mut pos = offset.min(self.text.len());
        let mut end = pos.saturating_add(2).min(self.text.len());
        while pos > min {
            let next_break = self.next(pos, end)?;
            if next_break == pos {
                return Ok(pos);
            }
            end = pos;
            if pos > 0 && trail(self.text[pos - 1]) && pos >= 2 && lead(self.text[pos - 2]) {
                pos -= 2;
            } else if pos > 0 {
                pos -= 1;
            } else {
                break;
            }
        }
        Ok(min)
    }
}
impl<P: BoundaryProvider> Drop for LocalLinePolicy<P> {
    fn drop(&mut self) {
        self.invalidate()
    }
}

#[cfg(test)]
mod policy_trace_source {
    use super::*;

    struct FakeSession {
        id: usize,
        backing: Rc<[u16]>,
    }
    #[derive(Default)]
    struct Trace {
        events: Vec<String>,
        boundaries: Vec<i32>,
        property_error: bool,
        next_id: usize,
        character_boundary: Option<i32>,
        character_error: bool,
    }
    impl BoundaryProvider for Trace {
        type Session = FakeSession;
        fn acquire_line(
            &mut self,
            text: Rc<[u16]>,
            locale: &str,
        ) -> Result<Self::Session, BoundaryError> {
            self.next_id += 1;
            let id = self.next_id;
            self.events.push(format!("line({},{locale})#{id}", text.len()));
            Ok(FakeSession { id, backing: text })
        }
        fn following(
            &mut self,
            session: &Self::Session,
            q: i32,
        ) -> Result<Following, BoundaryError> {
            self.events.push(format!("following({}:{q})", session.id));
            Ok(self
                .boundaries
                .iter()
                .copied()
                .find(|v| *v > q)
                .map_or(Following::Done, Following::Boundary))
        }
        fn release_line(&mut self, session: Self::Session) {
            self.events.push(format!(
                "release-line({})-retained{}",
                session.id,
                session.backing.len()
            ));
        }
        fn acquire_character(&mut self, text: Rc<[u16]>) -> Result<Self::Session, BoundaryError> {
            self.next_id += 1;
            let id = self.next_id;
            self.events.push(format!("character#{id}"));
            Ok(FakeSession { id, backing: text })
        }
        fn character_following(
            &mut self,
            session: &Self::Session,
            q: i32,
        ) -> Result<Following, BoundaryError> {
            self.events.push(format!("character-following({}:{q})", session.id));
            if self.character_error {
                return Err(BoundaryError::Provider);
            }
            Ok(self.character_boundary.map_or(Following::Done, Following::Boundary))
        }
        fn release_character(&mut self, session: Self::Session) {
            self.events.push(format!(
                "release-character({})-retained{}",
                session.id,
                session.backing.len()
            ));
        }
        fn category(&mut self, _: u32) -> Category {
            Category::Letter
        }
        fn line_property(&mut self, cp: u32) -> Result<u8, BoundaryError> {
            self.events.push(format!("property(U+{cp:04X})"));
            if self.property_error {
                Err(BoundaryError::Provider)
            } else {
                Ok(IcuLineBreak::AL as u8)
            }
        }
    }

    // Prepared event-trace regressions. These are source fixtures; not run
    // under the active-build/user constraint and do not claim runtime parity.
    #[test]
    fn fast_positive_and_negative_avoid_icu() {
        let p = Rc::new(RefCell::new(Trace::default()));
        let yes_text = [b'a' as u16, b'(' as u16];
        let no_text = [b'a' as u16, b'b' as u16];
        let mut yes = LocalLinePolicy::new(&yes_text, p.clone()).unwrap();
        assert_eq!(yes.next(1, 2), Ok(1));
        let mut no = LocalLinePolicy::new(&no_text, p.clone()).unwrap();
        assert_eq!(no.next(1, 2), Ok(2));
        assert!(p.borrow().events.is_empty());
    }
    #[test]
    fn right_quote_feature_changes_fast_three_state() {
        let text = [b'a' as u16, 0x2019, b'b' as u16];
        let p = Rc::new(RefCell::new(Trace::default()));
        let mut enabled = LocalLinePolicy::new(&text, p.clone()).unwrap();
        enabled.set_right_quote(true);
        assert_eq!(enabled.next(1, 3), Ok(3));
        assert!(p.borrow().events.is_empty());
        let mut disabled = LocalLinePolicy::new(&text, p.clone()).unwrap();
        assert_eq!(disabled.next(1, 3), Ok(3));
        assert!(p.borrow().events.iter().any(|e| e.starts_with("following(")));
    }
    #[test]
    fn keep_all_propagates_property_error_unchanged() {
        let p = Rc::new(RefCell::new(Trace { property_error: true, ..Default::default() }));
        let text = [0x4e00, 0x4e01];
        let mut x = LocalLinePolicy::new(&text, p).unwrap();
        x.set_type(BreakType::KeepAll);
        assert_eq!(x.next(1, 2), Err(BoundaryError::Provider));
    }
    #[test]
    fn break_all_property_trace_covers_plus_mark_and_pair() {
        let p = Rc::new(RefCell::new(Trace::default()));
        let mut provider = p.borrow_mut();
        assert_eq!(
            line_property(&mut *provider, b'A' as u16, b'+' as u16),
            Ok(IcuLineBreak::AL as u8)
        );
        assert_eq!(line_property(&mut *provider, b'A' as u16, 0x0301), Ok(IcuLineBreak::AL as u8));
        assert_eq!(line_property(&mut *provider, 0xd83d, 0xde00), Ok(IcuLineBreak::AL as u8));
        assert!(after_all(
            IcuLineBreak::AL as u8,
            IcuLineBreak::AL as u8,
            b'a' as u16,
            Strictness::Normal
        ));
        assert!(!after_all(
            IcuLineBreak::AL as u8,
            IcuLineBreak::CM as u8,
            0x0301,
            Strictness::Normal
        ));
        assert!(!after_all(
            IcuLineBreak::AL as u8,
            IcuLineBreak::BA as u8,
            0x1361,
            Strictness::Normal
        ));
        assert!(after_all(
            IcuLineBreak::AL as u8,
            IcuLineBreak::BA as u8,
            0x1361,
            Strictness::Loose
        ));
        drop(provider);
        let events = &p.borrow().events;
        assert!(!events.iter().any(|e| e == "property(U+002B)")); // '+' is tailored to AL before ICU.
        assert!(events.iter().any(|e| e == "property(U+0301)"));
        assert!(events.iter().any(|e| e == "property(U+1F600)"));
    }
    #[test]
    fn following_is_strictly_after_query_and_soft_hyphen_retries() {
        let s = [b'a' as u16, 0xad, 0x4e2d];
        let p = Rc::new(RefCell::new(Trace { boundaries: vec![2], ..Default::default() }));
        let mut x = LocalLinePolicy::new(&s, p.clone()).unwrap();
        x.set_soft_hyphen(false);
        let _ = x.next(1, 3);
        let trace = p.borrow().events.join(",");
        assert!(trace.contains("following(1:1)"));
        assert!(trace.contains("following(1:2)"));
    }
    #[test]
    fn whitespace_after_run_and_every_space_are_local() {
        let s = [b' ' as u16, b' ' as u16, b'a' as u16];
        let p = Rc::new(RefCell::new(Trace::default()));
        let mut run = LocalLinePolicy::new(&s, p.clone()).unwrap();
        assert_eq!(run.next(0, 3), Ok(2));
        let mut each = LocalLinePolicy::new(&s, p.clone()).unwrap();
        each.set_space(BreakSpace::Every);
        assert_eq!(each.next(0, 3), Ok(1));
        assert!(p.borrow().events.is_empty());
    }
    #[test]
    fn policies_sharing_provider_keep_distinct_sessions_and_releases() {
        let text_a = Rc::<[u16]>::from([b'a' as u16, 0x4e2d]);
        let text_b = Rc::<[u16]>::from([b'b' as u16, 0x4e2d]);
        let p = Rc::new(RefCell::new(Trace { boundaries: vec![1], ..Default::default() }));
        let mut a = LocalLinePolicy::new(text_a, p.clone()).unwrap();
        let mut b = LocalLinePolicy::new(text_b, p.clone()).unwrap();
        let _ = a.next(1, 2);
        let _ = b.next(1, 2);
        a.set_locale("en");
        b.set_locale("ja");
        let events = p.borrow().events.join("|");
        assert!(events.contains("following(1:1)"));
        assert!(events.contains("following(2:1)"));
        assert!(events.contains("release-line(1)"));
        assert!(events.contains("release-line(2)"));
    }
    #[test]
    fn session_keeps_stable_suffix_backing_until_release() {
        let input = Rc::<[u16]>::from([b'a' as u16, 0x4e2d]);
        let p = Rc::new(RefCell::new(Trace { boundaries: vec![1], ..Default::default() }));
        let mut policy = LocalLinePolicy::new(input.clone(), p.clone()).unwrap();
        policy.set_start(1).unwrap();
        let _ = policy.next(1, 2);
        assert!(p.borrow().events.iter().any(|e| e == "line(1,)#1"));
        policy.set_start(0).unwrap();
        assert!(p.borrow().events.iter().any(|e| e == "release-line(1)-retained1"));
        assert_eq!(input[0], b'a' as u16);
    }

    // Source map: frozen text_break_iterator.{h,cc}, IsBreakable and
    // NextBreakOpportunity overloads (h:174-191, cc:467-478).
    #[test]
    fn forward_is_inclusive_and_enumeration_advances_by_returned_boundary() {
        let text = [b'a' as u16, 0x4e2d, b'b' as u16];
        let p = Rc::new(RefCell::new(Trace { boundaries: vec![1, 2], ..Default::default() }));
        let mut x = LocalLinePolicy::new(&text, p.clone()).unwrap();
        assert_eq!(x.next_opportunity(1), Ok(1));
        assert_eq!(x.next_opportunity(2), Ok(2));
        assert_eq!(x.next_opportunity(3), Ok(3));
        assert_eq!(x.is_breakable(3), Ok(true));
        assert!(p.borrow().events.iter().any(|e| e == "following(1:0)"));
    }
    #[test]
    fn empty_end_and_is_breakable_match_limited_native_wrapper() {
        let empty: [u16; 0] = [];
        let p = Rc::new(RefCell::new(Trace::default()));
        let mut x = LocalLinePolicy::new(&empty[..], p.clone()).unwrap();
        assert_eq!(x.next_opportunity(0), Ok(0));
        assert_eq!(x.is_breakable(0), Ok(true));
        let text = [b'a' as u16];
        let mut y = LocalLinePolicy::new(&text, p).unwrap();
        assert_eq!(y.next_opportunity(1), Ok(1));
        assert_eq!(y.is_breakable(1), Ok(true));
    }
    #[test]
    fn character_uses_suffix_offsets_and_ignores_limited_scan_end() {
        let text = [b'x' as u16, b'a' as u16, b'b' as u16];
        let p = Rc::new(RefCell::new(Trace { character_boundary: Some(2), ..Default::default() }));
        let mut x = LocalLinePolicy::new(&text, p.clone()).unwrap();
        x.set_start(1).unwrap();
        x.set_type(BreakType::BreakCharacter);
        assert_eq!(x.next_opportunity_limited(2, 2), Ok(3));
        assert!(p.borrow().events.iter().any(|e| e == "character-following(1:0)"));
        assert!(p.borrow().events.iter().any(|e| e == "release-character(1)-retained2"));
    }
    #[test]
    fn reverse_steps_over_supplementary_pair_and_honors_minimum() {
        let text = [b'a' as u16, 0xd83d, 0xde00, b'b' as u16];
        let p = Rc::new(RefCell::new(Trace::default()));
        let mut x = LocalLinePolicy::new(&text, p).unwrap();
        assert_eq!(x.previous_opportunity(4, 3), Ok(3));
        assert_eq!(x.previous_opportunity(3, 2), Ok(2));
        assert_eq!(x.previous_opportunity(2, 2), Ok(2));
    }
    #[test]
    fn character_error_and_bad_offset_release_only_its_handle() {
        for (error, boundary, expected) in [
            (true, None, Err(BoundaryError::Provider)),
            (false, Some(9), Err(BoundaryError::InvalidOffset)),
        ] {
            let text = [b'a' as u16, b'b' as u16];
            let p = Rc::new(RefCell::new(Trace {
                character_error: error,
                character_boundary: boundary,
                ..Default::default()
            }));
            let mut x = LocalLinePolicy::new(&text, p.clone()).unwrap();
            x.set_type(BreakType::BreakCharacter);
            assert_eq!(x.next_opportunity_limited(0, 1), expected);
            let events = p.borrow().events.join("|");
            assert!(events.contains("release-character(1)-retained2"));
            assert!(!events.contains("release-line"));
        }
    }
}
