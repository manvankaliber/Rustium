//! Safe Rust candidate mirroring media/base/ranges.h at the frozen base commit.
//! Comparison operators intentionally use PartialOrd directly: finite f64 values
//! follow C++ double comparison semantics. NaN is unordered in both languages;
//! no total order or NaN insertion policy is introduced here.

#[derive(Clone, Debug, PartialEq)]
pub struct RangeSet<T> {
    ranges: Vec<(T, T)>,
}
impl<T> Default for RangeSet<T> {
    fn default() -> Self {
        Self { ranges: Vec::new() }
    }
}
impl<T: PartialOrd + PartialEq + Clone> RangeSet<T> {
    pub fn new() -> Self {
        Self { ranges: Vec::new() }
    }
    pub fn len(&self) -> usize {
        self.ranges.len()
    }
    pub fn is_empty(&self) -> bool {
        self.ranges.is_empty()
    }
    pub fn clear(&mut self) {
        self.ranges.clear();
    }
    pub fn start(&self, i: usize) -> &T {
        &self.ranges[i].0
    }
    pub fn end(&self, i: usize) -> &T {
        &self.ranges[i].1
    }
    pub fn back(&self) -> Option<&(T, T)> {
        self.ranges.last()
    }
    pub fn contains(&self, i: usize, entry: &T) -> bool {
        self.ranges[i].0 <= *entry && self.ranges[i].1 > *entry
    }
    /// Mirrors native Add's operation and comparison order. Equal endpoints
    /// return before DCHECK; reversed endpoints trigger the debug precondition.
    pub fn add(&mut self, start: T, end: T) -> usize {
        if start == end {
            return self.ranges.len();
        }
        debug_assert!(start < end, "DCHECK_LT(start, end)");
        let mut i = 0;
        while i < self.ranges.len() && self.ranges[i].1 < start {
            i += 1;
        }
        if i == self.ranges.len() {
            self.ranges.push((start, end));
            return self.ranges.len();
        }
        if end < self.ranges[i].0 {
            self.ranges.insert(i, (start, end));
            return self.ranges.len();
        }
        if start < self.ranges[i].0 {
            self.ranges[i].0 = start;
        }
        if self.ranges[i].1 < end {
            self.ranges[i].1 = end;
        }
        while i + 1 < self.ranges.len() && self.ranges[i + 1].0 <= self.ranges[i].1 {
            // C++ std::max(a,b) returns a unless a < b; preserve that order.
            if self.ranges[i].1 < self.ranges[i + 1].1 {
                self.ranges[i].1 = self.ranges[i + 1].1.clone();
            }
            self.ranges.remove(i + 1);
        }
        self.ranges.len()
    }
    pub fn intersection(&self, other: &Self) -> Self {
        let mut result = Self::new();
        let (mut i, mut j) = (0, 0);
        while i < self.len() && j < other.len() {
            // Exact native std::max(start(i), other.start(j)) and std::min
            // (end(i), other.end(j)) comparison direction.
            let s = if self.ranges[i].0 < other.ranges[j].0 {
                other.ranges[j].0.clone()
            } else {
                self.ranges[i].0.clone()
            };
            let e = if other.ranges[j].1 < self.ranges[i].1 {
                other.ranges[j].1.clone()
            } else {
                self.ranges[i].1.clone()
            };
            if s < e {
                result.add(s, e);
            }
            if self.ranges[i].1 < other.ranges[j].1 {
                i += 1;
            } else {
                j += 1;
            }
        }
        result
    }
}

/// Exact storage-unit adapter: Chromium TimeDelta internal values are signed
/// microseconds. This does not add arithmetic or alter sentinel/extrema values.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct TimeDeltaMicros(pub i64);
impl TimeDeltaMicros {
    pub const fn from_internal_value(v: i64) -> Self {
        Self(v)
    }
    pub const fn to_internal_value(self) -> i64 {
        self.0
    }
    /// Unconnected diagnostic adapter proposal. Generic add uses Rust's debug
    /// assertion; the production bridge must retain native DCHECK configuration
    /// and the TimeDelta signed-microsecond diagnostic. This is not that wiring.
    pub fn debug_check_lt(lhs: Self, rhs: Self) {
        debug_assert!(lhs.0 < rhs.0, "{} < {}", lhs.0, rhs.0);
    }
}

