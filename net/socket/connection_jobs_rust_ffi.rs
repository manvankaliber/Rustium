// Source-only connected-family draft; no compilation has run.
#[allow(dead_code)]
#[path = "../base/net_error_rust_ids.rs"]
mod error_ids;
pub mod errors {
    pub use super::error_ids::*;
    pub const OK: i32 = 0;
}
pub mod connection_jobs_rust_core;
pub mod connect_job_rust_core;
mod connect_job_rust_ffi;
pub mod transport_connect_job_rust_core;
pub mod tcp_connect_job_rust_core;
pub mod connect_job_factory_rust_core;
pub mod transport_connect_sub_job_rust_core;
pub mod tcp_connect_job_connector_rust_core;
pub mod connect_job_params_factory_rust_core;
mod socket_pool_additional_capacity_family_rust;
mod stream_socket_handle_family_rust;
pub mod connection_jobs_upper_rust_common;
mod ssl_connect_job_family_rust;
mod socks_connect_job_family_rust;
#[path = "../http/http_proxy_connect_job_family_rust.rs"]
mod http_proxy_connect_job_family_rust;
mod client_socket_handle_family_rust;
mod client_socket_pool_family_rust;
use std::ffi::c_void;
use connection_jobs_rust_core::{Reply, Services};
type Service = unsafe extern "C" fn(*mut c_void, u32, u32, i64, *mut i64) -> bool;
struct Native {
    context: *mut c_void,
    service: Option<Service>,
}
impl Services for Native {
    fn call(&mut self, module: u32, action: u32, argument: i64) -> Reply {
        let Some(service) = self.service else {
            return Reply { value: 0, alive: false };
        };
        let mut value = 0_i64;
        // SAFETY: synchronous caller-owned context and initialized output. The
        // callback reports invalidation before any later owner access.
        let alive = unsafe { service(self.context, module, action, argument, &mut value) };
        Reply { value, alive }
    }
}
struct BaseProjection<'a, S>(&'a mut S);
impl<S: Services> connect_job_rust_core::Services for BaseProjection<'_, S> {
    fn call(
        &mut self,
        action: connect_job_rust_core::Action,
        argument: i64,
    ) -> connect_job_rust_core::Reply {
        let r = self.0.call(0, action as u32, argument);
        connect_job_rust_core::Reply { value: r.value, alive: r.alive }
    }
}
pub fn base_run<S: Services>(operation: u32, argument: i64, native: &mut S) -> i64 {
    let Some(operation) = connect_job_rust_core::Operation::from_wire(operation) else {
        return errors::ERR_UNEXPECTED as i64;
    };
    connect_job_rust_core::run(operation, argument, &mut BaseProjection(native))
}
/// # Safety
/// Non-null callback/context pairs are synchronous and valid for this call;
/// neither is retained. Null callbacks are valid for pure scalar operations.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn connection_jobs_rust_run(
    module: u32,
    operation: u32,
    argument: i64,
    context: *mut c_void,
    service: Option<Service>,
) -> i64 {
    let native = &mut Native { context, service };
    match module {
        0 => base_run(operation, argument, native),
        1 => transport_connect_job_rust_core::run(operation, argument, native),
        2 if operation == 29 => tcp_connect_job_rust_core::fallback(native),
        2 => tcp_connect_job_rust_core::run(operation, argument, native),
        3 => connect_job_factory_rust_core::run(operation, argument, native),
        4 => ssl_connect_job_family_rust::run(operation, argument, native),
        5 => socks_connect_job_family_rust::run(operation, argument, native),
        6 => http_proxy_connect_job_family_rust::run(operation, argument, native),
        7 => client_socket_handle_family_rust::run(operation, argument, native),
        8 => client_socket_pool_family_rust::run(operation, argument, native),

        9 => transport_connect_sub_job_rust_core::run(operation, argument, native),
        10 => tcp_connect_job_connector_rust_core::run(operation, argument, native),
        11 => connect_job_params_factory_rust_core::run(operation, argument, native),
        12 => socket_pool_additional_capacity_family_rust::run(operation, argument, native),
        13 => stream_socket_handle_family_rust::run(operation, argument, native),
        _ => errors::ERR_UNEXPECTED as i64,
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn connection_jobs_rust_state_done(
    present_primary: bool,
    primary_done: bool,
    present_v4: bool,
    v4_done: bool,
) -> bool {
    (!present_primary || primary_done) && (!present_v4 || v4_done)
}
#[unsafe(no_mangle)]
pub extern "C" fn connection_jobs_rust_factory_has_input(present: bool) -> bool {
    present
}

#[unsafe(no_mangle)]
pub extern "C" fn connection_jobs_rust_connector_flag(kind: u32, state: i64) -> bool {
    tcp_connect_job_connector_rust_core::state_flag(kind, state)
}
#[unsafe(no_mangle)]
pub extern "C" fn connection_jobs_rust_either(first: bool, second: bool) -> bool {
    first || second
}

#[cfg(test)]
#[path = "connection_jobs_family_rust_tests.rs"]
mod family_tests;
