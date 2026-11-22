// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Paired with the independent original trace. The harness models native job
// ownership and priority propagation, not the browser or its providers.
mod retained_queue;
use retained_queue::{Action, Detach, RequestQueue, Token};
use std::collections::{HashMap, VecDeque};

struct Harness {
    queue: RequestQueue,
    requests: HashMap<Token, u8>,
    assigned_jobs: HashMap<Token, u32>,
    job_priorities: HashMap<u32, u8>,
    unassigned: VecDeque<u32>,
}
impl Harness {
    fn new() -> Self {
        Self {
            queue: RequestQueue::new(),
            requests: HashMap::new(),
            assigned_jobs: HashMap::new(),
            job_priorities: [(101, 0), (102, 0)].into(),
            unassigned: [101, 102].into(),
        }
    }
    fn order(&self) -> Vec<Token> {
        let mut out = vec![0; self.queue.len()];
        self.queue.copy_tokens(&mut out).unwrap();
        out
    }
    fn assign(&mut self, token: Token, job: u32) {
        self.assigned_jobs.insert(token, job);
        let priority = self.requests[&token];
        if self.job_priorities[&job] != priority {
            self.job_priorities.insert(job, priority);
            println!("priority job={job} value={priority} visible {:?}", self.order());
        }
    }
    fn apply(&mut self, token: Token, action: Action) -> String {
        match action {
            Action::None => "none".into(),
            Action::UnassignedFront => {
                let job = *self.unassigned.front().unwrap();
                self.assign(token, job);
                self.unassigned.pop_front();
                "front".into()
            }
            Action::Donor(donor) => {
                let job = self.assigned_jobs.remove(&donor).unwrap();
                self.assign(token, job);
                format!("donor={donor}")
            }
        }
    }
    fn create(&mut self, priority: u8, respect: bool) -> Token {
        let result = self
            .queue
            .create(priority, respect, self.assigned_jobs.len(), self.unassigned.len())
            .unwrap();
        // No native callback occurs between Rust publication and native owner
        // registration in this harness. Production needs a reviewed adapter.
        self.requests.insert(result.token, priority);
        let action = self.apply(result.token, result.action);
        println!("insert {} {action} {:?}", result.token, self.order());
        result.token
    }
    fn release_removed(&mut self, token: Token) {
        let mut action = "none".to_string();
        if let Some(job) = self.assigned_jobs.remove(&token) {
            self.unassigned.push_back(job);
            let target = self
                .queue
                .release_job(self.assigned_jobs.len(), self.unassigned.len(), false)
                .unwrap();
            if let Some(target) = target {
                let job = *self.unassigned.back().unwrap();
                self.assign(target, job);
                self.unassigned.pop_back();
                action = format!("target={target}");
            }
        }
        assert!(!self.order().contains(&token));
        println!("release {token} {action} {:?}", self.order());
    }
    fn remove(&mut self, token: Token) {
        self.queue.remove(token).unwrap();
        self.requests.remove(&token);
        self.release_removed(token);
    }
    fn reprioritize(&mut self, token: Token, priority: u8) {
        match self.queue.detach_for_priority(token, priority).unwrap() {
            Detach::Unchanged => println!("same {token} {:?}", self.order()),
            Detach::Detached => {
                // Simulates extracted native map node: no ordered lookup can
                // find the request until native release/reassignment finishes.
                self.requests.remove(&token);
                self.release_removed(token);
                self.requests.insert(token, priority);
                let result = self
                    .queue
                    .finish_priority(token, self.assigned_jobs.len(), self.unassigned.len())
                    .unwrap();
                let action = self.apply(token, result.action);
                println!("insert {token} {action} {:?}", self.order());
            }
        }
    }
    fn remove_assigned_job(&mut self, token: Token) {
        self.assigned_jobs.remove(&token).unwrap();
        let action =
            self.queue.select_hole(token, self.assigned_jobs.len(), self.unassigned.len()).unwrap();
        let action = self.apply(token, action);
        println!("job-remove {token} {action} {:?}", self.order());
    }
}
fn main() {
    let mut h = Harness::new();
    let a = h.create(2, true);
    h.create(2, true);
    h.create(2, true);
    let d = h.create(4, true);
    h.reprioritize(d, 2);
    h.reprioritize(a, 2);
    h.remove(a);
    h.create(5, false);
    let f = h.create(5, false);
    h.remove_assigned_job(f);
    h.reprioritize(f, 5);
}
