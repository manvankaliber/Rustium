//! Candidate policy and codec for Chromium's `FSU5` `.usage` Pickle record.
//! Native path, file-handle, flush, timer, and sequence checks are adapter duties.
use std::collections::BTreeMap;

pub const FILE_NAME: &str = ".usage";
pub const RECORD_SIZE: usize = 24;
const MAGIC: &[u8; 4] = b"FSU5";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Record { pub valid: bool, pub dirty: u32, pub usage: i64 }

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CodecError { Truncated, BadPickleLength, BadMagic, InvalidBool }

/// Exact Pickle bytes: u32 payload length (20), FSU5, Pickle int bool,
/// u32 dirty count, i64 usage. All fields use Chromium Pickle's native
/// little-endian POD representation on supported Chromium targets.
pub fn decode(bytes: &[u8]) -> Result<Record, CodecError> {
    if bytes.len() < RECORD_SIZE { return Err(CodecError::Truncated); }
    if u32::from_le_bytes(bytes[0..4].try_into().unwrap()) != 20 {
        return Err(CodecError::BadPickleLength);
    }
    if &bytes[4..8] != MAGIC { return Err(CodecError::BadMagic); }
    let b = i32::from_le_bytes(bytes[8..12].try_into().unwrap());
    if b != 0 && b != 1 { return Err(CodecError::InvalidBool); }
    Ok(Record { valid: b == 1,
        dirty: u32::from_le_bytes(bytes[12..16].try_into().unwrap()),
        usage: i64::from_le_bytes(bytes[16..24].try_into().unwrap()) })
}

pub fn encode(record: Record) -> [u8; RECORD_SIZE] {
    let mut b = [0; RECORD_SIZE];
    b[0..4].copy_from_slice(&20u32.to_le_bytes()); b[4..8].copy_from_slice(MAGIC);
    b[8..12].copy_from_slice(&(i32::from(record.valid)).to_le_bytes());
    b[12..16].copy_from_slice(&record.dirty.to_le_bytes());
    b[16..24].copy_from_slice(&record.usage.to_le_bytes()); b
}

/// Provider boundary. Implementations must provide exact-length reads and
/// writes at offset zero; writes do not implicitly flush. Delete is called
/// after a failed write, matching the original cleanup behavior.
pub trait FileProvider {
    type Key: Ord + Clone;
    type Error;
    fn read_exact(&mut self, key: &Self::Key, out: &mut [u8]) -> Result<(), Self::Error>;
    fn write_exact(&mut self, key: &Self::Key, bytes: &[u8]) -> Result<(), Self::Error>;
    fn flush(&mut self, key: &Self::Key) -> Result<(), Self::Error>;
    fn exists(&mut self, key: &Self::Key) -> Result<bool, Self::Error>;
    fn delete(&mut self, key: &Self::Key) -> Result<bool, Self::Error>;
}

/// Stateful policy. `incognito` isolates records in this instance's memory;
/// no provider operation occurs for private keys. The host must pin this value
/// to one sequence and implement two-handle/five-second close management.
pub struct UsageCache<K: Ord + Clone> {
    incognito: bool,
    private: BTreeMap<K, [u8; RECORD_SIZE]>,
}
impl<K: Ord + Clone> UsageCache<K> {
    pub fn new(incognito: bool) -> Self { Self { incognito, private: BTreeMap::new() } }
    fn read<P: FileProvider<Key=K>>(&self, p: &mut P, k: &K) -> Result<Record, P::Error> {
        let mut b = [0; RECORD_SIZE];
        if self.incognito { b.copy_from_slice(self.private.get(k).ok_or_else(|| unreachable!())?); }
        else { p.read_exact(k, &mut b)?; }
        // Corrupt/truncated bytes are a cache miss at the API boundary; preserve
        // provider errors separately through Result in concrete bridge adapter.
        decode(&b).map_err(|_| unreachable!("map codec miss before adapter"))
    }
    fn write<P: FileProvider<Key=K>>(&mut self, p: &mut P, k: &K, r: Record) -> Result<(), P::Error> {
        let bytes = encode(r);
        if self.incognito { self.private.insert(k.clone(), bytes); return Ok(()); }
        if let Err(e) = p.write_exact(k, &bytes) { let _ = p.delete(k); return Err(e); }
        Ok(())
    }
    pub fn put<P: FileProvider<Key=K>>(&mut self,p:&mut P,k:&K,usage:i64)->Result<(),P::Error>{self.write(p,k,Record{valid:true,dirty:0,usage})}
    pub fn get<P: FileProvider<Key=K>>(&self,p:&mut P,k:&K)->Result<Option<Record>,P::Error>{
        if self.incognito { return Ok(self.private.get(k).and_then(|b|decode(b).ok())); }
        let mut b=[0;RECORD_SIZE]; match p.read_exact(k,&mut b) { Ok(())=>Ok(decode(&b).ok()), Err(e)=>Err(e) }
    }
    pub fn increment<P: FileProvider<Key=K>>(&mut self,p:&mut P,k:&K)->Result<bool,P::Error>{
        let Some(mut r)=self.get(p,k)? else{return Ok(false)};
        let Some(d)=r.dirty.checked_add(1) else{return Ok(false)}; r.dirty=d;
        self.write(p,k,r)?; if d==1 && !self.incognito { p.flush(k)?; } Ok(true)
    }
    pub fn decrement<P: FileProvider<Key=K>>(&mut self,p:&mut P,k:&K)->Result<bool,P::Error>{let Some(mut r)=self.get(p,k)? else{return Ok(false)}; if r.dirty==0{return Ok(false)} r.dirty-=1; self.write(p,k,r)?; Ok(true)}
    pub fn invalidate<P: FileProvider<Key=K>>(&mut self,p:&mut P,k:&K)->Result<bool,P::Error>{let Some(mut r)=self.get(p,k)? else{return Ok(false)};r.valid=false;self.write(p,k,r)?;Ok(true)}
    pub fn update_delta<P: FileProvider<Key=K>>(&mut self,p:&mut P,k:&K,d:i64)->Result<bool,P::Error>{let Some(mut r)=self.get(p,k)? else{return Ok(false)};let Some(u)=r.usage.checked_add(d) else{return Ok(false)};r.usage=u;self.write(p,k,r)?;Ok(true)}
    pub fn exists<P: FileProvider<Key=K>>(&self,p:&mut P,k:&K)->Result<bool,P::Error>{if self.incognito{Ok(self.private.contains_key(k))}else{p.exists(k)}}
    pub fn delete<P: FileProvider<Key=K>>(&mut self,p:&mut P,k:&K)->Result<bool,P::Error>{if self.incognito{Ok(self.private.remove(k).is_some())}else{p.delete(k)}}
}

#[cfg(test)] mod tests {
 use super::*;
 #[test] fn exact_bytes_and_signed_values(){let r=Record{valid:true,dirty:u32::MAX,usage:i64::MIN};let b=encode(r);assert_eq!(b.len(),24);assert_eq!(&b[..8], &[20,0,0,0,b'F',b'S',b'U',b'5']);assert_eq!(decode(&b),Ok(r));}
 #[test] fn corrupt_truncated_and_bad_headers(){let b=encode(Record{valid:false,dirty:3,usage:-8});for n in 0..24{assert_eq!(decode(&b[..n]),Err(CodecError::Truncated));}let mut x=b;x[0]=19;assert_eq!(decode(&x),Err(CodecError::BadPickleLength));x=b;x[4]=0;assert_eq!(decode(&x),Err(CodecError::BadMagic));x=b;x[8]=2;assert_eq!(decode(&x),Err(CodecError::InvalidBool));}
}
