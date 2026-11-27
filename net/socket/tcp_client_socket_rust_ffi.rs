// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod tcp_client_socket_rust_core;
use std::ffi::c_void;
use tcp_client_socket_rust_core::{Action, Operation, Services, State};
#[repr(C)]
pub struct Reply {
    state: State,
    value: i64,
    alive: u64,
}
type Service = unsafe extern "C" fn(*mut c_void, State, u32, i64) -> Reply;
type FastService = unsafe extern "C" fn(*mut c_void, *mut State, u32, i64, *mut i64) -> bool;
struct Native {
    context: *mut c_void,
    service: Service,
}
impl Services for Native {
    fn call(&mut self, state: State, action: Action, value: i64) -> Option<(State, i64)> {
        // SAFETY: context is an invocation-local C++ bridge (not the owner).
        // Callback consumes/returns POD values only, retains no state pointer,
        // checks owner liveness, and never unwinds through Rust. Reentrant
        // calls use independent stack snapshots. No C++ memory is borrowed by
        // Rust.
        let reply = unsafe { (self.service)(self.context, state, action as u32, value) };
        (reply.alive != 0).then_some((reply.state, reply.value))
    }
}
/// # Safety
/// The synchronous service and invocation-local context must remain valid for
/// this call, including after owner deletion; neither is retained. The service
/// must obey the snapshot and liveness contract documented above.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn tcp_client_socket_rust_run(
    state: State,
    operation: u32,
    value: i64,
    context: *mut c_void,
    service: Service,
) -> i64 {
    let Some(op) = Operation::from_wire(operation) else {
        return tcp_client_socket_rust_core::ABORTED;
    };
    tcp_client_socket_rust_core::run(state, &mut Native { context, service }, op, value)
}

struct NativeFast {
    context: *mut c_void,
    service: FastService,
    final_state: State,
}

impl Services for NativeFast {
    fn call(&mut self, mut state: State, action: Action, value: i64) -> Option<(State, i64)> {
        if matches!(action, Action::Save) {
            // The C++ caller performs the final liveness check and commit after
            // this synchronous entrypoint returns. No native primitive or
            // callback occurs between this save and that check.
            self.final_state = state;
            return Some((state, 0));
        }
        let mut result = 0;
        // SAFETY: both output pointers address initialized invocation-local
        // Rust POD values. The synchronous service neither retains nor aliases
        // them and no Rust references to these values cross the call. It
        // commits before its native primitive and checks owner liveness before
        // copying any reentrant state back. Context remains stack-owned even
        // when that primitive deletes the socket.
        let alive = unsafe {
            (self.service)(self.context, &raw mut state, action as u32, value, &raw mut result)
        };
        alive.then_some((state, result))
    }
}

/// # Safety
/// `state` points to initialized, exclusively owned invocation-local POD
/// storage. The synchronous service/context obey the same ownership and
/// liveness contract as `tcp_client_socket_rust_run`, and retain neither
/// supplied output pointer. The caller checks owner liveness before publishing
/// the final state.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn tcp_client_socket_rust_run_fast(
    state: *mut State,
    operation: u32,
    value: i64,
    context: *mut c_void,
    service: FastService,
) -> i64 {
    let Some(op) = Operation::from_wire(operation) else {
        return tcp_client_socket_rust_core::ABORTED;
    };
    // SAFETY: the initialized POD belongs exclusively to this invocation.
    let initial = unsafe { state.read() };
    let mut native = NativeFast { context, service, final_state: initial };
    let result = tcp_client_socket_rust_core::run(initial, &mut native, op, value);
    // SAFETY: no callback retains this pointer; the service borrows only its
    // independent per-call copies above. The caller gates the native commit.
    unsafe { state.write(native.final_state) };
    result
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Probe {
        state: State,
        actions: Vec<(u32, i64)>,
        abort_read: bool,
    }

    unsafe extern "C" fn original_service(
        context: *mut c_void,
        state: State,
        action: u32,
        value: i64,
    ) -> Reply {
        // SAFETY: tests pass an exclusive invocation-local Probe and retain no
        // references across either entrypoint.
        let probe = unsafe { &mut *context.cast::<Probe>() };
        probe.state = state;
        if action != Action::Save as u32 {
            probe.actions.push((action, value));
        }
        if probe.abort_read && action == Action::Read as u32 {
            return Reply { state, value: 0, alive: 0 };
        }
        let result = match action {
            x if x == Action::Count as u32 => 2,
            x if x == Action::PortAllowed as u32 => 1,
            x if x == Action::Read as u32
                || x == Action::ReadReady as u32
                || x == Action::Write as u32 =>
            {
                // Simulate a native callback that changes the owner's scalar
                // state before the Rust continuation resumes.
                probe.state.received += 100;
                7
            }
            _ => 0,
        };
        Reply { state: probe.state, value: result, alive: 1 }
    }

    unsafe extern "C" fn pointer_service(
        context: *mut c_void,
        state: *mut State,
        action: u32,
        value: i64,
        result: *mut i64,
    ) -> bool {
        // SAFETY: both pointers are initialized exclusive locals supplied by
        // NativeFast, with no retained references from the original service.
        let reply = unsafe { original_service(context, state.read(), action, value) };
        unsafe {
            state.write(reply.state);
            result.write(reply.value);
        }
        reply.alive != 0
    }

    fn compare(operation: u32, value: i64, abort_read: bool) {
        let initial = State {
            index: -1,
            phase: if operation == Operation::CompleteConnect as u32 { 1 } else { 2 },
            received: 9,
            ..State::default()
        };
        let mut old = Probe { state: initial, actions: vec![], abort_read };
        let mut new = Probe { state: initial, actions: vec![], abort_read };
        let mut state = initial;
        // SAFETY: each service/context is synchronous and each output is
        // invocation-local. Neither service retains a supplied pointer.
        let old_result = unsafe {
            tcp_client_socket_rust_run(
                initial,
                operation,
                value,
                (&raw mut old).cast(),
                original_service,
            )
        };
        let new_result = unsafe {
            tcp_client_socket_rust_run_fast(
                &raw mut state,
                operation,
                value,
                (&raw mut new).cast(),
                pointer_service,
            )
        };
        assert_eq!(old_result, new_result, "operation {operation}");
        assert_eq!(old.actions, new.actions, "operation {operation}");
        if !abort_read {
            assert_eq!(old.state, state, "operation {operation}");
        }
    }

    #[test]
    fn pointer_entrypoint_preserves_all_operations_and_native_state_changes() {
        for operation in 0..=14 {
            compare(operation, 3, false);
        }
    }

    #[test]
    fn pointer_entrypoint_preserves_aborted_service_completion() {
        compare(Operation::Read as u32, 0, true);
    }

    #[test]
    fn pointer_entrypoint_preserves_invalid_operation_and_full_width_timeout() {
        compare(u32::MAX, 0, false);
        compare(Operation::Timeout as u32, i64::MAX, false);
    }
}
