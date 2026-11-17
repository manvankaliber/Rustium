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
