// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]
#[path = "proxy_value_rust_core.rs"]
#[allow(dead_code)] // Only allocation-size and raw-value boundaries run here.
mod core;

#[test]
fn serialization_length_boundaries_without_gigabyte_allocations() {
    assert_eq!(core::persisted_length(0), 0);
    assert_eq!(core::persisted_length(i32::MAX as usize - 1), i32::MAX - 1);
    assert_eq!(core::persisted_length(i32::MAX as usize), 0);
    assert_eq!(core::persisted_length(usize::MAX), 0);
}
#[test]
fn formatting_raw_values_retains_nul_and_port_first_order() {
    assert_eq!(core::host_for_url(b"a\0b"), b"a\0b");
    assert_eq!(core::host_for_url(b"a\0:b"), b"[a]");
    let a = core::Server { scheme: core::HTTP, host: b"z".to_vec(), port: 0 };
    let b = core::Server { scheme: core::HTTP, host: b"a".to_vec(), port: 1 };
    assert_eq!(core::compare_server(&a, &b), std::cmp::Ordering::Less);
    assert_eq!(core::format_server(&core::Server::default(), false), b"");
    assert!(!core::scheme_flag(core::HTTP, 255));
    assert_eq!(core::default_port(-1), -1);
}
