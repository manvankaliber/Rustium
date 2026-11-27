use super::*;
#[cfg(target_os = "windows")]
#[test]
fn native_windows_warning_and_reentry() {
    use std::sync::atomic::{AtomicU32, Ordering};
    static OBSERVED: AtomicU32 = AtomicU32::new(0);
    unsafe extern "C" fn warn(code: u32) {
        // Reentry performs no warning and borrows no prior Rust/native data.
        // SAFETY: the same valid non-unwinding callback is passed recursively.
        assert_eq!(unsafe { net_error_rust_windows_native(0, warn) }, 0);
        OBSERVED.store(code, Ordering::Relaxed);
    }
    let call = std::hint::black_box(
        net_error_rust_windows_native
            as unsafe extern "C" fn(u32, unsafe extern "C" fn(u32)) -> i32,
    );
    // SAFETY: warn is valid, synchronous, non-unwinding on the tested inputs.
    unsafe {
        assert_eq!(call(0, warn), 0);
        assert_eq!(
            call(windows_sys::Win32::Networking::WinSock::WSAEINVAL as u32, warn),
            core::ids::ERR_INVALID_ARGUMENT
        );
        assert_eq!(OBSERVED.load(Ordering::Relaxed), 0);
        assert_eq!(call(u32::MAX, warn), core::ids::ERR_FAILED);
        assert_eq!(OBSERVED.load(Ordering::Relaxed), u32::MAX);
    }
}
#[test]
fn win_ffi() {
    let c = WindowsConstants {
        wsaewouldblock: 100,
        wsa_io_pending: 101,
        wsaeacces: 102,
        wsaenetdown: 103,
        wsaetimedout: 104,
        wsaeconnreset: 105,
        wsaenetreset: 106,
        wsaeconnaborted: 107,
        wsaeconnrefused: 108,
        wsa_io_incomplete: 109,
        wsaediscon: 110,
        wsaeisconn: 111,
        wsaehostunreach: 112,
        wsaenetunreach: 113,
        wsaeaddrnotavail: 114,
        wsaemsgsize: 115,
        wsaenotconn: 116,
        wsaeafnosupport: 117,
        wsaeinval: 118,
        wsaeaddrinuse: 119,
        wsaenobufs: 120,
        error_file_not_found: 121,
        error_path_not_found: 122,
        error_too_many_open_files: 123,
        error_access_denied: 124,
        error_invalid_handle: 125,
        error_not_enough_memory: 126,
        error_outofmemory: 127,
        error_write_protect: 128,
        error_sharing_violation: 129,
        error_lock_violation: 130,
        error_handle_eof: 131,
        error_handle_disk_full: 132,
        error_file_exists: 133,
        error_invalid_parameter: 134,
        error_buffer_overflow: 135,
        error_disk_full: 136,
        error_call_not_implemented: 137,
        error_invalid_name: 138,
        error_dir_not_empty: 139,
        error_busy: 140,
        error_already_exists: 141,
        error_filename_exced_range: 142,
        error_file_too_large: 143,
        error_virus_infected: 144,
        error_io_device: 145,
        error_possible_deadlock: 146,
        error_bad_device: 147,
        error_broken_pipe: 148,
    };
    // SAFETY: synthetic initialized POD is immutable and lives through each
    // call.
    let call = std::hint::black_box(
        net_error_rust_windows as unsafe extern "C" fn(u32, *const WindowsConstants) -> i32,
    );
    unsafe {
        assert_eq!(call(0, &c), 0);
        assert_ne!(call(c.wsaewouldblock, &c), UNKNOWN);
        assert_ne!(call(c.wsa_io_pending, &c), UNKNOWN);
        assert_ne!(call(c.wsaeacces, &c), UNKNOWN);
        assert_ne!(call(c.wsaenetdown, &c), UNKNOWN);
        assert_ne!(call(c.wsaetimedout, &c), UNKNOWN);
        assert_ne!(call(c.wsaeconnreset, &c), UNKNOWN);
        assert_ne!(call(c.wsaenetreset, &c), UNKNOWN);
        assert_ne!(call(c.wsaeconnaborted, &c), UNKNOWN);
        assert_ne!(call(c.wsaeconnrefused, &c), UNKNOWN);
        assert_ne!(call(c.wsa_io_incomplete, &c), UNKNOWN);
        assert_ne!(call(c.wsaediscon, &c), UNKNOWN);
        assert_ne!(call(c.wsaeisconn, &c), UNKNOWN);
        assert_ne!(call(c.wsaehostunreach, &c), UNKNOWN);
        assert_ne!(call(c.wsaenetunreach, &c), UNKNOWN);
        assert_ne!(call(c.wsaeaddrnotavail, &c), UNKNOWN);
        assert_ne!(call(c.wsaemsgsize, &c), UNKNOWN);
        assert_ne!(call(c.wsaenotconn, &c), UNKNOWN);
        assert_ne!(call(c.wsaeafnosupport, &c), UNKNOWN);
        assert_ne!(call(c.wsaeinval, &c), UNKNOWN);
        assert_ne!(call(c.wsaeaddrinuse, &c), UNKNOWN);
        assert_ne!(call(c.wsaenobufs, &c), UNKNOWN);
        assert_ne!(call(c.error_file_not_found, &c), UNKNOWN);
        assert_ne!(call(c.error_path_not_found, &c), UNKNOWN);
        assert_ne!(call(c.error_too_many_open_files, &c), UNKNOWN);
        assert_ne!(call(c.error_access_denied, &c), UNKNOWN);
        assert_ne!(call(c.error_invalid_handle, &c), UNKNOWN);
        assert_ne!(call(c.error_not_enough_memory, &c), UNKNOWN);
        assert_ne!(call(c.error_outofmemory, &c), UNKNOWN);
        assert_ne!(call(c.error_write_protect, &c), UNKNOWN);
        assert_ne!(call(c.error_sharing_violation, &c), UNKNOWN);
        assert_ne!(call(c.error_lock_violation, &c), UNKNOWN);
        assert_ne!(call(c.error_handle_eof, &c), UNKNOWN);
        assert_ne!(call(c.error_handle_disk_full, &c), UNKNOWN);
        assert_ne!(call(c.error_file_exists, &c), UNKNOWN);
        assert_ne!(call(c.error_invalid_parameter, &c), UNKNOWN);
        assert_ne!(call(c.error_buffer_overflow, &c), UNKNOWN);
        assert_ne!(call(c.error_disk_full, &c), UNKNOWN);
        assert_ne!(call(c.error_call_not_implemented, &c), UNKNOWN);
        assert_ne!(call(c.error_invalid_name, &c), UNKNOWN);
        assert_ne!(call(c.error_dir_not_empty, &c), UNKNOWN);
        assert_ne!(call(c.error_busy, &c), UNKNOWN);
        assert_ne!(call(c.error_already_exists, &c), UNKNOWN);
        assert_ne!(call(c.error_filename_exced_range, &c), UNKNOWN);
        assert_ne!(call(c.error_file_too_large, &c), UNKNOWN);
        assert_ne!(call(c.error_virus_infected, &c), UNKNOWN);
        assert_ne!(call(c.error_io_device, &c), UNKNOWN);
        assert_ne!(call(c.error_possible_deadlock, &c), UNKNOWN);
        assert_ne!(call(c.error_bad_device, &c), UNKNOWN);
        assert_ne!(call(c.error_broken_pipe, &c), UNKNOWN);
        assert_eq!(call(1, &c), UNKNOWN);
    }
}
#[test]
fn posix_ffi() {
    let c = PosixConstants {
        eagain: 100,
        ewouldblock: 101,
        eacces: 102,
        enetdown: 103,
        etimedout: 104,
        econnreset: 105,
        enetreset: 106,
        epipe: 107,
        econnaborted: 108,
        econnrefused: 109,
        ehostunreach: 110,
        ehostdown: 111,
        enetunreach: 112,
        eafnosupport: 113,
        eaddrnotavail: 114,
        emsgsize: 115,
        enotconn: 116,
        eisconn: 117,
        einval: 118,
        eaddrinuse: 119,
        e2big: 120,
        ebadf: 121,
        ebusy: 122,
        ecanceled: 123,
        edeadlk: 124,
        edquot: 125,
        eexist: 126,
        efault: 127,
        efbig: 128,
        eisdir: 129,
        enametoolong: 130,
        enfile: 131,
        enobufs: 132,
        enodev: 133,
        enoent: 134,
        enolck: 135,
        enomem: 136,
        enospc: 137,
        enosys: 138,
        enotdir: 139,
        enotsup: 140,
        eperm: 141,
        erofs: 142,
        etxtbsy: 143,
        eusers: 144,
        emfile: 145,
        enoprotoopt: 146,
    };
    // SAFETY: synthetic initialized POD is immutable and lives through each
    // call.
    let call = std::hint::black_box(
        net_error_rust_posix as unsafe extern "C" fn(i32, *const PosixConstants) -> i32,
    );
    unsafe {
        assert_eq!(call(0, &c), 0);
        assert_ne!(call(c.eagain, &c), UNKNOWN);
        assert_ne!(call(c.ewouldblock, &c), UNKNOWN);
        assert_ne!(call(c.eacces, &c), UNKNOWN);
        assert_ne!(call(c.enetdown, &c), UNKNOWN);
        assert_ne!(call(c.etimedout, &c), UNKNOWN);
        assert_ne!(call(c.econnreset, &c), UNKNOWN);
        assert_ne!(call(c.enetreset, &c), UNKNOWN);
        assert_ne!(call(c.epipe, &c), UNKNOWN);
        assert_ne!(call(c.econnaborted, &c), UNKNOWN);
        assert_ne!(call(c.econnrefused, &c), UNKNOWN);
        assert_ne!(call(c.ehostunreach, &c), UNKNOWN);
        assert_ne!(call(c.ehostdown, &c), UNKNOWN);
        assert_ne!(call(c.enetunreach, &c), UNKNOWN);
        assert_ne!(call(c.eafnosupport, &c), UNKNOWN);
        assert_ne!(call(c.eaddrnotavail, &c), UNKNOWN);
        assert_ne!(call(c.emsgsize, &c), UNKNOWN);
        assert_ne!(call(c.enotconn, &c), UNKNOWN);
        assert_ne!(call(c.eisconn, &c), UNKNOWN);
        assert_ne!(call(c.einval, &c), UNKNOWN);
        assert_ne!(call(c.eaddrinuse, &c), UNKNOWN);
        assert_ne!(call(c.e2big, &c), UNKNOWN);
        assert_ne!(call(c.ebadf, &c), UNKNOWN);
        assert_ne!(call(c.ebusy, &c), UNKNOWN);
        assert_ne!(call(c.ecanceled, &c), UNKNOWN);
        assert_ne!(call(c.edeadlk, &c), UNKNOWN);
        assert_ne!(call(c.edquot, &c), UNKNOWN);
        assert_ne!(call(c.eexist, &c), UNKNOWN);
        assert_ne!(call(c.efault, &c), UNKNOWN);
        assert_ne!(call(c.efbig, &c), UNKNOWN);
        assert_ne!(call(c.eisdir, &c), UNKNOWN);
        assert_ne!(call(c.enametoolong, &c), UNKNOWN);
        assert_ne!(call(c.enfile, &c), UNKNOWN);
        assert_ne!(call(c.enobufs, &c), UNKNOWN);
        assert_ne!(call(c.enodev, &c), UNKNOWN);
        assert_ne!(call(c.enoent, &c), UNKNOWN);
        assert_ne!(call(c.enolck, &c), UNKNOWN);
        assert_ne!(call(c.enomem, &c), UNKNOWN);
        assert_ne!(call(c.enospc, &c), UNKNOWN);
        assert_ne!(call(c.enosys, &c), UNKNOWN);
        assert_ne!(call(c.enotdir, &c), UNKNOWN);
        assert_ne!(call(c.enotsup, &c), UNKNOWN);
        assert_ne!(call(c.eperm, &c), UNKNOWN);
        assert_ne!(call(c.erofs, &c), UNKNOWN);
        assert_ne!(call(c.etxtbsy, &c), UNKNOWN);
        assert_ne!(call(c.eusers, &c), UNKNOWN);
        assert_ne!(call(c.emfile, &c), UNKNOWN);
        assert_ne!(call(c.enoprotoopt, &c), UNKNOWN);
        assert_eq!(call(1, &c), UNKNOWN);
    }
}

