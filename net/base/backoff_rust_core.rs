// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

#[derive(Clone, Copy, Debug)]
pub struct Policy {
    pub ignore: i32,
    pub initial_ms: i64,
    pub factor: f64,
    pub jitter: f64,
    pub maximum_ms: i64,
    pub lifetime_ms: i64,
    pub always_initial: bool,
}

// The implementation supplies Chromium's exact primitive TimeDelta/TimeTicks
// operations, clock, random generator, and platform pow; no policy callback.
pub trait Services {
    fn now(&mut self) -> i64;
    fn random(&mut self) -> f64;
    fn pow(&mut self, base: f64, exponent: i32) -> f64;
    fn add_time_delta(&self, tick: i64, delta_us: i64) -> i64;
    fn subtract_times(&self, left: i64, right: i64) -> i64;
    fn milliseconds(&self, value: i64) -> i64;
    fn in_milliseconds(&self, value: i64) -> i64;
    fn checked_delay_us(&self, value: f64) -> i64;
    fn current_limits(&self) -> (i64, i64);
    fn publish_failures(&mut self, failures: i32);
    fn current_entry(&self) -> Entry;
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Entry {
    pub failures: i32,
    pub release: i64,
}

impl Entry {
    pub fn reset(&mut self) {
        *self = Self::default();
    }
    pub fn custom_release(&mut self, release: i64) {
        self.release = release;
    }
    pub fn reject<S: Services>(&self, service: &mut S) -> bool {
        let now = service.now();
        service.current_entry().release > now
    }
    pub fn until_release<S: Services>(&self, service: &mut S) -> i64 {
        let now = service.now();
        let release = service.current_entry().release;
        if release <= now {
            0
        } else {
            service.subtract_times(release, now)
        }
    }
    pub fn discard<S: Services>(&self, policy: &Policy, service: &mut S) -> bool {
        if policy.lifetime_ms == -1 {
            return false;
        }
        let now = service.now();
        let current = service.current_entry();
        let unused = service.in_milliseconds(service.subtract_times(now, current.release));
        if unused < 0 {
            return false;
        }
        let (maximum, lifetime) = service.current_limits();
        unused >= if current.failures > 0 { maximum.max(lifetime) } else { lifetime }
    }
    pub fn release_for_duration<S: Services>(
        duration: i64,
        _policy: &Policy,
        service: &mut S,
    ) -> i64 {
        let now = service.now();
        let calculated = duration.checked_add(now).unwrap_or(i64::MAX);
        let (maximum_ms, _) = service.current_limits();
        let maximum = if maximum_ms >= 0 {
            maximum_ms.checked_mul(1000).and_then(|v| v.checked_add(now)).unwrap_or(i64::MAX)
        } else {
            i64::MAX
        };
        service.add_time_delta(0, calculated.min(maximum))
    }
    pub fn inform<S: Services>(&mut self, succeeded: bool, policy: &Policy, service: &mut S) {
        if succeeded {
            if self.failures > 0 {
                self.failures -= 1;
            }
            service.publish_failures(self.failures);
            let delay =
                if policy.always_initial { service.milliseconds(policy.initial_ms) } else { 0 };
            let now = service.now();
            let current = service.current_entry();
            self.failures = current.failures;
            self.release = service.add_time_delta(now, delay).max(current.release);
        } else {
            // Overflow of ++failure_count_ is undefined in the C++ source.
            self.failures =
                self.failures.checked_add(1).expect("undefined C++ failure-count overflow");
            service.publish_failures(self.failures);
            let mut effective = self.failures.saturating_sub(policy.ignore).max(0);
            if policy.always_initial {
                effective = effective.saturating_add(1);
            }
            if effective == 0 {
                let now = service.now();
                let current = service.current_entry();
                self.failures = current.failures;
                self.release = now.max(current.release);
                return;
            }
            let mut delay = policy.initial_ms as f64;
            delay *= service.pow(policy.factor, effective - 1);
            delay -= service.random() * policy.jitter * delay;
            let rounded = delay + 0.5;
            let duration = service.checked_delay_us(rounded);
            let release = Self::release_for_duration(duration, policy, service);
            let current = service.current_entry();
            self.failures = current.failures;
            self.release = release.max(current.release);
        }
    }
}

// Values from base::Time/TimeDelta use the two extrema as infinity sentinels.
fn finite(v: i64) -> bool {
    v != i64::MIN && v != i64::MAX
}

pub fn serialize_times<S: SerializerServices>(
    release: i64,
    ticks_now: i64,
    wall_now: i64,
    service: &S,
) -> (i64, i64) {
    let mut duration = 0;
    if finite(release) && finite(ticks_now) {
        duration = service.delta_subtract(release, ticks_now);
    }
    if !finite(duration) {
        duration = 0;
    }
    let mut absolute = service.wall_add(wall_now, duration);
    if !finite(absolute) {
        absolute = 0;
    }
    (duration, absolute)
}
pub trait SerializerServices {
    fn len(&self) -> usize;
    fn kind(&self, index: usize) -> u8;
    fn integer(&self, index: usize) -> i32;
    fn double_seconds(&self, index: usize) -> i64;
    fn parse_i64(&self, index: usize) -> Option<i64>;
    fn delta_subtract(&self, left: i64, right: i64) -> i64;
    fn wall_add(&self, wall: i64, delta: i64) -> i64;
}
pub struct Decoded {
    pub failures: i32,
    pub duration: i64,
    pub absolute: i64,
}
pub fn decode<S: SerializerServices>(s: &S) -> Option<Decoded> {
    if s.len() != 4 || s.kind(0) != 1 {
        return None;
    }
    let version = s.integer(0);
    if version != 1 && version != 2 {
        return None;
    }
    if s.kind(1) != 1 {
        return None;
    }
    let failures = s.integer(1);
    if failures < 0 {
        return None;
    }
    let failures = failures.min(4389);
    let duration = if version == 1 {
        if s.kind(2) != 2 {
            return None;
        }
        s.double_seconds(2)
    } else {
        if s.kind(2) != 3 {
            return None;
        }
        s.parse_i64(2)?
    };
    if s.kind(3) != 3 {
        return None;
    }
    let absolute = s.parse_i64(3)?;
    Some(Decoded { failures, duration, absolute })
}
// Called only after the failure replay, exactly as in the original.
pub fn restore_duration<S: SerializerServices>(
    original: i64,
    absolute: i64,
    wall_now: i64,
    s: &S,
) -> Option<i64> {
    let duration = if absolute == 0 {
        original
    } else {
        if !finite(absolute) || !finite(wall_now) {
            return None;
        }
        s.delta_subtract(absolute, wall_now).min(original)
    };
    if finite(duration) {
        Some(duration)
    } else {
        None
    }
}
pub fn finite_release(release: i64) -> bool {
    finite(release)
}