#[cfg(test)]
mod public_api_tests {
    use super::*;
    #[test]
    fn accessors_clone_and_time_delta_endpoints() {
        let mut r: RangeSet<TimeDeltaMicros> = Default::default();
        assert!(r.is_empty());
        assert_eq!(r.back(), None);
        r.add(TimeDeltaMicros(i64::MIN), TimeDeltaMicros(-1));
        r.add(TimeDeltaMicros(0), TimeDeltaMicros(i64::MAX));
        assert_eq!(r.len(), 2);
        assert_eq!(*r.start(0), TimeDeltaMicros(i64::MIN));
        assert_eq!(*r.end(0), TimeDeltaMicros(-1));
        assert!(r.contains(0, &TimeDeltaMicros(i64::MIN)));
        assert!(!r.contains(0, &TimeDeltaMicros(-1)));
        assert_eq!(r.back(), Some(&(TimeDeltaMicros(0), TimeDeltaMicros(i64::MAX))));
        let mut copied = r.clone();
        copied.clear();
        assert!(copied.is_empty());
        assert_eq!(r.len(), 2);
        let mut other = RangeSet::new();
        other.add(TimeDeltaMicros(-2), TimeDeltaMicros(1));
        let intersection = r.intersection(&other);
        assert_eq!(intersection.len(), 2);
        assert_eq!(
            (*intersection.start(0), *intersection.end(0)),
            (TimeDeltaMicros(-2), TimeDeltaMicros(-1))
        );
        assert_eq!(
            (*intersection.start(1), *intersection.end(1)),
            (TimeDeltaMicros(0), TimeDeltaMicros(1))
        );
        for value in [i64::MIN, -1, 0, 1, i64::MAX] {
            assert_eq!(TimeDeltaMicros::from_internal_value(value).to_internal_value(), value);
        }
        TimeDeltaMicros::debug_check_lt(TimeDeltaMicros(i64::MIN), TimeDeltaMicros(i64::MAX));
    }
    #[test]
    fn signed_zero_intersection_ties_preserve_first_operand_bits() {
        let mut negative = RangeSet::<f64>::new();
        negative.add(-0.0, 1.0);
        let mut positive = RangeSet::<f64>::new();
        positive.add(0.0, 1.0);
        assert_eq!(negative.intersection(&positive).start(0).to_bits(), (-0.0_f64).to_bits());
        assert_eq!(positive.intersection(&negative).start(0).to_bits(), 0.0_f64.to_bits());
        let mut negative_end = RangeSet::<f64>::new();
        negative_end.add(-1.0, -0.0);
        let mut positive_end = RangeSet::<f64>::new();
        positive_end.add(-1.0, 0.0);
        assert_eq!(negative_end.intersection(&positive_end).end(0).to_bits(), (-0.0_f64).to_bits());
        assert_eq!(positive_end.intersection(&negative_end).end(0).to_bits(), 0.0_f64.to_bits());
    }
    #[cfg(debug_assertions)]
    #[test]
    fn rust_debug_configuration_rejects_reversed_and_unordered_inputs() {
        for (start, end) in [(2.0, 1.0), (f64::NAN, 1.0), (0.0, f64::NAN)] {
            assert!(std::panic::catch_unwind(|| {
                let mut r = RangeSet::new();
                r.add(start, end);
            })
            .is_err());
        }
        assert!(std::panic::catch_unwind(|| TimeDeltaMicros::debug_check_lt(
            TimeDeltaMicros(2),
            TimeDeltaMicros(1)
        ))
        .is_err());
    }
}

