// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
//! Ordinary core tests. Native public compatibility is a separate, pending gate.
use crate::{session, session_pool};

#[derive(Default)]
struct WriteOwner {
    deleted: bool,
}
impl session::Services for WriteOwner {
    fn op(&mut self, op: u32, _: u64, _: u64, _: u64) -> i64 {
        assert!(!self.deleted, "native owner accessed after removal");
        match op {
            0 | 1 => 0,
            2 | 4 => 1,
            3 => 0,
            5 => {
                self.deleted = true;
                0
            }
            _ => panic!("unexpected native action"),
        }
    }
}

#[test]
fn final_write_removal_is_the_last_owner_operation() {
    let mut owner = WriteOwner::default();
    assert_eq!(session::pump_write_loop(&mut owner), 0);
    assert!(owner.deleted);
}

struct SendWindow {
    delta: i32,
    current: i32,
    wrote: bool,
    drained: bool,
}
impl session::Services for SendWindow {
    fn op(&mut self, op: u32, a: u64, _: u64, _: u64) -> i64 {
        match op {
            0 => i64::from(self.delta),
            1 => i64::from(self.current),
            2 => 0,
            3 => {
                self.drained = true;
                0
            }
            4 => {
                self.current = a as i32;
                self.wrote = true;
                0
            }
            5..=7 => 0,
            _ => panic!("unexpected action"),
        }
    }
}

#[test]
fn send_window_overflow_drains_without_writing_the_window() {
    let mut window = SendWindow { delta: 42, current: i32::MAX - 41, wrote: false, drained: false };
    session::increase_send_window_size(&mut window);
    assert!(window.drained);
    assert!(!window.wrote);
    assert_eq!(window.current, i32::MAX - 41);
}

#[test]
fn send_window_exact_maximum_is_accepted() {
    let mut window = SendWindow { delta: 42, current: i32::MAX - 42, wrote: false, drained: false };
    session::increase_send_window_size(&mut window);
    assert!(!window.drained);
    assert!(window.wrote);
    assert_eq!(window.current, i32::MAX);
}

struct DataPlan {
    len: i32,
    stream_window: i32,
    session_window: i32,
    effective: i32,
    flags: u64,
    stalled: bool,
    created: bool,
    pinged: bool,
    callback: bool,
    end_stream: bool,
}
impl session::Services for DataPlan {
    fn op(&mut self, op: u32, a: u64, _: u64, _: u64) -> i64 {
        match op {
            0 | 1 => 0,
            2 => i64::from(self.len),
            3 => panic!("invalid input diagnostic"),
            4 => 16375,
            5 => {
                self.effective = a as i32;
                0
            }
            6 => i64::from(self.stream_window),
            7 => i64::from(self.session_window == 0),
            8 | 10 | 11 | 13 | 15 | 22 => 0,
            9 => {
                self.stalled = true;
                0
            }
            12 => i64::from(self.effective),
            14 => i64::from(self.session_window),
            16 => self.flags as i64,
            17 => 1,
            18 => {
                self.flags = a;
                0
            }
            19 => {
                self.pinged = true;
                0
            }
            20 | 21 => {
                self.created = true;
                0
            }
            23 => {
                self.callback = true;
                0
            }
            24 => {
                self.end_stream = a != 0;
                0
            }
            _ => panic!("unexpected action"),
        }
    }
}
fn data_plan(len: i32, stream_window: i32, session_window: i32) -> DataPlan {
    DataPlan {
        len,
        stream_window,
        session_window,
        effective: -1,
        flags: 1,
        stalled: false,
        created: false,
        pinged: false,
        callback: false,
        end_stream: true,
    }
}

#[test]
fn data_payload_obeys_both_windows_and_defers_fin() {
    let mut plan = data_plan(100_000, 10, 5);
    session::create_data_buffer(&mut plan);
    assert_eq!(plan.effective, 5);
    assert!(plan.created && plan.pinged && plan.callback);
    assert!(!plan.end_stream);
    assert!(!plan.stalled);
}

