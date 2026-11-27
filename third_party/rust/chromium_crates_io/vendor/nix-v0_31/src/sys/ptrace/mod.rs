//! Provides helpers for making ptrace system calls

#[cfg(linux)]
mod linux;

#[cfg(linux)]
pub use self::linux::*;

#[cfg(bsd)]
mod bsd;

#[cfg(bsd)]
pub use self::bsd::*;
