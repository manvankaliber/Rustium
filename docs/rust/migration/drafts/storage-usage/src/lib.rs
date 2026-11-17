//! Candidate FSU5 codec and usage-cache policy. Native primitives are injected;
//! Rust owns handle caching, eviction order, timer decisions, and state updates.
use std::{collections::BTreeMap, time::Duration};
pub const FILE_NAME: &str = ".usage";
pub const RECORD_SIZE: usize = 24;
pub const MAX_HANDLES: usize = 2;
pub const CLOSE_DELAY: Duration = Duration::from_secs(5);
const MAGIC: &[u8; 4] = b"FSU5";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Record {
    pub valid: bool,
    pub dirty: u32,
    pub usage: i64,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CodecError {
    Truncated,
    BadPickleLength,
    BadMagic,
}
pub fn decode(b: &[u8]) -> Result<Record, CodecError> {
    if b.len() < RECORD_SIZE {
        return Err(CodecError::Truncated);
    }
    if u32::from_le_bytes(b[0..4].try_into().unwrap()) != 20 {
        return Err(CodecError::BadPickleLength);
    }
    if &b[4..8] != MAGIC {
        return Err(CodecError::BadMagic);
    }
    // Pickle ReadBool reads one byte (nonzero means true), then aligns over three.
    Ok(Record {
        valid: b[8] != 0,
        dirty: u32::from_le_bytes(b[12..16].try_into().unwrap()),
        usage: i64::from_le_bytes(b[16..24].try_into().unwrap()),
    })
}
pub fn encode(r: Record) -> [u8; RECORD_SIZE] {
    let mut b = [0; RECORD_SIZE];
    b[..4].copy_from_slice(&20u32.to_le_bytes());
    b[4..8].copy_from_slice(MAGIC);
    b[8] = u8::from(r.valid);
    b[12..16].copy_from_slice(&r.dirty.to_le_bytes());
    b[16..24].copy_from_slice(&r.usage.to_le_bytes());
    b
}

/// Primitive-only host boundary. Reads/writes are at the supplied offset and
/// may be short; returned counts are interpreted by Rust policy.
pub trait PrimitiveProvider {
    type Key: Ord + Clone;
    type Handle: Clone;
    type Error;
    fn is_empty_key(&self, key: &Self::Key) -> bool;
    fn open_always_read_write(&mut self, key: &Self::Key) -> Result<Self::Handle, Self::Error>;
    fn read_at(
        &mut self,
        h: &Self::Handle,
        offset: u64,
        out: &mut [u8],
    ) -> Result<usize, Self::Error>;
    fn write_at(&mut self, h: &Self::Handle, offset: u64, b: &[u8]) -> Result<usize, Self::Error>;
    fn flush(&mut self, h: &Self::Handle) -> Result<(), Self::Error>;
    fn close(&mut self, h: Self::Handle);
    fn exists(&mut self, key: &Self::Key) -> Result<bool, Self::Error>;
    fn delete(&mut self, key: &Self::Key) -> Result<bool, Self::Error>;
    fn restart_close_timer(&mut self, delay: Duration);
    fn stop_close_timer(&mut self);
}
#[derive(Debug, PartialEq, Eq)]
pub enum CacheError<E> {
    Missing,
    Codec(CodecError),
    Provider(E),
    ShortRead,
    ShortWrite,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SequenceMismatch;

pub struct UsageCache<K: Ord + Clone, H: Clone> {
    incognito: bool,
    private: BTreeMap<K, [u8; RECORD_SIZE]>,
    handles: BTreeMap<K, H>,
    bound_sequence: Option<u64>,
}
impl<K: Ord + Clone, H: Clone> UsageCache<K, H> {
    pub fn new(incognito: bool) -> Self {
        Self { incognito, private: BTreeMap::new(), handles: BTreeMap::new(), bound_sequence: None }
    }
    /// Candidate seam for the facade's existing sequence checker. Construction
    /// is detached; first use binds, subsequent use must match. This is not a
    /// scheduler or thread service and does not replace native DCHECK behavior.
    pub fn check_sequence(&mut self, token: u64) -> Result<(), SequenceMismatch> {
        match self.bound_sequence {
            None => {
                self.bound_sequence = Some(token);
                Ok(())
            }
            Some(bound) if bound == token => Ok(()),
            Some(_) => Err(SequenceMismatch),
        }
    }
    pub fn has_handle(&self, k: &K) -> bool {
        self.handles.contains_key(k)
    }
    pub fn close_all<P: PrimitiveProvider<Key = K, Handle = H>>(&mut self, p: &mut P) {
        for (_, h) in std::mem::take(&mut self.handles) {
            p.close(h)
        }
        p.stop_close_timer();
    }
    /// Timer callback uses exactly the same close-and-stop transition.
    pub fn on_timer<P: PrimitiveProvider<Key = K, Handle = H>>(&mut self, p: &mut P) {
        self.close_all(p)
    }
    /// Native facade must call this before destruction to close owned handles.
    pub fn close_for_destruction<P: PrimitiveProvider<Key = K, Handle = H>>(&mut self, p: &mut P) {
        self.close_all(p)
    }
    fn get_file<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<H, CacheError<P::Error>> {
        if self.handles.len() >= MAX_HANDLES {
            self.close_all(p)
        }
        p.restart_close_timer(CLOSE_DELAY);
        if let Some(h) = self.handles.get(k) {
            return Ok(h.clone());
        }
        match p.open_always_read_write(k) {
            Ok(h) => {
                self.handles.insert(k.clone(), h.clone());
                Ok(h)
            }
            Err(e) => {
                self.handles.remove(k);
                Err(CacheError::Provider(e))
            }
        }
    }
    fn read_record<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<Option<Record>, CacheError<P::Error>> {
        if p.is_empty_key(k) {
            return Err(CacheError::Missing);
        }
        if self.incognito {
            return match self.private.get(k) {
                None => Err(CacheError::Missing),
                Some(b) => decode(b).map_err(CacheError::Codec).map(Some),
            };
        }
        let h = self.get_file(p, k)?;
        let mut b = [0; RECORD_SIZE];
        let n = p.read_at(&h, 0, &mut b).map_err(CacheError::Provider)?;
        if n < RECORD_SIZE {
            return Err(CacheError::ShortRead);
        }
        decode(&b).map(Some).map_err(CacheError::Codec)
    }
    fn read_for_update<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<Option<Record>, CacheError<P::Error>> {
        match self.read_record(p, k) {
            Err(CacheError::Missing | CacheError::Codec(_) | CacheError::ShortRead) => Ok(None),
            other => other,
        }
    }
    fn write_record<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
        r: Record,
    ) -> Result<(), CacheError<P::Error>> {
        let b = encode(r);
        if self.incognito {
            self.private.insert(k.clone(), b);
            return Ok(());
        }
        let result = match self.get_file(p, k) {
            Ok(h) => match p.write_at(&h, 0, &b) {
                Ok(n) if n == RECORD_SIZE => Ok(()),
                Ok(_) => Err(CacheError::ShortWrite),
                Err(e) => Err(CacheError::Provider(e)),
            },
            Err(e) => Err(e),
        };
        if result.is_err() {
            let _ = self.delete(p, k);
        }
        result
    }
    pub fn get<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<Option<Record>, CacheError<P::Error>> {
        self.read_record(p, k)
    }
    pub fn put<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
        usage: i64,
    ) -> Result<(), CacheError<P::Error>> {
        self.write_record(p, k, Record { valid: true, dirty: 0, usage })
    }
    pub fn increment<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<bool, CacheError<P::Error>> {
        let new_handle = !self.incognito && !self.has_handle(k);
        let Some(mut r) = self.read_for_update(p, k)? else { return Ok(false) };
        let old_dirty = r.dirty;
        r.dirty = r.dirty.wrapping_add(1);
        self.write_record(p, k, r)?;
        if old_dirty == 0 && new_handle && !self.incognito {
            if let Ok(h) = self.get_file(p, k) {
                let _ = p.flush(&h);
            }
        }
        Ok(true)
    }
    pub fn decrement<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<bool, CacheError<P::Error>> {
        let Some(mut r) = self.read_for_update(p, k)? else { return Ok(false) };
        if r.dirty == 0 {
            return Ok(false);
        }
        r.dirty -= 1;
        self.write_record(p, k, r)?;
        Ok(true)
    }
    pub fn invalidate<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<bool, CacheError<P::Error>> {
        let Some(mut r) = self.read_for_update(p, k)? else { return Ok(false) };
        r.valid = false;
        self.write_record(p, k, r)?;
        Ok(true)
    }
    /// Signed addition behavior remains provisional pending a configured C++ oracle.
    pub fn update_delta<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
        d: i64,
    ) -> Result<bool, CacheError<P::Error>> {
        let Some(mut r) = self.read_for_update(p, k)? else { return Ok(false) };
        r.usage = r.usage.wrapping_add(d);
        self.write_record(p, k, r)?;
        Ok(true)
    }
    /// Bool/out-parameter facade shape: errors and misses return false without
    /// modifying the caller's output, matching the C++ public API contract.
    pub fn get_usage_into<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
        out: &mut i64,
    ) -> bool {
        match self.read_record(p, k) {
            Ok(Some(r)) => {
                *out = r.usage;
                true
            }
            _ => false,
        }
    }
    pub fn get_dirty_into<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
        out: &mut u32,
    ) -> bool {
        match self.read_record(p, k) {
            Ok(Some(r)) => {
                *out = r.dirty;
                true
            }
            _ => false,
        }
    }
    pub fn is_valid<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> bool {
        matches!(self.read_record(p, k), Ok(Some(Record { valid: true, .. })))
    }
    pub fn exists<P: PrimitiveProvider<Key = K, Handle = H>>(
        &self,
        p: &mut P,
        k: &K,
    ) -> Result<bool, CacheError<P::Error>> {
        if self.incognito {
            Ok(self.private.contains_key(k))
        } else {
            p.exists(k).map_err(CacheError::Provider)
        }
    }
    pub fn delete<P: PrimitiveProvider<Key = K, Handle = H>>(
        &mut self,
        p: &mut P,
        k: &K,
    ) -> Result<bool, CacheError<P::Error>> {
        self.close_all(p);
        if self.incognito {
            Ok(self.private.remove(k).is_some())
        } else {
            p.delete(k).map_err(CacheError::Provider)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::BTreeMap;
    #[derive(Default)]
    struct Fake {
        files: BTreeMap<String, Vec<u8>>,
        handle_keys: BTreeMap<usize, String>,
        events: Vec<String>,
        next: usize,
        fail_open: bool,
        fail_read: bool,
        short_read: bool,
        fail_write: bool,
        short_write: bool,
        fail_flush: bool,
        fail_delete: bool,
    }
    impl PrimitiveProvider for Fake {
        type Key = String;
        type Handle = usize;
        type Error = &'static str;
        fn is_empty_key(&self, k: &String) -> bool {
            k.is_empty()
        }
        fn open_always_read_write(&mut self, k: &String) -> Result<usize, Self::Error> {
            self.events.push(format!("open:{k}"));
            if self.fail_open {
                return Err("open");
            }
            self.next += 1;
            self.files.entry(k.clone()).or_default();
            self.handle_keys.insert(self.next, k.clone());
            Ok(self.next)
        }
        fn read_at(&mut self, h: &usize, o: u64, b: &mut [u8]) -> Result<usize, Self::Error> {
            self.events.push(format!("read:{h}:{o}"));
            if self.fail_read {
                return Err("read");
            }
            let k = self.handle_keys.get(h).ok_or("closed")?;
            let f = self.files.get(k).ok_or("missing")?;
            let offset = o as usize;
            let count = b.len().min(f.len().saturating_sub(offset));
            let n = if self.short_read { count.saturating_sub(1) } else { count };
            b[..n].copy_from_slice(&f[offset..offset + n]);
            Ok(n)
        }
        fn write_at(&mut self, h: &usize, o: u64, b: &[u8]) -> Result<usize, Self::Error> {
            self.events.push(format!("write:{h}:{o}"));
            if self.fail_write {
                return Err("write");
            }
            let k = self.handle_keys.get(h).cloned().ok_or("closed")?;
            let f = self.files.get_mut(&k).ok_or("missing")?;
            let offset = o as usize;
            let n = if self.short_write { b.len().saturating_sub(1) } else { b.len() };
            if f.len() < offset + n {
                f.resize(offset + n, 0)
            }
            f[offset..offset + n].copy_from_slice(&b[..n]);
            Ok(n)
        }
        fn flush(&mut self, h: &usize) -> Result<(), Self::Error> {
            self.events.push(format!("flush:{h}"));
            if !self.handle_keys.contains_key(h) {
                return Err("closed");
            }
            if self.fail_flush {
                Err("flush")
            } else {
                Ok(())
            }
        }
        fn close(&mut self, h: usize) {
            self.events.push(format!("close:{h}"));
            self.handle_keys.remove(&h);
        }
        fn exists(&mut self, k: &String) -> Result<bool, Self::Error> {
            Ok(self.files.contains_key(k))
        }
        fn delete(&mut self, k: &String) -> Result<bool, Self::Error> {
            self.events.push(format!("delete:{k}"));
            if self.fail_delete {
                return Err("delete");
            }
            self.files.remove(k);
            Ok(true)
        }
        fn restart_close_timer(&mut self, d: Duration) {
            self.events.push(format!("timer:{}", d.as_secs()))
        }
        fn stop_close_timer(&mut self) {
            self.events.push("stop".into())
        }
    }
    #[test]
    fn codec_and_original_cases() {
        let r = Record { valid: true, dirty: u32::MAX, usage: i64::MIN };
        assert_eq!(decode(&encode(r)), Ok(r));
        let mut b = encode(r);
        b[8] = 9;
        b[9..12].fill(255);
        assert!(decode(&b).unwrap().valid);
        let mut c = UsageCache::<String, usize>::new(true);
        let mut p = Fake::default();
        let k: String = "k".into();
        c.private.insert(k.clone(), encode(Record { valid: true, dirty: u32::MAX, usage: 7 }));
        assert!(c.increment(&mut p, &k).unwrap());
        assert_eq!(c.get(&mut p, &k).unwrap().unwrap().dirty, 0);
        assert!(!c.decrement(&mut p, &k).unwrap());
        assert_eq!(c.get(&mut p, &k).unwrap().unwrap().usage, 7);
    }
    #[test]
    fn construction_detached_then_first_use_binds() {
        let mut c = UsageCache::<String, usize>::new(false);
        assert!(c.check_sequence(41).is_ok());
        assert!(c.check_sequence(41).is_ok());
        assert_eq!(c.check_sequence(42), Err(SequenceMismatch));
    }
    #[test]
    fn capacity_hit_closes_all_before_timer_lookup() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let a = "a".to_string();
        let b = "b".to_string();
        assert_eq!(c.get(&mut p, &a), Err(CacheError::ShortRead));
        assert_eq!(c.get(&mut p, &b), Err(CacheError::ShortRead));
        p.events.clear();
        assert_eq!(c.get(&mut p, &a), Err(CacheError::ShortRead));
        assert_eq!(&p.events[..4], &["close:1", "close:2", "stop", "timer:5"]);
        assert_eq!(p.events[4], "open:a");
    }
    #[test]
    fn failed_open_keeps_timer_and_failed_write_deletes() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake { fail_open: true, ..Default::default() };
        let k = "k".to_string();
        assert_eq!(c.get(&mut p, &k), Err(CacheError::Provider("open")));
        assert_eq!(p.events, vec!["timer:5", "open:k"]);
        p.events.clear();
        assert_eq!(c.put(&mut p, &k, 1), Err(CacheError::Provider("open")));
        assert_eq!(p.events, vec!["timer:5", "open:k", "stop", "delete:k"]);
    }
    #[test]
    fn trailing_bytes_and_short_read_and_timer_close() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let k = "k".to_string();
        c.put(&mut p, &k, 9).unwrap();
        p.files.get_mut(&k).unwrap().extend_from_slice(&[1, 2]);
        assert_eq!(c.get(&mut p, &k).unwrap().unwrap().usage, 9);
        p.short_read = true;
        assert_eq!(c.get(&mut p, &k), Err(CacheError::ShortRead));
        c.on_timer(&mut p);
        assert!(p.events.iter().any(|e| e == "stop"));
    }
    #[test]
    fn flush_uses_saved_handle_and_dirty_state_and_ignores_error() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake { fail_flush: true, ..Default::default() };
        let k = "k".to_string();
        c.put(&mut p, &k, 3).unwrap();
        c.close_all(&mut p);
        p.events.clear();
        assert!(c.increment(&mut p, &k).unwrap());
        assert!(p.events.iter().any(|e| e.starts_with("flush:")));
        c.close_all(&mut p);
        p.events.clear();
        assert!(c.increment(&mut p, &k).unwrap());
        assert!(!p.events.iter().any(|e| e.starts_with("flush:")));
    }
    #[test]
    fn saved_new_handle_stays_false_when_capacity_evicts_during_read() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let a = "a".to_string();
        let b = "b".to_string();
        c.put(&mut p, &a, 1).unwrap();
        assert!(c.increment(&mut p, &a).unwrap());
        c.put(&mut p, &b, 2).unwrap();
        assert!(c.has_handle(&a) && c.has_handle(&b));
        p.events.clear();
        assert!(c.increment(&mut p, &a).unwrap());
        assert_eq!(&p.events[..5], &["close:1", "close:2", "stop", "timer:5", "open:a"]);
        assert!(!p.events.iter().any(|e| e.starts_with("flush:")));
    }
    #[test]
    fn write_at_capacity_evicts_and_restarts_timer() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let a = "a".to_string();
        let b = "b".to_string();
        c.put(&mut p, &a, 1).unwrap();
        c.put(&mut p, &b, 2).unwrap();
        p.events.clear();
        c.put(&mut p, &a, 3).unwrap();
        assert_eq!(&p.events[..4], &["close:1", "close:2", "stop", "timer:5"]);
        assert!(p.events[4].starts_with("open:"));
    }
    #[test]
    fn delete_closes_stops_then_deletes_and_destruction_is_idempotent() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let k = "k".to_string();
        c.put(&mut p, &k, 1).unwrap();
        p.events.clear();
        assert!(c.delete(&mut p, &k).unwrap());
        assert_eq!(&p.events[..3], &["close:1", "stop", "delete:k"]);
        p.events.clear();
        c.close_for_destruction(&mut p);
        assert_eq!(p.events, vec!["stop"]);
    }
    #[test]
    fn original_disk_assertions_for_private_and_persistent_modes() {
        for private in [false, true] {
            let mut c = UsageCache::<String, usize>::new(private);
            let mut p = Fake::default();
            let k = ".usage".to_string();
            assert!(c.put(&mut p, &k, 0).is_ok());
            let mut created = 9;
            assert!(c.get_usage_into(&mut p, &k, &mut created));
            assert_eq!(created, 0);
            c.put(&mut p, &k, 240122).unwrap();
            let mut usage = 0;
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, 240122);
            c.put(&mut p, &k, i64::MAX).unwrap();
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, i64::MAX);
            c.put(&mut p, &k, 98214).unwrap();
            assert!(c.increment(&mut p, &k).unwrap());
            let mut dirty = 0;
            assert!(c.get_dirty_into(&mut p, &k, &mut dirty));
            assert_eq!(dirty, 1);
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, 98214);
            c.put(&mut p, &k, 71839).unwrap();
            assert!(!c.decrement(&mut p, &k).unwrap());
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, 71839);
            c.put(&mut p, &k, 198491).unwrap();
            assert!(c.increment(&mut p, &k).unwrap());
            assert!(c.decrement(&mut p, &k).unwrap());
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, 198491);
            c.put(&mut p, &k, 854238).unwrap();
            assert!(!c.decrement(&mut p, &k).unwrap());
            assert!(c.increment(&mut p, &k).unwrap());
            assert!(c.get_dirty_into(&mut p, &k, &mut dirty));
            assert_eq!(dirty, 1);
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, 854238);
            c.put(&mut p, &k, 82412).unwrap();
            for _ in 0..20 {
                assert!(c.increment(&mut p, &k).unwrap())
            }
            for _ in 0..20 {
                assert!(c.decrement(&mut p, &k).unwrap())
            }
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, 82412);
            c.put(&mut p, &k, 19319).unwrap();
            for _ in 0..20 {
                assert!(c.increment(&mut p, &k).unwrap())
            }
            for _ in 0..19 {
                assert!(c.decrement(&mut p, &k).unwrap())
            }
            assert!(c.get_dirty_into(&mut p, &k, &mut dirty));
            assert_eq!(dirty, 1);
            assert!(c.get_usage_into(&mut p, &k, &mut usage));
            assert_eq!(usage, 19319);
            assert!(!c.get_usage_into(&mut p, &"missing".into(), &mut usage));
            assert!(!c.increment(&mut p, &"missing".into()).unwrap());
        }
    }
    #[test]
    fn error_and_corruption_leave_outputs_unchanged() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let k = "x".to_string();
        c.put(&mut p, &k, 42).unwrap();
        let mut out = 123;
        assert!(c.get_usage_into(&mut p, &k, &mut out));
        assert_eq!(out, 42);
        p.fail_read = true;
        out = 77;
        assert!(!c.get_usage_into(&mut p, &k, &mut out));
        assert_eq!(out, 77);
        p.fail_read = false;
        p.files.get_mut(&k).unwrap()[4] = 0;
        out = 88;
        assert!(!c.get_usage_into(&mut p, &k, &mut out));
        assert_eq!(out, 88);
    }
    #[test]
    fn negative_usage_and_malformed_record_boundaries() {
        let r = Record { valid: false, dirty: 0, usage: -71839 };
        let b = encode(r);
        assert_eq!(decode(&b), Ok(r));
        for n in 0..RECORD_SIZE {
            assert_eq!(decode(&b[..n]), Err(CodecError::Truncated));
        }
        let mut bad = b;
        bad[..4].copy_from_slice(&19u32.to_le_bytes());
        assert_eq!(decode(&bad), Err(CodecError::BadPickleLength));
        bad = b;
        bad[4] = 0;
        assert_eq!(decode(&bad), Err(CodecError::BadMagic));
    }
    #[test]
    fn provider_failures_delete_and_reopen_are_traced() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let k = "x".to_string();
        c.put(&mut p, &k, 1).unwrap();
        p.events.clear();
        p.fail_write = true;
        assert_eq!(c.put(&mut p, &k, 2), Err(CacheError::Provider("write")));
        assert_eq!(p.events, vec!["timer:5", "write:1:0", "close:1", "stop", "delete:x"]);
        p.events.clear();
        p.fail_write = false;
        p.fail_delete = true;
        assert_eq!(c.delete(&mut p, &k), Err(CacheError::Provider("delete")));
        assert_eq!(p.events, vec!["stop", "delete:x"]);
        p.fail_delete = false;
        p.fail_open = true;
        p.events.clear();
        assert_eq!(c.get(&mut p, &k), Err(CacheError::Provider("open")));
        assert_eq!(p.events, vec!["timer:5", "open:x"]);
    }
    #[test]
    fn write_open_failure_closes_other_handles_stops_timer_deletes_and_keeps_error() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let a = "a".to_string();
        let b = "b".to_string();
        c.put(&mut p, &a, 1).unwrap();
        p.events.clear();
        p.fail_open = true;
        assert_eq!(c.put(&mut p, &b, 2), Err(CacheError::Provider("open")));
        assert_eq!(p.events, vec!["timer:5", "open:b", "close:1", "stop", "delete:b"]);
    }
    #[test]
    fn empty_key_read_rejected_before_private_or_file_activity_but_write_allowed() {
        let empty = String::new();
        for private in [false, true] {
            let mut c = UsageCache::<String, usize>::new(private);
            let mut p = Fake::default();
            c.put(&mut p, &empty, 6).unwrap();
            p.events.clear();
            let mut out = 77;
            assert!(!c.get_usage_into(&mut p, &empty, &mut out));
            assert_eq!(out, 77);
            assert!(p.events.is_empty());
        }
    }
    #[test]
    fn partial_write_overwrites_prefix_and_retains_old_tail() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let k = "tail".to_string();
        c.put(&mut p, &k, 9).unwrap();
        p.files.get_mut(&k).unwrap().extend_from_slice(&[0xaa, 0xbb]);
        let h = *c.handles.get(&k).unwrap();
        p.short_write = true;
        let bytes = encode(Record { valid: false, dirty: 2, usage: -1 });
        assert_eq!(p.write_at(&h, 0, &bytes).unwrap(), RECORD_SIZE - 1);
        assert_eq!(p.files[&k].len(), RECORD_SIZE + 2);
        assert_eq!(&p.files[&k][..RECORD_SIZE - 1], &bytes[..RECORD_SIZE - 1]);
        assert_eq!(&p.files[&k][RECORD_SIZE..], &[0xaa, 0xbb]);
    }
    #[test]
    fn timer_restart_expiry_and_sequence_outcomes() {
        let mut c = UsageCache::<String, usize>::new(false);
        let mut p = Fake::default();
        let k = "x".to_string();
        assert_eq!(c.get(&mut p, &k), Err(CacheError::ShortRead));
        p.events.clear();
        assert_eq!(c.get(&mut p, &k), Err(CacheError::ShortRead));
        assert_eq!(p.events, vec!["timer:5", "read:1:0"]);
        p.events.clear();
        c.on_timer(&mut p);
        assert_eq!(p.events, vec!["close:1", "stop"]);
        assert!(c.check_sequence(11).is_ok());
        assert!(c.check_sequence(11).is_ok());
        assert_eq!(c.check_sequence(12), Err(SequenceMismatch));
    }
    #[test]
    fn private_instances_are_isolated_without_file_operations() {
        let mut p = Fake::default();
        let k = "same".to_string();
        let mut a = UsageCache::<String, usize>::new(true);
        let mut b = UsageCache::<String, usize>::new(true);
        a.put(&mut p, &k, 5).unwrap();
        assert_eq!(a.get(&mut p, &k).unwrap().unwrap().usage, 5);
        assert_eq!(b.get(&mut p, &k), Err(CacheError::Missing));
        assert!(!b.delete(&mut p, &k).unwrap());
        assert!(a.exists(&mut p, &k).unwrap());
        assert!(a.delete(&mut p, &k).unwrap());
        assert!(!a.exists(&mut p, &k).unwrap());
        // Native Delete invokes CloseCacheFiles even in private mode.
        assert_eq!(p.events, vec!["stop", "stop"]);
        assert!(p.files.is_empty());
        assert!(p.handle_keys.is_empty());
    }
}
