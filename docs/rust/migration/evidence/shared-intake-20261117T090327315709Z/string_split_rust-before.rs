// Copyright 2026 The Rustium Authors
// Director-owned typed bridge proposal. No production installation yet.
#[path = "string_split_rust_policy.rs"]
mod policy;

#[cxx::bridge(namespace = "base::internal::rust_string_split")]
mod ffi {
    struct Range {
        offset: usize,
        length: usize,
        default_empty: bool,
    }
    struct OnceOutput {
        found: bool,
        first: Range,
        second: Range,
    }
    struct PairRange {
        key: Range,
        value: Range,
        diagnostic: u8,
    }
    extern "Rust" {
        fn split_bytes(
            input: &[u8],
            delimiter: &[u8],
            trim: bool,
            nonempty: bool,
            substring: bool,
        ) -> Vec<Range>;
        fn split_utf16(
            input: &[u16],
            delimiter: &[u16],
            trim: bool,
            nonempty: bool,
            substring: bool,
        ) -> Vec<Range>;
        fn split_once(input: &[u8], delimiters: &[u8], reverse: bool) -> OnceOutput;
        fn key_value_segment(input: &[u8], delimiter: u8) -> PairRange;
    }
}

fn flags(trim: bool, nonempty: bool) -> (policy::Whitespace, policy::Results) {
    (
        if trim { policy::Whitespace::Trim } else { policy::Whitespace::Keep },
        if nonempty { policy::Results::NonEmpty } else { policy::Results::All },
    )
}
fn range8(input: &[u8], value: &[u8]) -> ffi::Range {
    ffi::Range {
        offset: value.as_ptr() as usize - input.as_ptr() as usize,
        length: value.len(),
        default_empty: false,
    }
}
fn range16(input: &[u16], value: &[u16]) -> ffi::Range {
    ffi::Range {
        offset: (value.as_ptr() as usize - input.as_ptr() as usize) / 2,
        length: value.len(),
        default_empty: false,
    }
}
fn empty() -> ffi::Range {
    ffi::Range { offset: 0, length: 0, default_empty: true }
}
fn split_bytes(
    input: &[u8],
    delimiter: &[u8],
    trim: bool,
    nonempty: bool,
    substring: bool,
) -> Vec<ffi::Range> {
    let (whitespace, results) = flags(trim, nonempty);
    let pieces = if substring {
        policy::split_bytes_substr(input, delimiter, whitespace, results)
    } else {
        policy::split_bytes(input, delimiter, whitespace, results)
    };
    pieces.into_iter().map(|piece| range8(input, piece)).collect()
}
fn split_utf16(
    input: &[u16],
    delimiter: &[u16],
    trim: bool,
    nonempty: bool,
    substring: bool,
) -> Vec<ffi::Range> {
    let (whitespace, results) = flags(trim, nonempty);
    let pieces = if substring {
        policy::split_utf16_substr(input, delimiter, whitespace, results)
    } else {
        policy::split_utf16(input, delimiter, whitespace, results)
    };
    pieces.into_iter().map(|piece| range16(input, piece)).collect()
}
fn split_once(input: &[u8], delimiters: &[u8], reverse: bool) -> ffi::OnceOutput {
    match policy::split_once(input, delimiters, reverse) {
        Some((first, second)) => ffi::OnceOutput {
            found: true,
            first: range8(input, first),
            second: range8(input, second),
        },
        None => ffi::OnceOutput { found: false, first: empty(), second: empty() },
    }
}
// The caller freezes the initial trimmed segment boundaries with split_bytes.
// Parse exactly this segment, without re-trimming or re-splitting it: a native
// logging callback may have changed its bytes since the boundaries were taken.
// This is the segment policy from the frozen AppendStringViewKeyValue and the
// accepted Rust key_value_views loop. No borrow survives the synchronous call.
fn key_value_segment(input: &[u8], delimiter: u8) -> ffi::PairRange {
    let Some(key_end) = input.iter().position(|c| *c == delimiter) else {
        return ffi::PairRange { key: empty(), value: empty(), diagnostic: 1 };
    };
    let key = range8(input, &input[..key_end]);
    let Some(value_offset) = input[key_end..].iter().position(|c| *c != delimiter) else {
        return ffi::PairRange { key, value: empty(), diagnostic: 2 };
    };
    ffi::PairRange { key, value: range8(input, &input[key_end + value_offset..]), diagnostic: 0 }
}
