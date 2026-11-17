//! Candidate safe Rust ownership model for Chromium `media::Ranges<T>`.
//!
//! This module is intentionally standalone: the production C++ API, exports,
//! build wiring, and ABI remain director-owned. `T` must provide a total order;
//! this deliberately does not choose a NaN policy for floating-point endpoints.

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct RangeSet<T> {
    ranges: Vec<(T, T)>,
}

impl<T: Ord + Clone> RangeSet<T> {
    pub fn new() -> Self { Self { ranges: Vec::new() } }
    pub fn len(&self) -> usize { self.ranges.len() }
    pub fn is_empty(&self) -> bool { self.ranges.is_empty() }
    pub fn clear(&mut self) { self.ranges.clear(); }
    pub fn start(&self, i: usize) -> &T { &self.ranges[i].0 }
    pub fn end(&self, i: usize) -> &T { &self.ranges[i].1 }
    pub fn back(&self) -> Option<&(T, T)> { self.ranges.last() }
    pub fn contains(&self, i: usize, value: &T) -> bool {
        self.ranges[i].0 <= *value && *value < self.ranges[i].1
    }

    /// Insert [start,end), coalescing overlap and adjacency; returns new count.
    /// Equal endpoints are ignored before the debug-only reversed-range check,
    /// matching the native implementation's contract.
    pub fn add(&mut self, start: T, end: T) -> usize {
        if start == end { return self.ranges.len(); }
        debug_assert!(start < end, "range start must be less than end");
        let mut i = 0;
        while i < self.ranges.len() && self.ranges[i].1 < start { i += 1; }
        if i == self.ranges.len() {
            self.ranges.push((start, end));
            return self.ranges.len();
        }
        if end < self.ranges[i].0 {
            self.ranges.insert(i, (start, end));
            return self.ranges.len();
        }
        if start < self.ranges[i].0 { self.ranges[i].0 = start; }
        if self.ranges[i].1 < end { self.ranges[i].1 = end; }
        while i + 1 < self.ranges.len() && self.ranges[i + 1].0 <= self.ranges[i].1 {
            let next_end = self.ranges[i + 1].1.clone();
            if self.ranges[i].1 < next_end { self.ranges[i].1 = next_end; }
            self.ranges.remove(i + 1);
        }
        self.ranges.len()
    }

    pub fn intersection(&self, other: &Self) -> Self {
        let mut result = Self::new();
        let (mut i, mut j) = (0, 0);
        while i < self.len() && j < other.len() {
            let start = std::cmp::max(self.ranges[i].0.clone(), other.ranges[j].0.clone());
            let end = std::cmp::min(self.ranges[i].1.clone(), other.ranges[j].1.clone());
            if start < end { result.add(start, end); }
            if self.ranges[i].1 < other.ranges[j].1 { i += 1; } else { j += 1; }
        }
        result
    }
}

/// Chromium base::TimeDelta stores signed microseconds; this transparent newtype
/// preserves every int64 bit pattern, including native sentinel extrema.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct TimeDeltaMicros(pub i64);

impl TimeDeltaMicros {
    pub const fn from_internal_value(micros: i64) -> Self { Self(micros) }
    pub const fn to_internal_value(self) -> i64 { self.0 }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn representative_sequences_and_edges() {
        let mut r = RangeSet::new();
        assert_eq!(r.add(0i64, 1), 1);
        assert_eq!(r.add(2, 3), 2);
        assert_eq!(r.add(1, 2), 1);
        assert_eq!(r.ranges, vec![(0, 3)]);
        assert_eq!(r.add(3, 3), 1); // equal/empty ignored
        assert_eq!(r.add(i64::MIN, i64::MIN + 1), 2);
        assert_eq!(r.add(i64::MAX - 1, i64::MAX), 3);
        assert_eq!(r.ranges, vec![(i64::MIN, i64::MIN + 1), (0, 3), (i64::MAX - 1, i64::MAX)]);
        assert!(r.contains(1, &0));
        assert!(!r.contains(1, &3));
    }
    #[test]
    fn intersection_half_open_and_time_conversion() {
        let mut a = RangeSet::new();
        let mut b = RangeSet::new();
        for (s,e) in [(0,1),(4,7),(10,12)] { a.add(s, e); }
        for (s,e) in [(0,3),(5,11)] { b.add(s, e); }
        assert_eq!(a.intersection(&b).ranges, vec![(0,1),(5,7),(10,11)]);
        let t = TimeDeltaMicros::from_internal_value(i64::MIN);
        assert_eq!(t.to_internal_value(), i64::MIN);
    }
}
