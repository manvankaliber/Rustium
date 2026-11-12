// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]
#[path = "http_status_code_generated.rs"]
mod http_status_code_generated;
pub use http_status_code_generated::reason;
#[cfg(test)]
mod tests {
    #[test]
    fn boundaries() {
        assert_eq!(super::reason(200), Some("OK"));
        assert_eq!(super::reason(600), Some("HTTP Status Code Max"));
        for code in [i32::MIN, -1, 0, 99, 104, 199, 306, 599, 601, i32::MAX] {
            assert_eq!(super::reason(code), None);
        }
    }
}
