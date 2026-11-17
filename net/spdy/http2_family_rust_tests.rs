// Source-only ordinary deterministic control-flow tests. Not executed.
use crate::{dispatch, Services};
struct Script {
    steps: std::collections::VecDeque<(u32, i64, i64)>,
}
impl Services for Script {
    fn call(&mut self, op: u32, arg: i64) -> i64 {
        let (expected_op, expected_arg, result) =
            self.steps.pop_front().expect("unexpected native operation after terminal edge");
        assert_eq!((op, arg), (expected_op, expected_arg));
        result
    }
}
#[test]
fn closed_adjust_does_not_read_window() {
    let mut s = Script { steps: vec![(0, 0, 5)].into() };
    assert_eq!(dispatch(1, 6, &mut s), 1);
    assert!(s.steps.is_empty());
}
#[test]
fn overflow_does_not_commit_or_notify() {
    let mut s = Script { steps: vec![(0, 0, 1),(1, 0, 1),(2, 0, 2147483647)].into() };
    assert_eq!(dispatch(1, 6, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn negative_adjust_commits_before_log_resume() {
    let mut s = Script {
        steps: vec![(0, 0, 1),(1, 0, -5),(2, 0, 10),(3, 0, 0),(4, 5, 0),(5, 0, 0),(6, 0, 0)].into(),
    };
    assert_eq!(dispatch(1, 6, &mut s), 1);
    assert!(s.steps.is_empty());
}
#[test]
fn normal_consumption_does_not_restore_window() {
    let mut s = Script { steps: vec![(0, 0, 0)].into() };
    assert_eq!(dispatch(1, 7, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn discard_restores_only_payload() {
    let mut s = Script { steps: vec![(0, 0, 1),(1, 0, 20),(2, 0, 7),(3, 7, 0),(4, 7, 0)].into() };
    assert_eq!(dispatch(1, 7, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn data_before_headers_is_terminal() {
    let mut s = Script { steps: vec![(0, 0, 0),(1, 0, 0),(2, 0, 0)].into() };
    assert_eq!(dispatch(1, 21, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn trailers_commit_before_delegate() {
    let mut s = Script { steps: vec![(0, 0, 1),(13, 2, 0),(22, 0, 0)].into() };
    assert_eq!(dispatch(1, 20, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn headers_sent_commit_sequence() {
    let mut s = Script { steps: vec![(0, 0, 0),(1, 1, 0),(2, 0, 0)].into() };
    assert_eq!(dispatch(1, 24, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn dequeue_owner_deletion_prevents_later_owner_reads() {
    let mut s = Script {
        steps:
            vec![(0, 0, 1),(1, 0, 0),(2, 0, 0),(3, 0, 0),(4, 0, 0),(5, 0, 9),(6, 0, 0),(7, 0, -100)]
                .into(),
    };
    assert_eq!(dispatch(2, 5, &mut s), -100);
    assert!(s.steps.is_empty());
}
#[test]
fn closed_error_callback_is_terminal() {
    let mut s = Script {
        steps: vec![(0, 0, 0),(1, 0, 1),(2, 0, -20),(3, 0, 0),(4, 0, 1),(5, 0, 0)].into(),
    };
    assert_eq!(dispatch(2, 33, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn request_callback_deletion_stops_close_delivery() {
    let mut s = Script { steps: vec![(0, 0, 0),(1, 0, 0),(4, 0, 0),(5, 0, 0),(6, 0, 0),(7, 0, 0),(8, 0, 0),(9, 0, 0),(10, 0, 0),(11, 0, 0),(12, 0, 1),(13, 0, 0),(14, 0, 0)].into() };
    assert_eq!(dispatch(2, 22, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn upload_presence_short_circuits_provider_reads() {
    let mut s = Script { steps: vec![(40000, 0, 0),(40001, 0, 0)].into() };
    assert_eq!(dispatch(2, 25, &mut s), 0);
    assert!(s.steps.is_empty());
}
#[test]
fn small_read_retains_one_ms_delay() {
    let mut s = Script { steps: vec![(0, 0, 0),(1, 0, 1),(2, 0, 8),(3, 0, 9),(5, 0, 0)].into() };
    assert_eq!(dispatch(2, 32, &mut s), 0);
    assert!(s.steps.is_empty());
}
