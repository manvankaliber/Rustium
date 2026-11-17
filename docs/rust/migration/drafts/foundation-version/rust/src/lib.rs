//! Safe, byte-oriented candidate for Chromium base::Version behavior.
//! Input is parsed as bytes, matching std::string_view; no UTF-8 assumption.
use std::cmp::Ordering;

#[derive(Clone, Debug, Default)]
pub struct Version {
    components: Option<Vec<u32>>,
}

impl Version {
    pub fn invalid() -> Self {
        Self { components: None }
    }
    pub fn from_components(components: Vec<u32>) -> Self {
        if components.is_empty() {
            Self::invalid()
        } else {
            Self { components: Some(components) }
        }
    }
    pub fn parse(input: &[u8]) -> Self {
        if input.is_empty() {
            return Self::invalid();
        }
        let mut out = Vec::new();
        for (i, part) in input.split(|b| *b == b'.').enumerate() {
            if part.is_empty() || part.iter().any(|b| !b.is_ascii_digit()) {
                return Self::invalid();
            }
            let mut n: u32 = 0;
            for &b in part {
                n = match n.checked_mul(10).and_then(|v| v.checked_add((b - b'0') as u32)) {
                    Some(v) => v,
                    None => return Self::invalid(),
                };
            }
            if i == 0 && part.len() > 1 && part[0] == b'0' {
                return Self::invalid();
            }
            out.push(n);
        }
        Self::from_components(out)
    }
    pub fn is_valid(&self) -> bool {
        self.components.is_some()
    }
    pub fn components(&self) -> Option<&[u32]> {
        self.components.as_deref()
    }
    pub fn to_string(&self) -> String {
        match &self.components {
            None => "invalid".to_owned(),
            Some(v) => v.iter().map(u32::to_string).collect::<Vec<_>>().join("."),
        }
    }
    pub fn compare_to(&self, other: &Self) -> Option<Ordering> {
        Some(compare_components(self.components.as_deref()?, other.components.as_deref()?))
    }
    pub fn equals_by_compare_to(&self, other: &Self) -> bool {
        self.compare_to(other) == Some(Ordering::Equal)
    }
    pub fn is_valid_wildcard(input: &[u8]) -> bool {
        let prefix = input.strip_suffix(b".*").unwrap_or(input);
        Self::parse(prefix).is_valid()
    }
    pub fn compare_to_wildcard(&self, input: &[u8]) -> Option<Ordering> {
        let mine = self.components.as_deref()?;
        if !Self::is_valid_wildcard(input) {
            return None;
        }
        let Some(prefix) = input.strip_suffix(b".*") else {
            return self.compare_to(&Self::parse(input));
        };
        let wildcard = Self::parse(prefix).components?;
        let cmp = compare_components(mine, &wildcard);
        if cmp != Ordering::Greater {
            return Some(cmp);
        }
        let n = mine.len().min(wildcard.len());
        if mine[..n] == wildcard[..n] {
            Some(Ordering::Equal)
        } else {
            Some(Ordering::Greater)
        }
    }
}
fn compare_components(a: &[u32], b: &[u32]) -> Ordering {
    for i in 0..a.len().min(b.len()) {
        match a[i].cmp(&b[i]) {
            Ordering::Equal => (),
            x => return x,
        }
    }
    if a.len() > b.len() {
        if a[b.len()..].iter().any(|x| *x != 0) {
            return Ordering::Greater;
        }
    }
    if b.len() > a.len() {
        if b[a.len()..].iter().any(|x| *x != 0) {
            return Ordering::Less;
        }
    }
    Ordering::Equal
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn original_parse_and_serialization_fixtures() {
        let cases: &[(&[u8], bool, &str)] = &[
            (b"", false, "invalid"),
            (b" ", false, "invalid"),
            (b".", false, "invalid"),
            (b"0", true, "0"),
            (b"0.", false, "invalid"),
            (b"0.0", true, "0.0"),
            (b"4294967295.0", true, "4294967295.0"),
            (b"4294967296.0", false, "invalid"),
            (b"-1.0", false, "invalid"),
            (b"+1.0", false, "invalid"),
            (b"1.0a", false, "invalid"),
            (b"02.1", false, "invalid"),
            (b"0.01", true, "0.1"),
            (b"15.007.20011", true, "15.7.20011"),
            (b"1.*.3", false, "invalid"),
        ];
        for (raw, valid, rendered) in cases {
            let v = Version::parse(raw);
            assert_eq!(v.is_valid(), *valid, "{:?}", raw);
            assert_eq!(v.to_string(), *rendered, "{:?}", raw);
        }
        assert!(!Version::parse(&[b'1', b'.', 0xff]).is_valid());
    }
    #[test]
    fn compare_padded_zeros_and_wildcards() {
        assert_eq!(
            Version::parse(b"1.0.0").compare_to(&Version::parse(b"1")),
            Some(Ordering::Equal)
        );
        assert_eq!(Version::parse(b"1.2.3").compare_to_wildcard(b"1.2.*"), Some(Ordering::Equal));
        assert_eq!(Version::parse(b"1.2.0.0").compare_to_wildcard(b"1.2.*"), Some(Ordering::Equal));
        assert_eq!(Version::parse(b"1.3").compare_to_wildcard(b"1.2.*"), Some(Ordering::Greater));
        assert_eq!(Version::parse(b"1.2").compare_to_wildcard(b"1.3.*"), Some(Ordering::Less));
        for bad in [&b"*"[..], b"1.*.3", b"1.2.3*", b"+2.*"] {
            assert!(!Version::is_valid_wildcard(bad));
        }
    }
    #[test]
    fn copy_move_boundary_and_invalid_state() {
        let source = Version::parse(b"4294967295.0");
        let copied = source.clone();
        assert!(source.equals_by_compare_to(&copied));
        let moved = copied;
        assert_eq!(moved.to_string(), "4294967295.0");
        assert_eq!(Version::default().to_string(), "invalid");
        assert!(!Version::from_components(vec![]).is_valid());
    }
}
#[cfg(test)]
mod tests {
    use super::*;

