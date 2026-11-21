//! Safe common request-family helpers; native operations return initialized POD.
pub use crate::Services;
use crate::Words;
pub fn w(x: i64) -> Words {
    Words { a: 0, b: 0, x, y: 0 }
}
pub fn wa(a: u64) -> Words {
    Words { a, b: 0, x: 0, y: 0 }
}
pub fn partition<S: Services>(s: &mut S, enabled: i64) -> Words {
    w(i64::from(enabled != 0 && s.call(0, w(0)).x != 0))
}
