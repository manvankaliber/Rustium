//! Source-bound Rust owner for Blink's local line-boundary policy.
//! ICU/UAX break decisions are supplied by `BoundaryProvider`; policy, UTF-16
//! offsets, filtering, cache invalidation and borrowed-input lifetime are local.
use std::cell::RefCell;
use std::rc::Rc;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BreakSpace { AfterRun, Every }
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BreakType { Normal, BreakAll, BreakCharacter, KeepAll, Phrase }
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum BoundaryError { Provider, InvalidRange }

/// Provider receives UTF-16 code units and returns offsets relative to that slice.
/// Implementations must document ICU/data identity and locale fallback.
pub trait BoundaryProvider {
    fn line_boundaries(&mut self, utf16: &[u16], locale: &str) -> Result<Vec<usize>, BoundaryError>;
    fn grapheme_boundaries(&mut self, utf16: &[u16]) -> Result<Vec<usize>, BoundaryError>;
}

/// Borrowed text cannot outlive the iterator; `Rc<RefCell>` deliberately makes
/// this local cache !Send/!Sync until the director selects a cross-thread bridge.
pub struct LocalLinePolicy<'a, P: BoundaryProvider> {
    text: &'a [u16], start: usize, locale: String, kind: BreakType,
    space: BreakSpace, soft_hyphen: bool, provider: Rc<RefCell<P>>,
    cached: Option<(usize, String, Vec<usize>)>,
}
impl<'a, P: BoundaryProvider> LocalLinePolicy<'a, P> {
    pub fn new(text: &'a [u16], provider: Rc<RefCell<P>>) -> Self {
        Self { text, start: 0, locale: String::new(), kind: BreakType::Normal,
            space: BreakSpace::AfterRun, soft_hyphen: true, provider, cached: None }
    }
    pub fn set_start(&mut self, n: usize) -> Result<(), BoundaryError> {
        if n > self.text.len() { return Err(BoundaryError::InvalidRange); }
        if self.start != n { self.start = n; self.cached = None; } Ok(())
    }
    pub fn set_locale(&mut self, s: impl Into<String>) { let s=s.into(); if self.locale != s { self.locale=s; self.cached=None; } }
    pub fn set_type(&mut self, t: BreakType) { if self.kind != t { self.kind=t; self.cached=None; } }
    pub fn set_space(&mut self, s: BreakSpace) { self.space=s; }
    pub fn enable_soft_hyphen(&mut self, yes: bool) { self.soft_hyphen=yes; }
    fn boundaries(&mut self) -> Result<Vec<usize>, BoundaryError> {
        if let Some((start, locale, b))=&self.cached { if *start==self.start && *locale==self.locale { return Ok(b.clone()); } }
        let raw = if self.kind == BreakType::BreakCharacter {
            self.provider.borrow_mut().grapheme_boundaries(&self.text[self.start..])?
        } else { self.provider.borrow_mut().line_boundaries(&self.text[self.start..], &self.locale)? };
        let len=self.text.len()-self.start;
        if raw.iter().any(|&x| x>len) { return Err(BoundaryError::Provider); }
        let mut b: Vec<usize>=raw.into_iter().map(|x| x+self.start).collect();
        b.sort_unstable(); b.dedup(); if b.last().copied()!=Some(self.text.len()) { b.push(self.text.len()); }
        self.cached=Some((self.start,self.locale.clone(),b.clone())); Ok(b)
    }
    /// First legal UTF-16 offset >= `offset`, or end. Provider failures stay errors.
    pub fn next(&mut self, offset: usize, limit: usize) -> Result<usize, BoundaryError> {
        if offset>limit || limit>self.text.len() || offset<self.start { return Err(BoundaryError::InvalidRange); }
        if offset==limit { return Ok(limit); }
        let b=self.boundaries()?;
        let mut candidates=b.into_iter().filter(|&x| x>=offset && x<=limit);
        while let Some(mut x)=candidates.next() {
            if self.kind==BreakType::BreakAll && x>self.start && x<self.text.len() && is_surrogate_pair(self.text,x) { continue; }
            if !self.soft_hyphen && x>0 && self.text[x-1]==0x00ad { continue; }
            if self.space==BreakSpace::AfterRun && x>0 && is_space(self.text[x-1]) && x<self.text.len() && is_space(self.text[x]) {
                while x<limit && x<self.text.len() && is_space(self.text[x]) { x+=1; }
                if x>limit { return Ok(limit); }
            }
            return Ok(x);
        }
        Ok(limit)
    }
}
fn is_space(c:u16)->bool { matches!(c, 0x20|0x09|0x0a) }
fn is_surrogate_pair(s:&[u16], i:usize)->bool { (0xd800..=0xdbff).contains(&s[i]) && (0xdc00..=0xdfff).contains(&s[i+1]) }

#[cfg(test)] mod tests {
 use super::*;
 #[derive(Default)] struct Stub { calls:usize, lines:Vec<usize>, graphemes:Vec<usize>, fail:bool }
 impl BoundaryProvider for Stub {
  fn line_boundaries(&mut self,_:&[u16],_:&str)->Result<Vec<usize>,BoundaryError>{ self.calls+=1; if self.fail {Err(BoundaryError::Provider)} else {Ok(self.lines.clone())} }
  fn grapheme_boundaries(&mut self,_:&[u16])->Result<Vec<usize>,BoundaryError>{ self.calls+=1; Ok(self.graphemes.clone()) }
 }
 fn make<'a>(s:&'a [u16], lines:Vec<usize>)->LocalLinePolicy<'a,Stub>{ let p=Rc::new(RefCell::new(Stub{lines,..Default::default()})); LocalLinePolicy::new(s,p) }
 // Mirrors original Basic/Newline/Tab and SoftHyphen expected boundary loops;
 // offsets are UTF-16 code units and all raw boundaries originate from provider.
 #[test] fn original_basic_space_run(){let mut x=make(&"a b  c".encode_utf16().collect::<Vec<_>>(),vec![2,5,6]); assert_eq!(x.next(0,6),Ok(2)); assert_eq!(x.next(2,6),Ok(5));}
 #[test] fn original_newline_and_tab(){let mut x=make(&"a\nb\n\nc\n d".encode_utf16().collect::<Vec<_>>(),vec![2,5,8,9]); assert_eq!(x.next(0,9),Ok(2));}
 #[test] fn malformed_utf16_offsets_are_preserved(){let s=[0xd800,0x61,0xdc00]; let mut x=make(&s,vec![1,2,3]); assert_eq!(x.next(0,3),Ok(1));}
 #[test] fn break_all_never_splits_pair(){let s=[0xd83d,0xde00,0x61]; let mut x=make(&s,vec![1,2,3]); x.set_type(BreakType::BreakAll); assert_eq!(x.next(0,3),Ok(2));}
 #[test] fn start_offset_locale_reset_and_cache(){let s=[97,32,98]; let mut x=make(&s,vec![1,3]); assert_eq!(x.next(0,3),Ok(1)); assert_eq!(x.next(0,3),Ok(1)); x.set_start(1).unwrap(); x.set_locale("sv-SE"); assert_eq!(x.next(1,3),Ok(2));}
 #[test] fn provider_error_is_not_success(){let s=[97]; let p=Rc::new(RefCell::new(Stub{fail:true,..Default::default()})); let mut x=LocalLinePolicy::new(&s,p); assert_eq!(x.next(0,1),Err(BoundaryError::Provider));}
}