#[test]
fn stalled_payload_preserves_partial_output_order() {
    let mut plan = data_plan(100_000, -1, 5);
    session::create_data_buffer(&mut plan);
    // The original writes its capped effective length before stall rejection.
    assert_eq!(plan.effective, 16375);
    assert!(plan.stalled);
    assert!(!plan.created && !plan.callback && !plan.pinged);
    // Rejection does not overwrite the caller's end_stream output.
    assert!(plan.end_stream);
}

#[test]
fn empty_fin_does_not_consume_flow_window_or_send_preface_ping() {
    let mut plan = data_plan(0, 10, 5);
    session::create_data_buffer(&mut plan);
    assert!(plan.created && plan.end_stream);
    assert_eq!(plan.effective, 0);
    assert!(!plan.callback && !plan.pinged && !plan.stalled);
}

#[test]
fn protocol_defaults_include_zero_connect_but_no_unbounded_defaults() {
    for (id, value) in [(1, 4096), (2, 1), (4, 65535), (5, 16384), (8, 0)] {
        assert!(session::setting_is_default(id,value));
        assert!(!session::setting_is_default(id,value+1));
    }
    for id in [0, 3, 6, 7, 9, u32::MAX] {
        assert!(!session::setting_is_default(id,0));
    }
}

struct EndpointEligibility(Vec<bool>);
impl session_pool::Services for EndpointEligibility {
    fn op(&mut self, op: u32, a: u64, _: u64, _: u64) -> i64 {
        match op {
            0 => self.0.len() as i64,
            1 => i64::from(self.0[a as usize]),
            _ => panic!("unexpected action"),
        }
    }
}
#[test]
fn endpoint_eligibility_distinguishes_empty_h3_and_h2_lists() {
    for (protocols, expected) in [(vec![], true), (vec![false], false), (vec![false,true], true)] {
        assert_eq!(session_pool::is_endpoint_eligible_for_h2_v0(&mut EndpointEligibility(protocols))!=0,expected);
    }
}

struct UniqueEndpoints {
    target: Vec<u32>,
    source: Vec<u32>,
}
impl session_pool::Services for UniqueEndpoints {
    fn op(&mut self, op: u32, a: u64, b: u64, _: u64) -> i64 {
        match op {
            0 => self.source.len() as i64,
            1 => self.target.len() as i64,
            2 => i64::from(self.target[b as usize] == self.source[a as usize]),
            3 => {
                self.target.push(self.source[a as usize]);
                0
            }
            _ => panic!("unexpected action"),
        }
    }
}
#[test]
fn endpoint_union_preserves_first_occurrence_across_both_inputs() {
    let mut endpoints = UniqueEndpoints { target: vec![2,1], source: vec![1,3,3,2,4] };
    session_pool::append_unique_ip_endpoints_v0(&mut endpoints);
    assert_eq!(endpoints.target,vec![2,1,3,4]);
}

#[derive(Default)]
struct PendingRequest {
    map_present: bool,
    removed: bool,
    delivered: usize,
}
impl session_pool::Services for PendingRequest {
    fn op(&mut self, op: u32, _: u64, _: u64, _: u64) -> i64 {
        match op {
            0 | 2 | 3 | 6 | 9 => 1,
            1 | 5 | 7 | 11 | 22 => 0,
            4 => i64::from(self.map_present),
            12 => {
                self.map_present = false;
                self.removed = true;
                0
            }
            13 => {
                assert!(self.removed);
                self.delivered += 1;
                0
            }
            _ => panic!("stale request map accessed after callback/removal"),
        }
    }
}
#[test]
fn pending_delivery_relooks_up_map_after_delegate_reentry() {
    let mut requests = PendingRequest { map_present: true, ..PendingRequest::default() };
    session_pool::update_pending_requests_v0(&mut requests);
    assert_eq!(requests.delivered,1);
    assert!(!requests.map_present);
}
