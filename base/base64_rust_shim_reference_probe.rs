// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use base64::Engine as _;

#[cxx::bridge(namespace = "base::internal")]
mod ffi {
    extern "Rust" {
        fn base64_simd_encoded_length(length: usize) -> usize;
        fn base64_simd_encode(input: &[u8], output: &mut [u8]) -> usize;
    }
}

fn base64_simd_encoded_length(length: usize) -> usize {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    base64::encoded_len(length, /* padding= */ true).unwrap()
}

#[cfg(any(target_arch = "x86_64", all(target_arch = "aarch64", target_feature = "neon")))]
fn base64_simd_encode(input: &[u8], output: &mut [u8]) -> usize {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    use base64::engine::{general_purpose::PAD, Simd};
    use std::sync::LazyLock;

    static ENGINE: LazyLock<Simd> = LazyLock::new(|| Simd::standard(PAD));
    ENGINE.encode_slice(input, output).unwrap()
}

#[cfg(not(any(target_arch = "x86_64", all(target_arch = "aarch64", target_feature = "neon"))))]
fn base64_simd_encode(input: &[u8], output: &mut [u8]) -> usize {
    #[cfg(rustium_reference_probe)]
    rustium_reference_probe::hit();
    use base64::engine::general_purpose::STANDARD;

    STANDARD.encode_slice(input, output).unwrap()
}

#[cfg(rustium_reference_probe)]
mod rustium_reference_probe {
    use std::sync::atomic::{AtomicU64, Ordering};
    static COUNT: AtomicU64 = AtomicU64::new(0);
    pub(super) fn hit() {
        COUNT.fetch_add(1, Ordering::Relaxed);
    }
    #[unsafe(no_mangle)]
    pub extern "C" fn rustium_probe_base64_count() -> u64 {
        COUNT.load(Ordering::Relaxed)
    }
}
