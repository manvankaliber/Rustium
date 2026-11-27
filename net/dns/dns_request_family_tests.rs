//! Ordinary bounded policy tests only. Drafts; no execution claimed.
use std::collections::BTreeMap;
use crate::{Services, Words};
use crate::dns_request_family_services::{w, wa};
use crate::dns_request_family_ids::*;

struct Model {
    domain: u32,
    reads: BTreeMap<i64, i64>,
    returns: BTreeMap<u32, Words>,
    calls: Vec<(u32, Words)>,
    queued: u64,
    dead: bool,
}
impl Model {
    fn new(domain: u32) -> Self {
        Self {
            domain,
            reads: BTreeMap::new(),
            returns: BTreeMap::new(),
            calls: Vec::new(),
            queued: 0,
            dead: false,
        }
    }
    fn field(mut self, k: i64, v: i64) -> Self {
        self.reads.insert(k, v);
        self
    }
    fn answer(mut self, k: u32, v: Words) -> Self {
        self.returns.insert(k, v);
        self
    }
    fn count(&self, op: u32) -> usize {
        self.calls.iter().filter(|(x, _)| *x == op).count()
    }
}
impl Services for Model {
    fn call(&mut self, op: u32, input: Words) -> Words {
        assert!(!self.dead,"native access after terminal callback");
        self.calls.push((op, input));
        let read = match self.domain {
            1 => RequestOp::Read,
            2 => EndpointOp::Read,
            3 => ClientOp::Read,
            4 => TransactionOp::Read,
            _ => ProbeOp::Read,
        };
        if op == read {
            return w(*self.reads.get(&input.x).unwrap_or(&0));
        }
        if self.domain == 1 && op == RequestOp::Write {
            if input.x == 1 {
                self.reads.insert(4, input.y);
            }
            if input.x == 5 {
                self.reads.insert(5, 1);
            }
        }
        if self.domain == 2 && op == EndpointOp::Write && input.x == 1 {
            self.reads.insert(9, input.y);
        }
        if self.domain == 3 {
            if op == ClientOp::Write && input.x == 4 {
                self.reads.insert(19, input.y);
            }
            if op == ClientOp::UpgradeNameservers {
                self.reads.insert(12, i64::from(input.x == 1));
            }
        }
        if self.domain == 4 {
            if op == TransactionOp::Write && input.x == 2 {
                self.reads.insert(3, input.y);
            }
            if op == TransactionOp::PushBase || op == TransactionOp::PushSuffix {
                self.queued += 1;
            }
            if op == TransactionOp::PopQname {
                self.queued -= 1;
            }
            if op == TransactionOp::QnamesEmpty {
                return w(i64::from(self.queued == 0));
            }
            if op == TransactionOp::EncodedByte {
                return wa(if input.a == 0 || input.a == 4 { 3 } else { 0 });
            }
        }
        let terminal = match self.domain {
            1 => op == RequestOp::RunCallback,
            2 => op == EndpointOp::RunFinished || op == EndpointOp::RunUpdated,
            4 => op == TransactionOp::RunCallback,
            _ => false,
        };
        if terminal {
            self.dead = true;
        }
        if let Some(value) = self.returns.get(&op) {
            return *value;
        }
        let squash = (self.domain == 1 && op == RequestOp::Squash)
            || (self.domain == 2 && op == EndpointOp::Squash);
        if squash {
            return w(match input.x {
                0 | -1 | -105 => input.x,
                _ => -105,
            });
        }
        w(0)
    }
}

