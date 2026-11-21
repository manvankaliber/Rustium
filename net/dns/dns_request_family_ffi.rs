//! Synchronous FFI only; policy modules hold no native pointers or Rust borrows.
use std::ffi::c_void;
use dns_resolution_policy::{Services, Words};
use dns_resolution_policy::dns_request_family_services::w;

type NativeCall = unsafe extern "C" fn(*mut c_void, u32, Words) -> Words;
struct Native {
    context: *mut c_void,
    call: NativeCall,
}
impl Services for Native {
    fn call(&mut self, op: u32, input: Words) -> Words {
        // SAFETY: the native invocation owns the frame for this synchronous call.
        // Replies are initialized POD returned by value. Policy creates no
        // references/slices into native storage and never retains this frame.
        unsafe { (self.call)(self.context, op, input) }
    }
}

#[no_mangle]
/// # Safety
/// `context` and `call` must denote one live synchronous native invocation.
/// The native callback returns initialized Words, does not unwind, and honors
/// the domain's ownership/terminal-callback contract until this function returns.
pub unsafe extern "C" fn rustium_dns_request_family_run(
    domain: u32,
    event: u32,
    input: Words,
    context: *mut c_void,
    call: NativeCall,
) -> Words {
    let mut native = Native { context, call };
    match domain {
        1 => dns_resolution_policy::dns_request_policy::run(&mut native, event, input),
        2 => dns_resolution_policy::dns_endpoint_request_policy::run(&mut native, event, input),
        3 => dns_resolution_policy::dns_client_policy::run(&mut native, event, input),
        4 => dns_resolution_policy::dns_transaction_policy::run(&mut native, event, input),
        5 => dns_resolution_policy::dns_probe_policy::run(&mut native, event, input),
        6 => dns_resolution_policy::dns_request_family_services::partition(&mut native, input.x),
        _ => w(-4),
    }
}
