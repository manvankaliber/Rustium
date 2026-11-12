// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]
#[path = "http_status_code_generated.rs"]
mod http_status_code_generated;
pub use http_status_code_generated::{known, reason_record};
#[cfg(test)]
mod tests {
    #[test]
    fn boundaries() {
        assert_eq!(super::reason_record(200), Some(b"\x02OK\0".as_slice()));
        assert_eq!(super::reason_record(600), Some(b"\x14HTTP Status Code Max\0".as_slice()));
        assert!(super::known(200));
        assert!(super::known(600));
        for code in [i32::MIN, -1, 0, 99, 104, 199, 306, 599, 601, i32::MAX] {
            assert_eq!(super::reason_record(code), None);
            assert!(!super::known(code));
        }
    }

    #[test]
    fn record_schema_and_boolean_lookup_agree() {
        for code in -65536..=65536 {
            let record = super::reason_record(code);
            assert_eq!(super::known(code), record.is_some());
            if let Some(record) = record {
                assert_eq!(record.len(), usize::from(record[0]) + 2);
                assert_eq!(record.last(), Some(&0));
            }
        }
    }
}