mod tests {
    use super::*;
    #[test]
    fn raw_output_and_lengths() {
        // Exercise genuinely uninitialized output without ever forming a u8
        // slice.
        let mut out = [std::mem::MaybeUninit::<u8>::uninit(); 128];
        let name = b"QUIC_INTERNAL_ERROR";
        let call = std::hint::black_box(
            net_error_rust_format
                as unsafe extern "C" fn(i32, bool, *const u8, usize, *mut u8, usize) -> usize,
        );
        for (error, full, quic, length) in [
            (0, false, std::ptr::null(), 0),
            (-2, true, std::ptr::null(), 0),
            (-356, true, name.as_ptr(), name.len()),
        ] {
            // SAFETY: valid immutable input and writable uninitialized output.
            unsafe {
                let n = call(error, full, quic, length, std::ptr::null_mut(), 0);
                assert_eq!(call(error, full, quic, length, out.as_mut_ptr().cast(), out.len()), n);
                let bytes = std::slice::from_raw_parts(out.as_ptr().cast::<u8>(), n);
                assert!(!bytes.is_empty());
                let mut short = [0xA5u8; 1];
                assert_eq!(call(error, full, quic, length, short.as_mut_ptr(), 1), n);
                assert_eq!(short, [0xA5]);
            }
        }
    }
}
