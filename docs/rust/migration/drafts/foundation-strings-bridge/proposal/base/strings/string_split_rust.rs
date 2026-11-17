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
        segment_offset: usize,
        segment_length: usize,
        diagnostic: u8,
    }
    struct KeyValueOutput {
        success: bool,
        pairs: Vec<PairRange>,
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
        fn key_value(input: &[u8], delimiter: u8, pairs_delimiter: &[u8]) -> KeyValueOutput;
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
fn key_value(input: &[u8], delimiter: u8, pairs_delimiter: &[u8]) -> ffi::KeyValueOutput {
    let (success, pairs) = policy::key_value_views(input, delimiter, pairs_delimiter);
    // Diagnostic segment provenance is retained for the native DVLOG adapter.
    // Both calls use the same reviewed Rust splitter; no native policy scan.
    let segments = policy::split_bytes_substr(
        input,
        pairs_delimiter,
        policy::Whitespace::Trim,
        policy::Results::NonEmpty,
    );
    assert_eq!(pairs.len(), segments.len());
    let pairs = pairs
        .into_iter()
        .zip(segments)
        .map(|(pair, segment)| {
            let key_default = pair.key.origin == policy::ViewOrigin::DefaultEmpty;
            let value_default = pair.value.origin == policy::ViewOrigin::DefaultEmpty;
            let segment_range = range8(input, segment);
            ffi::PairRange {
                key: ffi::Range {
                    offset: pair.key.offset,
                    length: pair.key.bytes.len(),
                    default_empty: key_default,
                },
                value: ffi::Range {
                    offset: pair.value.offset,
                    length: pair.value.bytes.len(),
                    default_empty: value_default,
                },
                segment_offset: segment_range.offset,
                segment_length: segment_range.length,
                diagnostic: if key_default {
                    1
                } else if value_default {
                    2
                } else {
                    0
                },
            }
        })
        .collect();
    ffi::KeyValueOutput { success, pairs }
}
