// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
//! Ordinary state/order tests prepared with the full subsystem draft. Not run.
use crate::{Words, Services, manager, job, resolver_task as task, client_bridge};
use std::collections::VecDeque;
struct Step(u32, Words, Words);
#[derive(Default)]
struct Trace(VecDeque<Step>);
impl Trace {
    fn new(steps: Vec<Step>) -> Self {
        Self(steps.into())
    }
    fn done(&self) {
        assert!(self.0.is_empty(),"unvisited native operation");
    }
}
impl Services for Trace {
    fn call(&mut self, op: u32, input: Words) -> Words {
        let Step(expected, args, result) =
            self.0.pop_front().expect("unexpected access after terminal action");
        assert_eq!(op,expected);
        assert_eq!(input,args);
        result
    }
}
fn read(op: u32, result: Words) -> Step {
    Step(op, Words::default(), result)
}
fn action(op: u32) -> Step {
    read(op, Words::default())
}
fn boolean(op: u32, result: bool) -> Step {
    read(op, Words::flag(result))
}
fn unsigned(op: u32, result: u64) -> Step {
    read(op, Words::arg(result))
}
#[test]
fn disabled_secure_policy_never_queries_client() {
    let mut s = Trace::new(vec![unsigned(1,1)]);
    assert_eq!(manager::geteffectivesecurednsmode(&mut s),Words::arg(task::OFF));
    s.done();
}
#[test]
fn missing_cache_never_reads_clock_or_key() {
    let mut s = Trace::new(vec![action(1),boolean(2,false)]);
    manager::maybeservefromcache(&mut s);
    s.done();
}
#[test]
fn disallowed_cache_never_materializes_effective_key() {
    let mut s = Trace::new(vec![action(1),boolean(2,true),unsigned(3,3)]);
    manager::maybeservefromcache(&mut s);
    s.done();
}
#[test]
fn exhausted_job_without_results_is_terminal() {
    let mut s = Trace::new(vec![unsigned(1,0),unsigned(2,0),action(3)]);
    job::runnexttask(&mut s);
    s.done();
}
#[test]
fn last_request_cancellation_does_not_access_destroyed_job() {
    let mut s = Trace::new(vec![action(1),action(2),unsigned(3,0),action(6)]);
    job::cancelrequest(&mut s);
    s.done();
}
#[test]
fn surviving_request_detaches_after_priority_update() {
    let mut s = Trace::new(vec![action(1),action(2),unsigned(3,1),action(4),action(5)]);
    job::cancelrequest(&mut s);
    s.done();
}
#[test]
fn idle_finish_avoids_dispatcher_and_task_mutation() {
    let mut s = Trace::new(vec![boolean(1,false),boolean(11,false)]);
    job::finish(&mut s);
    s.done();
}
#[test]
fn cancellation_stops_if_request_callback_destroys_resolver() {
    let mut s =
        Trace::new(vec![action(1),boolean(2,true),action(3),action(4),action(5),boolean(6,false)]);
    job::cancelrequests(&mut s);
    s.done();
}
#[test]
fn completed_queued_job_preserves_reentrant_handle() {
    let mut s = Trace::new(vec![action(1),boolean(2,false),action(4),boolean(5,true)]);
    job::schedule(&mut s);
    s.done();
}
#[test]
fn intermediate_slots_reused_before_dispatcher_release() {
    let mut s=Trace::new(vec![boolean(1,true),action(2),read(3,Words::signed(3)),read(4,Words::signed(1)),
        read(5,Words::signed(1)),action(6),read(5,Words::signed(0)),boolean(7,true),read(5,Words::signed(0)),
        action(8),action(9),action(10),boolean(11,false)]);
    job::onintermediatetransactionscomplete(&mut s);
    s.done();
}
#[derive(Default)]
struct Tasks {
    items: VecDeque<u64>,
    secure: bool,
    fallback: bool,
}
impl Services for Tasks {
    fn call(&mut self, op: u32, w: Words) -> Words {
        match op {
            1 => self.items.push_front(w.a),
            2 => self.items.push_back(w.a),
            3 => {
                self.items.pop_front();
            }
            4 => return Words::arg(*self.items.front().unwrap()),
            5 => return Words::flag(self.secure),
            6 => return Words::flag(self.fallback),
            7 => {
                assert_eq!(self.items.front().copied(),Some(w.a));
                assert!(w.b==0||w.x!=0);
            }
            8 => return Words::arg(self.items.len() as u64),
            9 => return Words::arg(self.items[w.a as usize]),
            9000 => return Words::flag(false),
            9001 => (),
            _ => panic!("invalid task protocol op {op}"),
        }
        Words::default()
    }
}
#[test]
fn automatic_dns_separates_secure_and_insecure_cache_before_system_fallback() {
    let mut s =
        Tasks { items: VecDeque::from([task::CACHE, task::HOSTS]), secure: true, fallback: false };
    task::push_dns_tasks(
        &mut s,
        task::DnsTasks {
            allowed: true,
            fallback: true,
            system: true,
            secure: task::AUTOMATIC,
            insecure: 1,
            cache: true,
            local_first: false,
        },
    );
    assert_eq!(s.items,VecDeque::from([task::HOSTS,task::SECURE_CACHE,task::SECURE_DNS,task::INSECURE_CACHE,task::DNS,task::SYSTEM]));
}
#[test]
fn platform_no_system_does_not_gain_system_fallback() {
    let mut s = Tasks::default();
    task::push_dns_tasks(
        &mut s,
        task::DnsTasks {
            allowed: true,
            fallback: true,
            system: true,
            secure: task::OFF,
            insecure: 3,
            cache: false,
            local_first: false,
        },
    );
    assert_eq!(s.items,VecDeque::from([task::PLATFORM]));
}
#[test]
fn secure_misconfiguration_does_not_synthesize_system_task() {
    let mut s =
        Tasks { items: VecDeque::from([task::SECURE_CACHE]), secure: false, fallback: false };
    task::push_dns_tasks(
        &mut s,
        task::DnsTasks {
            allowed: true,
            fallback: true,
            system: false,
            secure: task::SECURE,
            insecure: 0,
            cache: true,
            local_first: false,
        },
    );
    assert_eq!(s.items,VecDeque::from([task::SECURE_CACHE]));
}
#[derive(Default)]
struct ClientFrame {
    depth: u32,
    allocations: u32,
    reads: u32,
}
impl Services for ClientFrame {
    fn call(&mut self, op: u32, input: Words) -> Words {
        match op {
            9000 => {
                if self.depth == 0 {
                    self.allocations += 1;
                }
                self.depth += 1;
                Words::flag(true)
            }
            9001 => {
                assert!(self.depth>0);
                self.depth -= 1;
                Words::default()
            }
            10000 => {
                assert!(self.depth>0);
                assert_eq!(input.x,1);
                self.reads += 1;
                Words::signed(1)
            }
            _ => panic!("unexpected client provider operation {op}"),
        }
    }
}
#[test]
fn complete_read_only_decision_reuses_one_provider_frame() {
    use crate::dns_request_family_ids::ClientEvent;
    let mut s = ClientFrame::default();
    client_bridge::scope(&mut s, |s| {
        assert!(client_bridge::boolean(s,ClientEvent::Secure,Words::default(),5));
        assert!(client_bridge::boolean(s,ClientEvent::Secure,Words::default(),5));
    });
    assert_eq!((s.allocations,s.reads,s.depth),(1,2,0));
}
#[test]
fn read_only_decision_without_capability_query_never_opens_frame() {
    let mut s = Trace::default();
    assert_eq!(client_bridge::scope(&mut s,|_|Words::flag(true)),Words::flag(true));
    s.done();
}
#[test]
fn custom_virtual_query_closes_before_client_mutation() {
    let mut s = Trace::new(vec![boolean(9000,false),unsigned(23,1),action(9001),action(5)]);
    let result = client_bridge::query(
        &mut s,
        crate::dns_request_family_ids::ClientEvent::Insecure,
        Words::signed(-1),
        23,
    );
    assert_eq!(result.x,1);
    s.act(5);
    s.done();
}

#[test]
fn job_without_resolve_context_defaults_to_opportunistic_ech() {
    let mut services = Trace::new(vec![boolean(1, false)]);
    assert_eq!(job::getechmode(&mut services), Words::arg(1));
    services.done();
}
#[test]
fn attached_resolve_context_preserves_every_ech_mode() {
    for mode in [0, 1, 2] {
        let mut services = Trace::new(vec![boolean(1, true), unsigned(2, mode)]);
        assert_eq!(job::getechmode(&mut services), Words::arg(mode));
        services.done();
    }
}
