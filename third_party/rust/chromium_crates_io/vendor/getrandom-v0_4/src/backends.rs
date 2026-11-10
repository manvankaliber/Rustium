//! System-specific implementations.
//!
//! This module should provide `fill_inner` with the signature
//! `fn fill_inner(dest: &mut [MaybeUninit<u8>]) -> Result<(), Error>`.
//! The function MUST fully initialize `dest` when `Ok(())` is returned;
//! the function may need to use `sanitizer::unpoison` as well.
//! The function MUST NOT ever write uninitialized bytes into `dest`,
//! regardless of what value it returns.

cfg_if! {
    if #[cfg(getrandom_backend = "custom")] {
        mod custom;
        pub use custom::*;
    } else if #[cfg(getrandom_backend = "linux_getrandom")] {
        mod getrandom;
        pub use getrandom::*;
    } else if #[cfg(getrandom_backend = "linux_raw")] {
        mod linux_raw;
        pub use linux_raw::*;
    } else if #[cfg(getrandom_backend = "rdrand")] {
        mod rdrand;
        pub use rdrand::*;
    } else if #[cfg(getrandom_backend = "rndr")] {
        mod rndr;
        pub use rndr::*;
    } else if #[cfg(getrandom_backend = "efi_rng")] {
        mod efi_rng;
        pub use efi_rng::*;
    } else if #[cfg(getrandom_backend = "windows_legacy")] {
        mod windows_legacy;
        pub use windows_legacy::*;
    } else if #[cfg(getrandom_backend = "unsupported")] {
        mod unsupported;
        pub use unsupported::*;
    } else if #[cfg(getrandom_backend = "extern_impl")] {
        pub(crate) mod extern_impl;
        pub use extern_impl::*;
    } else if #[cfg(all(target_os = "linux", target_env = ""))] {
        mod linux_raw;
        pub use linux_raw::*;
    } else if #[cfg(target_os = "espidf")] {
        mod esp_idf;
        pub use esp_idf::*;
    } else if #[cfg(any(
        target_os = "haiku",
        target_os = "redox",
        target_os = "nto",
        target_os = "aix",
    ))] {
        mod use_file;
        pub use use_file::*;
    } else if #[cfg(any(
        target_os = "macos",
        target_os = "openbsd",
        target_os = "vita",
        target_os = "emscripten",
    ))] {
        mod getentropy;
        pub use getentropy::*;
    } else if #[cfg(all(target_os = "linux", any(target_arch = "aarch64", target_arch = "arm", target_arch = "powerpc", target_arch = "powerpc64", target_arch = "s390x", target_arch = "x86", target_arch = "x86_64", all(target_env = "musl", not(any(target_arch = "riscv64", target_arch = "riscv32"))))))] {
        mod use_file;
        mod linux_android_with_fallback;
        pub use linux_android_with_fallback::*;
    } else if #[cfg(any(target_os = "linux", target_os = "dragonfly", target_os = "freebsd", target_os = "hurd", target_os = "illumos", target_os = "cygwin", // Check for target_arch = "arm" to only include the 3DS. Does not
        // include the Nintendo Switch (which is target_arch = "aarch64").
        all(target_os = "horizon", target_arch = "arm")))] {
        mod getrandom;
        pub use getrandom::*;
    } else if #[cfg(target_os = "solaris")] {
        mod solaris;
        pub use solaris::*;
    } else if #[cfg(target_os = "netbsd")] {
        mod netbsd;
        pub use netbsd::*;
    }  else if #[cfg(any(target_os = "visionos", target_os = "watchos", target_os = "tvos"))] {
        mod apple_other;
        pub use apple_other::*;
    } else if #[cfg(all(target_arch = "wasm32", target_os = "wasi"))] {
        cfg_if! {
            if #[cfg(target_env = "p1")] {
                mod wasi_p1;
                pub use wasi_p1::*;
            } else {
                mod wasi_p2_3;
                pub use wasi_p2_3::*;
            }
        }
    } else if #[cfg(target_os = "hermit")] {
        mod hermit;
        pub use hermit::*;
    } else if #[cfg(all(target_arch = "x86_64", target_os = "motor"))] {
        mod rdrand;
        pub use rdrand::*;
    } else if #[cfg(target_os = "vxworks")] {
        mod vxworks;
        pub use vxworks::*;
    } else if #[cfg(target_os = "solid_asp3")] {
        mod solid;
        pub use solid::*;
    } else if #[cfg(all(windows, target_vendor = "win7"))] {
        mod windows_legacy;
        pub use windows_legacy::*;
    } else if #[cfg(windows)] {
        mod windows;
        pub use windows::*;
    } else if #[cfg(all(target_arch = "x86_64", target_env = "sgx"))] {
        mod rdrand;
        pub use rdrand::*;
    } else if #[cfg(all(target_family = "wasm", any(target_os = "unknown", target_os = "none")))] {
        cfg_if! {
            if #[cfg(feature = "wasm_js")] {
                mod wasm_js;
                pub use wasm_js::*;
            } else {
                compile_error!(concat!(
                    "The wasm32/64-unknown-unknown are not supported by default; \
                    you may need to enable the \"wasm_js\" crate feature. \
                    For more information see: \
                    https://docs.rs/getrandom/", env!("CARGO_PKG_VERSION"), "/#webassembly-support"
                ));
            }
        }
    } else {
        compile_error!(concat!(
            "target is not supported. You may need to define a custom backend see: \
            https://docs.rs/getrandom/", env!("CARGO_PKG_VERSION"), "/#custom-backend"
        ));
    }
}