#[test]
fn client_insecure_mode_and_config_matrix() {
    for mode in 0..4 {
        for names in 0..2 {
            for unhandled in 0..2 {
                let mut s = Model::new(3).field(2, mode).field(4, names).field(3, unhandled);
                let result =
                    crate::dns_client_policy::run(&mut s, ClientEvent::Insecure, w(-1)).x != 0;
                assert_eq!(result,mode>=2||(mode==1&&names!=0&&unhandled==0));
            }
        }
    }
}
#[test]
fn strict_platform_override_never_falls_back() {
    let mut s = Model::new(3).field(2, 0).field(7, 1).field(10, 100);
    assert_eq!(crate::dns_client_policy::run(&mut s,ClientEvent::Mode,w(2)).x,3);
    assert_eq!(crate::dns_client_policy::run(&mut s,ClientEvent::InsecureFallback,w(2)).x,0);
}
#[test]
fn secure_fallback_reason_is_preserved() {
    for (doh, available, reason) in [(0, 3, 1), (1, 0, 4), (1, 3, 0)] {
        let mut s = Model::new(3).field(1, doh).field(9, available);
        let result = crate::dns_client_policy::run(&mut s, ClientEvent::SecureFallback, w(0));
        assert_eq!(result.x,i64::from(reason!=0));
        assert!(s.calls.iter().any(|(op,v)|*op==ClientOp::Metric&&v.x==7&&v.y==reason));
    }
}
#[test]
fn doh_fallback_upgrade_keeps_metric_and_flag_order() {
    let mut s = Model::new(3)
        .field(14, 1)
        .field(15, 1)
        .field(16, 1)
        .field(17, 1)
        .answer(ClientOp::NameserverCount, wa(2))
        .answer(ClientOp::NameserverPublic, w(1))
        .answer(ClientOp::Feature, w(1));
    crate::dns_client_policy::run(&mut s, ClientEvent::Upgrade, w(0));
    let upgrades: Vec<i64> = s
        .calls
        .iter()
        .filter(|(op, _)| *op == ClientOp::UpgradeNameservers)
        .map(|(_, v)| v.x)
        .collect();
    assert_eq!(upgrades,vec![0,1]);
    assert_eq!(s.reads.get(&19),Some(&1));
}
#[test]
fn request_context_shutdown_never_stores_callback() {
    let mut s = Model::new(1);
    assert_eq!(crate::dns_request_policy::run(&mut s,RequestEvent::Start,w(0)).x,-105);
    assert!(!s.calls.iter().any(|(op,v)|*op==RequestOp::Write&&v.x==5));
    assert!(s.calls.iter().any(|(op,v)|*op==RequestOp::Write&&v.x==6&&v.y==-26));
}
#[test]
fn local_only_async_probe_finishes_synchronously() {
    let mut s = Model::new(1).field(1, 1).field(2, 1).answer(RequestOp::StartIPv6, w(-1));
    assert_eq!(crate::dns_request_policy::run(&mut s,RequestEvent::Start,w(0)).x,-105);
    assert_eq!(s.count(RequestOp::InitializeKey),0);
    assert!(!s.dead);
}
#[test]
fn request_job_completion_calls_client_last() {
    let mut s = Model::new(1);
    crate::dns_request_policy::run(&mut s, RequestEvent::Completed, w(-105));
    assert!(s.dead);
    assert_eq!(s.calls.last().unwrap().0,RequestOp::RunCallback);
}
#[test]
fn endpoint_shutdown_completion_calls_delegate_last() {
    let mut s = Model::new(2);
    crate::dns_endpoint_request_policy::run(&mut s, EndpointEvent::IOComplete, w(0));
    assert!(s.dead);
    assert_eq!(s.calls.last().unwrap().0,EndpointOp::RunFinished);
}
#[test]
fn negative_stale_result_is_not_exposed_during_refresh() {
    let mut s = Model::new(2)
        .field(3, 2)
        .field(4, 1)
        .field(5, 1)
        .answer(EndpointOp::LocalError, w(-105))
        .answer(EndpointOp::FreshError, w(-804));
    crate::dns_endpoint_request_policy::run(&mut s, EndpointEvent::ResolveLocal, w(0));
    assert_eq!(s.count(EndpointOp::ConvertStale),0);
    assert_eq!(s.count(EndpointOp::PostStale),0);
    assert!(s.calls.iter().any(|(op,v)|*op==EndpointOp::ResetStale&&v.x==1));
}
#[test]
fn endpoint_finalized_view_precedes_stale_and_live() {
    let mut s = Model::new(2).field(6, 1).field(7, 1).field(0, 1);
    assert_eq!(crate::dns_endpoint_request_policy::run(&mut s,EndpointEvent::EndpointSource,w(0)).x,1);
    assert_eq!(s.count(EndpointOp::HasTaskManager),0);
}
#[test]
fn suffix_search_places_base_before_search_when_ndots_met() {
    let mut s = Model::new(4)
        .field(8, 1)
        .field(9, 1)
        .answer(TransactionOp::EncodeHost, w(1))
        .answer(TransactionOp::BaseLength, wa(9))
        .answer(TransactionOp::SearchCount, wa(2))
        .answer(TransactionOp::EncodeSuffix, w(1))
        .answer(TransactionOp::EncodedLength, wa(12));
    assert_eq!(crate::dns_transaction_policy::run(&mut s,TransactionEvent::Prepare,w(0)).x,0);
    assert_eq!(s.queued,3);
    let pushes: Vec<u32> = s
        .calls
        .iter()
        .map(|(op, _)| *op)
        .filter(|op| *op == TransactionOp::PushBase || *op == TransactionOp::PushSuffix)
        .collect();
    assert_eq!(pushes,vec![TransactionOp::PushBase,TransactionOp::PushSuffix,TransactionOp::PushSuffix]);
}
#[test]
fn trailing_dot_skips_all_suffix_search() {
    let mut s = Model::new(4)
        .answer(TransactionOp::EncodeHost, w(1))
        .answer(TransactionOp::HostTrailing, w(1));
    assert_eq!(crate::dns_transaction_policy::run(&mut s,TransactionEvent::Prepare,w(0)).x,0);
    assert_eq!(s.queued,1);
    assert_eq!(s.count(TransactionOp::SearchCount),0);
}
#[test]
fn old_parallel_attempt_failure_is_not_recorded_twice() {
    let mut s = Model::new(4)
        .answer(TransactionOp::AttemptCount, wa(1))
        .answer(TransactionOp::AttemptAt, wa(2))
        .answer(TransactionOp::AttemptPending, w(1));
    let result = crate::dns_transaction_policy::run(
        &mut s,
        TransactionEvent::Process,
        Words { a: 1, ..w(-2) },
    );
    assert_eq!(result.x,-1);
    assert_eq!(s.count(TransactionOp::RecordFailure),0);
    assert_eq!(s.count(TransactionOp::TimerStop),0);
}
#[test]
fn transaction_failure_callback_is_terminal() {
    let mut s = Model::new(4).field(0, 1).field(11, 1);
    crate::dns_transaction_policy::run(&mut s, TransactionEvent::Callback, w(-105));
    assert!(s.dead);
    assert_eq!(s.calls.last().unwrap().0,TransactionOp::RunCallback);
}
#[test]
fn stopped_probe_context_drops_stats_without_attempt() {
    let mut s = Model::new(5);
    crate::dns_probe_policy::run(&mut s, ProbeEvent::Continue, wa(2));
    assert_eq!(s.count(ProbeOp::ClearAll),1);
    assert_eq!(s.count(ProbeOp::ConstructHttp),0);
}
#[test]
fn out_of_range_probe_delay_avoids_stats_dereference() {
    let mut s = Model::new(5);
    assert_eq!(crate::dns_probe_policy::run(&mut s,ProbeEvent::Delay,wa(1)).x,0);
    assert_eq!(s.count(ProbeOp::StatsPresent),0);
}
