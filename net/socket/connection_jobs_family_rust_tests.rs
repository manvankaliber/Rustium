//! Prepared ordinary unit checks for control ordering. Not a native acceptance
//! substitute; these exercise the real safe cores with scalar primitive replies.
use crate::connection_jobs_rust_core::{Reply, Services};
use crate::{
    connect_job_factory_rust_core as factory, tcp_connect_job_rust_core as tcp,
    transport_connect_job_rust_core as transport,
};
use std::collections::BTreeMap;
#[derive(Default)]
struct Native {
    values: BTreeMap<(u32, u32), i64>,
    log: Vec<(u32, u32, i64)>,
    state: i64,
    waiting: i64,
    die_on: Option<(u32, u32)>,
}
impl Native {
    fn put(&mut self, m: u32, a: u32, v: i64) {
        self.values.insert((m, a), v);
    }
    fn called(&self, m: u32, a: u32) -> bool {
        self.log.iter().any(|&(x, y, _)| x == m && y == a)
    }
    fn position(&self, m: u32, a: u32) -> usize {
        self.log.iter().position(|&(x, y, _)| x == m && y == a).unwrap()
    }
}
impl Services for Native {
    fn call(&mut self, m: u32, a: u32, arg: i64) -> Reply {
        self.log.push((m, a, arg));
        let mut value = *self.values.get(&(m, a)).unwrap_or(&0);
        if m == 1 {
            if a == transport::Action::State as u32 {
                value = self.state;
            }
            if a == transport::Action::SetState as u32 {
                self.state = arg;
            }
        }
        if m == 2 {
            if a == tcp::Action::WaitingDeletion as u32 {
                value = self.waiting;
            }
            if a == tcp::Action::IncrementWaiting as u32 {
                self.waiting += 1;
            }
            if a == tcp::Action::DecrementWaiting as u32 {
                self.waiting -= 1;
            }
        }
        Reply { value, alive: self.die_on != Some((m, a)) }
    }
}
#[test]
fn factory_ssl_precedes_other_variants() {
    let mut n = Native::default();
    n.put(3, factory::Action::IsSsl as u32, 1);
    n.put(3, factory::Action::IsTransport as u32, 1);
    factory::run(0, 0, &mut n);
    assert!(n.called(3,factory::Action::Ssl as u32));
    assert!(!n.called(3,factory::Action::IsTransport as u32));
}
#[test]
fn factory_socks_diagnostic_precedes_ownership_transfer() {
    let mut n = Native::default();
    factory::run(0, 0, &mut n);
    assert!(n.position(3,factory::Action::CheckSocks as u32)<
            n.position(3,factory::Action::Socks as u32));
}
#[test]
fn factory_deletion_stops_before_virtual_dispatch() {
    let mut n = Native::default();
    n.die_on = Some((3, factory::Action::CheckSocks as u32));
    factory::run(0, 0, &mut n);
    assert!(!n.called(3,factory::Action::Socks as u32));
}
#[test]
fn transport_resolving_load_states_and_none_are_stable() {
    let mut n = Native::default();
    for state in 0..=2 {
        n.state = state;
        assert_eq!(transport::run(1,0,&mut n),10);
    }
    n.state = 5;
    assert_eq!(transport::run(1,0,&mut n),0);
}
#[test]
fn transport_connecting_load_state_wins_over_waiting() {
    let mut n = Native::default();
    n.state = 4;
    for a in [
        transport::Action::HasV6,
        transport::Action::V6Started,
        transport::Action::HasV4,
        transport::Action::V4Started,
    ] {
        n.put(1, a as u32, 1);
    }
    n.put(1, transport::Action::V6Load as u32, 11);
    n.put(1, transport::Action::V4Load as u32, 2);
    assert_eq!(transport::run(1,0,&mut n),11);
    assert!(!n.called(1,transport::Action::V4Load as u32));
}
#[test]
fn transport_priority_changes_only_active_resolution() {
    let mut n = Native::default();
    n.state = 2;
    transport::run(13, 4, &mut n);
    assert!(!n.called(1,transport::Action::Priority as u32));
    n.state = 1;
    transport::run(13, 4, &mut n);
    assert!(n.called(1,transport::Action::Priority as u32));
}
#[test]
fn transport_suspend_does_not_start_the_other_attempt() {
    let mut n = Native::default();
    n.put(1, transport::Action::HasV4 as u32, 1);
    assert_eq!(transport::run(9,crate::errors::ERR_NETWORK_IO_SUSPENDED as i64,&mut n),
               crate::errors::ERR_NETWORK_IO_SUSPENDED as i64);
    assert!(!n.called(1,transport::Action::StartSelected as u32));
}
#[test]
fn transport_override_does_not_start_a_resolver_request() {
    let mut n = Native::default();
    n.put(1, transport::Action::HasOverride as u32, 1);
    assert_eq!(transport::run(4,0,&mut n),0);
    assert_eq!(n.state,3);
    assert!(!n.called(1,transport::Action::StartRequest as u32));
}
#[test]
fn transport_async_delete_pause_posts_before_return() {
    let mut n = Native::default();
    n.put(1, transport::Action::HasCallback as u32, 1);
    n.put(1, transport::Action::HostCallback as u32, 1);
    assert_eq!(transport::run(5,0,&mut n),-1);
    assert_eq!(n.state,2);
    assert!(n.position(1,transport::Action::HostCallback as u32)<
            n.position(1,transport::Action::PostResolution as u32));
}
#[test]
fn transport_without_alternatives_is_svcb_optional() {
    let mut n = Native::default();
    n.put(1, transport::Action::Https as u32, 1);
    n.put(1, transport::Action::EchEnabled as u32, 1);
    assert_eq!(transport::run(14,0,&mut n),1);
}
#[test]
fn authority_endpoint_obeys_svcb_optional_bit() {
    let mut n = Native::default();
    assert_eq!(transport::run(15,0,&mut n),0);
    assert_eq!(transport::run(15,1,&mut n),1);
}
#[test]
fn tcp_waiting_deletion_does_not_advance_connectors() {
    let mut n = Native::default();
    n.waiting = 2;
    assert_eq!(tcp::run(8,0,&mut n),-1);
    assert!(!n.called(2,tcp::Action::AdvanceConnector as u32));
}
#[test]
fn tcp_async_deletion_counter_decrements_before_resumption() {
    let mut n = Native::default();
    n.waiting = 2;
    tcp::run(7, 1, &mut n);
    assert_eq!(n.waiting,1);
    assert!(n.position(2,tcp::Action::DecrementWaiting as u32)<
            n.position(2,tcp::Action::WaitingDeletion as u32));
    assert!(!n.called(2,tcp::Action::AdvanceConnector as u32));
}
#[test]
fn tcp_stale_connector_stops_when_request_disappears() {
    let mut n = Native::default();
    n.put(2, tcp::Action::InputIsStale as u32, 1);
    assert_eq!(tcp::run(14,0,&mut n),crate::errors::ERR_NAME_NOT_RESOLVED as i64);
    assert!(!n.called(2,tcp::Action::SelectIp as u32));
}
#[test]
fn tcp_state_without_connectors_is_done() {
    let mut n = Native::default();
    assert_eq!(tcp::run(19,0,&mut n),1);
}
#[test]
fn reset_state_order_stops_timer_before_owned_connector_destruction() {
    let mut n = Native::default();
    tcp::run(18, 1, &mut n);
    assert!(n.position(2,tcp::Action::StopSlow as u32)<n.position(2,tcp::Action::ResetPrimary as u32));
    assert!(n.position(2,tcp::Action::ResetPrimary as u32)<n.position(2,tcp::Action::ResetV4 as u32));
    assert!(n.position(2,tcp::Action::ResetV4 as u32)<n.position(2,tcp::Action::ResetIndex as u32));
}
#[test]
fn fallback_prefers_nonzero_destination_rtt_before_global_estimator() {
    let mut n = Native::default();
    for a in [
        tcp::FallbackAction::RttEnabled,
        tcp::FallbackAction::HasProperties,
        tcp::FallbackAction::LookupStats,
        tcp::FallbackAction::StatsNonzero,
        tcp::FallbackAction::HasRtt,
    ] {
        n.put(2, a as u32, 1);
    }
    tcp::fallback(&mut n);
    assert!(n.called(2,tcp::FallbackAction::RttFromStats as u32));
    assert!(!n.called(2,tcp::FallbackAction::HasNqe as u32));
    assert!(n.called(2,tcp::FallbackAction::Multiply as u32));
}
#[test]
fn fallback_minimum_clamp_precedes_maximum_branch() {
    let mut n = Native::default();
    for a in [
        tcp::FallbackAction::RttEnabled,
        tcp::FallbackAction::HasRtt,
        tcp::FallbackAction::BelowMinimum,
        tcp::FallbackAction::AboveMaximum,
    ] {
        n.put(2, a as u32, 1);
    }
    tcp::fallback(&mut n);
    assert!(n.called(2,tcp::FallbackAction::Minimum as u32));
    assert!(!n.called(2,tcp::FallbackAction::AboveMaximum as u32));
}
#[test]
fn connector_scalar_states_distinguish_waiting_and_done() {
    for state in 0..=7 {
        assert_eq!(crate::tcp_connect_job_connector_rust_core::state_flag(0,state),state==1);
        assert_eq!(crate::tcp_connect_job_connector_rust_core::state_flag(1,state),state==1||state==5);
        assert_eq!(crate::tcp_connect_job_connector_rust_core::state_flag(2,state),state==7);
    }
}
