// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]
// Native flat_set/key object lifetime remains a provider. All collection policy,
// short circuit, key-bit selection and output iteration lives in this module.
pub fn is_empty(specific: bool, empty: bool) -> bool {
    specific && empty
}
pub fn contains_all(specific: bool) -> bool {
    !specific
}
pub trait Keys {
    fn len(&mut self, right: bool) -> usize;
    fn compare_query(&mut self, index: usize) -> i32;
    fn equal_at(&mut self, index: usize) -> bool;
}
pub fn contains(specific: bool, keys: &mut impl Keys) -> bool {
    if !specific {
        return true;
    }
    let length = keys.len(false);
    let (mut first, mut count) = (0, length);
    while count != 0 {
        let step = count / 2;
        let middle = first + step;
        if keys.compare_query(middle) < 0 {
            first = middle + 1;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    first != length && keys.compare_query(first) == 0
}
pub fn matches_site(mut add: impl FnMut(i32)) {
    add(0);
    add(1);
}
pub fn equals(left_specific: bool, right_specific: bool, keys: &mut impl Keys) -> bool {
    if left_specific != right_specific {
        return false;
    }
    if !left_specific {
        return true;
    }
    let length = keys.len(false);
    length == keys.len(true) && (0..length).all(|index| keys.equal_at(index))
}
pub trait Stream {
    fn contains_all(&mut self) -> bool;
    fn begin(&mut self);
    fn next(&mut self) -> bool;
    fn key(&mut self);
    fn text(&mut self, text: &'static str);
}
pub fn stream(output: &mut impl Stream) {
    if output.contains_all() {
        output.text("(all keys)");
        return;
    }
    output.text("{");
    output.begin();
    let mut first = true;
    while output.next() {
        if !first {
            output.text(", ");
        }
        output.key();
        first = false;
    }
    output.text("}");
}
#[cfg(test)]
mod tests {
    use super::*;
    struct Unavailable;
    impl Keys for Unavailable {
        fn len(&mut self, _: bool) -> usize {
            panic!("universal has no native set")
        }
        fn compare_query(&mut self, _: usize) -> i32 {
            panic!("universal has no key")
        }
        fn equal_at(&mut self, _: usize) -> bool {
            panic!("universal has no key")
        }
    }
    #[test]
    fn universal_membership_does_not_read_unavailable_set() {
        assert!(contains(false,&mut Unavailable));
        assert!(!is_empty(false,true));
        assert!(contains_all(false));
        assert!(equals(false,false,&mut Unavailable));
    }
    #[test]
    fn matches_site_keeps_both_ancestor_bits() {
        let mut bits = Vec::new();
        matches_site(|bit| bits.push(bit));
        assert_eq!(bits,[0,1]);
    }
}
