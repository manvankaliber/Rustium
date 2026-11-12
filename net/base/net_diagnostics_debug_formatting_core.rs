// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#![forbid(unsafe_code)]
#[path = "net_diagnostics_load_flags_generated.rs"]
pub mod load_flags_generated;

const DEFINED_MASK: u32 = {
    let mut mask = 0;
    let mut index = 1;
    while index < load_flags_generated::LOAD_FLAGS.len() {
        mask |= load_flags_generated::LOAD_FLAGS[index].0;
        index += 1;
    }
    mask
};

pub fn load_flags_length(flags: i32) -> Option<usize> {
    let unsigned = flags as u32;
    if unsigned & !DEFINED_MASK != 0 {
        return None;
    }
    if flags == 0 {
        return load_flags_generated::LOAD_FLAGS.first().map(|(_, name)| 5 + name.len());
    }
    let mut length = 0;
    for &(value, name) in load_flags_generated::LOAD_FLAGS.iter().skip(1) {
        if unsigned & value != 0 {
            length += 5 + name.len() + usize::from(length != 0) * 3;
        }
    }
    Some(length)
}

fn emit_parts(flags: i32, mut emit: impl FnMut(&str)) {
    if flags == 0 {
        emit("LOAD_");
        emit(load_flags_generated::LOAD_FLAGS[0].1);
        return;
    }
    let mut first = true;
    for &(value, name) in load_flags_generated::LOAD_FLAGS.iter().skip(1) {
        if flags as u32 & value != 0 {
            if !first {
                emit(" | ");
            }
            emit("LOAD_");
            emit(name);
            first = false;
        }
    }
}

/// Validate before writing, so invalid flags or insufficient space leave the
/// caller's initialized output untouched. The caller owns the allocation.
pub fn write_load_flags(flags: i32, output: &mut [u8]) -> bool {
    let Some(length) = load_flags_length(flags) else { return false };
    if output.len() < length {
        return false;
    }
    let mut offset = 0;
    emit_parts(flags, |part| {
        let end = offset + part.len();
        output[offset..end].copy_from_slice(part.as_bytes());
        offset = end;
    });
    true
}

/// Rust callers can retain owned strings without token vectors or an
/// intermediate joined string; reserve the exact final length once.
pub fn format_load_flags(flags: i32) -> Option<String> {
    let mut result = String::with_capacity(load_flags_length(flags)?);
    emit_parts(flags, |part| result.push_str(part));
    Some(result)
}

pub fn privacy_mode_bytes(mode: i32) -> Option<&'static [u8]> {
    match mode {
        0 => Some(b"disabled\0"),
        1 => Some(b"enabled\0"),
        2 => Some(b"enabled without client certs\0"),
        3 => Some(b"enabled partitioned state allowed\0"),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn frozen_model(flags: i32) -> Option<String> {
        let expected = (flags as u32).count_ones() as usize;
        if flags == 0 {
            return Some("LOAD_NORMAL".to_owned());
        }
        if expected == 0 || expected > 33 {
            return None;
        }
        let names: Vec<_> = load_flags_generated::LOAD_FLAGS
            .iter()
            .skip(1)
            .filter(|(v, _)| (flags as u32) & *v != 0)
            .map(|(_, n)| *n)
            .collect();
        if names.len() != expected {
            return None;
        }
        Some(format!("LOAD_{}", names.join(" | LOAD_")))
    }
    #[test]
    fn exhaustive_valid_combinations() {
        for f in 0..(1 << 20) {
            assert_eq!(format_load_flags(f), frozen_model(f), "{f:#x}");
        }
    }
    #[test]
    fn million_defined_differential_inputs() {
        let mask = load_flags_generated::LOAD_FLAGS.iter().skip(1).fold(0u32, |m, (v, _)| m | *v);
        let mut x = 0x9e3779b9u32;
        for _ in 0..1_000_000 {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            let f = (x & mask) as i32;
            assert_eq!(format_load_flags(f), frozen_model(f));
        }
    }
    #[test]
    fn invalid_flags_and_all_privacy_modes() {
        for f in [-1, i32::MIN, 1 << 30, (1 << 19) | (1 << 30)] {
            assert!(format_load_flags(f).is_none());
        }
        assert_eq!(privacy_mode_bytes(0), Some(&b"disabled\0"[..]));
        assert_eq!(privacy_mode_bytes(1), Some(&b"enabled\0"[..]));
        assert_eq!(privacy_mode_bytes(2), Some(&b"enabled without client certs\0"[..]));
        assert_eq!(privacy_mode_bytes(3), Some(&b"enabled partitioned state allowed\0"[..]));
        assert!(privacy_mode_bytes(-1).is_none());
        assert!(privacy_mode_bytes(4).is_none());
    }
}
