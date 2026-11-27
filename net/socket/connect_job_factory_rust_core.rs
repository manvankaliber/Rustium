#![forbid(unsafe_code)]
use crate::connection_jobs_rust_core::{Controller, Module, Services};
use crate::errors::*;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Create = 0,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Action {
    IsSsl = 0,
    IsTransport = 1,
    IsHttp = 2,
    CheckSocks = 3,
    Ssl = 4,
    Transport = 5,
    Http = 6,
    Socks = 7,
}

pub fn run<S: Services>(op: u32, _arg: i64, native: &mut S) -> i64 {
    if op != 0 {
        return ERR_UNEXPECTED as i64;
    }
    let mut c = Controller::new(native, Module::Factory);
    let action = if c.flag(Action::IsSsl as u32, 0) {
        Action::Ssl
    } else if c.flag(Action::IsTransport as u32, 0) {
        Action::Transport
    } else if c.flag(Action::IsHttp as u32, 0) {
        Action::Http
    } else {
        if !c.step(Action::CheckSocks as u32, 0) {
            return 0;
        }
        Action::Socks
    };
    c.step(action as u32, 0);
    0
}