    // Transcribed row-for-row from reference-cpp/base/version_unittest.cc.
    // The C++ assertion/test name is retained in each test name/message.
    #[test]
    fn original_default_constructor() {
        assert!(!Version::default().is_valid(), "VersionTest.DefaultConstructor EXPECT_FALSE");
    }

    #[test]
    fn original_value_and_move_semantics() {
        let v1 = Version::parse(b"1.2.3.4");
        assert!(v1.is_valid(), "VersionTest.ValueSemantics");
        let invalid = Version::default();
        assert!(!invalid.is_valid(), "VersionTest.ValueSemantics");
        let v2 = v1.clone();
        let v3 = v2.clone();
        assert!(v2.is_valid(), "VersionTest.ValueSemantics");
        assert!(v1.equals_by_compare_to(&v2), "VersionTest.ValueSemantics");
        assert!(v3.equals_by_compare_to(&v1), "VersionTest.ValueSemantics");
        let moved = Version::from_components(vec![1, 2, 3, 4]);
        assert!(moved.is_valid(), "VersionTest.MoveSemantics");
        assert!(moved.equals_by_compare_to(&v1), "VersionTest.MoveSemantics");
    }

    #[test]
    fn original_get_version_from_string_every_row() {
        struct Case { input: &'static [u8], parts: usize, first: u32, success: bool }
        let cases = [
            Case { input: b"", parts: 0, first: 0, success: false },
            Case { input: b" ", parts: 0, first: 0, success: false },
            Case { input: b"\t", parts: 0, first: 0, success: false },
            Case { input: b"\n", parts: 0, first: 0, success: false },
            Case { input: b"  ", parts: 0, first: 0, success: false },
            Case { input: b".", parts: 0, first: 0, success: false },
            Case { input: b" . ", parts: 0, first: 0, success: false },
            Case { input: b"0", parts: 1, first: 0, success: true },
            Case { input: b"0.", parts: 0, first: 0, success: false },
            Case { input: b"0.0", parts: 2, first: 0, success: true },
            Case { input: b"4294967295.0", parts: 2, first: 4294967295, success: true },
            Case { input: b"4294967296.0", parts: 0, first: 0, success: false },
            Case { input: b"-1.0", parts: 0, first: 0, success: false },
            Case { input: b"1.-1.0", parts: 0, first: 0, success: false },
            Case { input: b"1,--1.0", parts: 0, first: 0, success: false },
            Case { input: b"+1.0", parts: 0, first: 0, success: false },
            Case { input: b"1.+1.0", parts: 0, first: 0, success: false },
            Case { input: b"1+1.0", parts: 0, first: 0, success: false },
            Case { input: b"++1.0", parts: 0, first: 0, success: false },
            Case { input: b"1.0a", parts: 0, first: 0, success: false },
            Case { input: b"1.2.3.4.5.6.7.8.9.0", parts: 10, first: 1, success: true },
            Case { input: b"02.1", parts: 0, first: 0, success: false },
            Case { input: b"0.01", parts: 2, first: 0, success: true },
            Case { input: b"f.1", parts: 0, first: 0, success: false },
            Case { input: b"15.007.20011", parts: 3, first: 15, success: true },
            Case { input: b"15.5.28.130162", parts: 4, first: 15, success: true },
        ];
        for (row, c) in cases.iter().enumerate() {
            let v = Version::parse(c.input);
            assert_eq!(v.is_valid(), c.success, "GetVersionFromString row {row}: {:?}", c.input);
            let actual = v.components().unwrap_or(&[]);
            assert_eq!(actual.len(), c.parts, "GetVersionFromString row {row}: parts");
            if c.success { assert_eq!(actual[0], c.first, "GetVersionFromString row {row}: firstpart"); }
        }
    }

    #[test]
    fn original_compare_rows_and_all_relational_operators() {
        struct Case { lhs: &'static [u8], rhs: &'static [u8], expected: i8 }
        let cases = [
            Case { lhs: b"1.0", rhs: b"1.0", expected: 0 }, Case { lhs: b"1.0", rhs: b"0.0", expected: 1 },
            Case { lhs: b"1.0", rhs: b"2.0", expected: -1 }, Case { lhs: b"1.0", rhs: b"1.1", expected: -1 },
            Case { lhs: b"1.1", rhs: b"1.0", expected: 1 }, Case { lhs: b"1.0", rhs: b"1.0.1", expected: -1 },
            Case { lhs: b"1.1", rhs: b"1.0.1", expected: 1 }, Case { lhs: b"1.1", rhs: b"1.0.1", expected: 1 },
            Case { lhs: b"1.0.0", rhs: b"1.0", expected: 0 }, Case { lhs: b"1.0.3", rhs: b"1.0.20", expected: -1 },
            Case { lhs: b"11.0.10", rhs: b"15.007.20011", expected: -1 }, Case { lhs: b"11.0.10", rhs: b"15.5.28.130162", expected: -1 },
            Case { lhs: b"15.5.28.130162", rhs: b"15.5.28.130162", expected: 0 },
        ];
        for (row, c) in cases.iter().enumerate() {
            let lhs = Version::parse(c.lhs); let rhs = Version::parse(c.rhs);
            let ord = match c.expected { -1 => Ordering::Less, 0 => Ordering::Equal, _ => Ordering::Greater };
            assert_eq!(lhs.compare_to(&rhs), Some(ord), "Compare row {row}");
            assert_eq!(lhs.compare_to_wildcard(c.rhs), Some(ord), "Compare wildcard RHS row {row}");
            assert_eq!(rhs.compare_to_wildcard(c.lhs), Some(ord.reverse()), "Compare wildcard LHS row {row}");
            assert_eq!(ord == Ordering::Less, lhs.to_string() != rhs.to_string() && lhs.compare_to(&rhs) == Some(Ordering::Less), "operator < row {row}");
            assert_eq!(ord != Ordering::Greater, lhs.compare_to(&rhs) != Some(Ordering::Greater), "operator <= row {row}");
            assert_eq!(ord != Ordering::Equal, !lhs.equals_by_compare_to(&rhs), "operator != row {row}");
            assert_eq!(ord == Ordering::Equal, lhs.equals_by_compare_to(&rhs), "operator == row {row}");
            assert_eq!(ord != Ordering::Less, lhs.compare_to(&rhs) != Some(Ordering::Less), "operator >= row {row}");
            assert_eq!(ord == Ordering::Greater, lhs.compare_to(&rhs) == Some(Ordering::Greater), "operator > row {row}");
        }
    }
}