#[cfg(test)]
mod original_ranges_unittest {
    use super::*;
    fn eq<T: PartialEq + std::fmt::Debug>(a: T, b: T) {
        assert_eq!(a, b);
    }
    fn r<T: PartialOrd + PartialEq + Clone + std::fmt::Debug>(
        x: &RangeSet<T>,
        expected: &[(T, T)],
    ) {
        eq(x.len(), expected.len());
        for (i, (start, end)) in expected.iter().enumerate() {
            eq(x.start(i), start);
            eq(x.end(i), end);
        }
    }
    #[test]
    fn simple_tests() {
        let mut x = RangeSet::new();
        eq(x.len(), 0);
        eq(x.add(0, 1), 1);
        eq(x.len(), 1);
        r(&x, &[(0, 1)]);
        eq(x.add(2, 3), 2);
        r(&x, &[(0, 1), (2, 3)]);
        eq(x.add(1, 2), 1);
        r(&x, &[(0, 3)]);
        eq(x.add(1, 4), 1);
        r(&x, &[(0, 4)]);
        eq(x.add(7, 9), 2);
        eq(x.add(5, 6), 3);
        r(&x, &[(0, 4), (5, 6), (7, 9)]);
        eq(x.add(6, 7), 2);
        r(&x, &[(0, 4), (5, 9)]);
    }
    #[test]
    fn extend_range_all_original_double_cases() {
        let mut x = RangeSet::new();
        eq(x.add(0.0, 1.0), 1);
        eq(x.add(0.5, 1.5), 1);
        r(&x, &[(0.0, 1.5)]);
        x.clear();
        eq(x.add(0.0, 1.0), 1);
        eq(x.add(-0.5, 0.5), 1);
        r(&x, &[(-0.5, 1.0)]);
        x.clear();
        eq(x.add(0.0, 1.0), 1);
        eq(x.add(2.0, 3.0), 2);
        eq(x.add(4.0, 5.0), 3);
        eq(x.add(0.5, 1.5), 3);
        r(&x, &[(0.0, 1.5), (2.0, 3.0), (4.0, 5.0)]);
        x.clear();
        eq(x.add(0.0, 1.0), 1);
        eq(x.add(2.0, 3.0), 2);
        eq(x.add(4.0, 5.0), 3);
        eq(x.add(1.5, 2.5), 3);
        r(&x, &[(0.0, 1.0), (1.5, 3.0), (4.0, 5.0)]);
    }
    #[test]
    fn coalesce_ranges_all_original_double_cases() {
        let mut x = RangeSet::new();
        eq(x.add(0.0, 1.0), 1);
        eq(x.add(2.0, 3.0), 2);
        eq(x.add(4.0, 5.0), 3);
        eq(x.add(0.5, 2.5), 2);
        r(&x, &[(0.0, 3.0), (4.0, 5.0)]);
        x.clear();
        eq(x.add(0.0, 1.0), 1);
        eq(x.add(2.0, 3.0), 2);
        eq(x.add(4.0, 5.0), 3);
        eq(x.add(0.5, 4.5), 1);
        r(&x, &[(0.0, 5.0)]);
        x.clear();
        eq(x.add(0.0, 1.0), 1);
        eq(x.add(1.0, 2.0), 1);
        r(&x, &[(0.0, 2.0)]);
    }
    #[test]
    fn intersection_with_all_original_assertions_both_directions() {
        let mut a = RangeSet::new();
        let mut b = RangeSet::new();
        eq(a.add(0, 1), 1);
        eq(a.add(4, 7), 2);
        eq(a.add(10, 12), 3);
        r(&a, &[(0, 1), (4, 7), (10, 12)]);
        r(&b, &[]);
        r(&a.intersection(&b), &[]);
        r(&b.intersection(&a), &[]);
        eq(b.add(-1, 13), 1);
        r(&a, &[(0, 1), (4, 7), (10, 12)]);
        r(&b, &[(-1, 13)]);
        r(&a.intersection(&b), &[(0, 1), (4, 7), (10, 12)]);
        r(&b.intersection(&a), &[(0, 1), (4, 7), (10, 12)]);
        b.clear();
        eq(b.add(1, 4), 1);
        eq(b.add(8, 9), 2);
        r(&a, &[(0, 1), (4, 7), (10, 12)]);
        r(&b, &[(1, 4), (8, 9)]);
        r(&a.intersection(&b), &[]);
        r(&b.intersection(&a), &[]);
        b.clear();
        eq(b.add(0, 3), 1);
        eq(b.add(5, 11), 2);
        r(&a, &[(0, 1), (4, 7), (10, 12)]);
        r(&b, &[(0, 3), (5, 11)]);
        r(&a.intersection(&b), &[(0, 1), (5, 7), (10, 11)]);
        r(&b.intersection(&a), &[(0, 1), (5, 7), (10, 11)]);
        b.clear();
        eq(b.add(0, 12), 1);
        r(&a, &[(0, 1), (4, 7), (10, 12)]);
        r(&b, &[(0, 12)]);
        r(&a.intersection(&b), &[(0, 1), (4, 7), (10, 12)]);
        r(&b.intersection(&a), &[(0, 1), (4, 7), (10, 12)]);
    }
    #[test]
    fn domain_edges() {
        let mut x = RangeSet::new();
        eq(x.add(2i64, 2), 0);
        eq(x.add(i64::MIN, i64::MIN + 1), 1);
        eq(x.add(i64::MAX - 1, i64::MAX), 2);
        assert!(x.contains(0, &i64::MIN));
        assert!(!x.contains(0, &(i64::MIN + 1)));
        eq(TimeDeltaMicros::from_internal_value(i64::MIN).to_internal_value(), i64::MIN);
        eq(TimeDeltaMicros::from_internal_value(i64::MAX).to_internal_value(), i64::MAX);
    }
}
