use super::*;

#[test]
fn original_helper_size_tables() {
    assert_eq!(encoded_len_unsigned(0), 1);
    assert_eq!(encoded_len_unsigned(1), 1);
    assert_eq!(encoded_len_unsigned(3), 1);
    for v in 4..128usize {
        assert_eq!(encoded_len_unsigned(v as u64), 1);
    }
    for v in (1 << 7)..(1 << 9) {
        assert_eq!(encoded_len_unsigned(v), 2);
    }
    for v in ((1 << 14)..(1 << 16)).step_by(33) {
        assert_eq!(encoded_len_unsigned(v), 3);
    }
    for v in ((1 << 21)..(1 << 24)).step_by(33_999) {
        assert_eq!(encoded_len_unsigned(v), 4);
    }
    for v in ((1usize << 28)..(1usize << 31)).step_by(33_997_779) {
        assert_eq!(encoded_len_unsigned(v as u64), 5);
    }
    assert_eq!(encoded_len_unsigned(0xffff_ffff), 5);

    assert_eq!(encoded_len_signed(0), 1);
    assert_eq!(encoded_len_signed(1), 1);
    assert_eq!(encoded_len_signed(3), 1);
    for v in 0..(1 << 6) {
        assert_eq!(encoded_len_signed(v), 1);
    }
    for v in (1 << 6)..(1 << 8) {
        assert_eq!(encoded_len_signed(v), 2);
    }
    for v in ((1 << 13)..(1 << 15)).step_by(31) {
        assert_eq!(encoded_len_signed(v), 3);
    }
    for v in ((1 << 20)..(1 << 22)).step_by(31_991) {
        assert_eq!(encoded_len_signed(v), 4);
    }
    for v in ((1 << 27)..(1 << 29)).step_by(3_199_893) {
        assert_eq!(encoded_len_signed(v), 5);
    }
    for v in -(1 << 6)..=0 {
        assert_eq!(encoded_len_signed(v), 1);
    }
    for v in -(1 << 13)..-(1 << 6) {
        assert_eq!(encoded_len_signed(v), 2);
    }
    for v in (-(1 << 20)..-(1 << 18)).step_by(11) {
        assert_eq!(encoded_len_signed(v), 3);
    }
    for v in (-(1 << 27)..-(1 << 25)).step_by(11_999) {
        assert_eq!(encoded_len_signed(v), 4);
    }
    for v in (-(1 << 30)..-(1 << 28)).step_by(1_199_999) {
        assert_eq!(encoded_len_signed(v), 5);
    }
}

#[test]
fn original_helper_write_decode_value_cases() {
    for v in [0, 1, 5, 99, 298, 87_348_723, 77_777, u32::MAX as u64] {
        let b = encode_unsigned(v, Width::U32).unwrap();
        assert_eq!(decode(&b, Width::U32, ValidationMode::Full).unwrap().value, v as i128);
    }
    for v in [
        0,
        1,
        5,
        99,
        298,
        87_348_723,
        77_777,
        -2,
        -4,
        -59,
        -288,
        -12_608,
        -87_328_723,
        -77_377,
        i32::MIN as i64,
        i32::MAX as i64,
    ] {
        let b = encode_signed(v, Width::I32).unwrap();
        assert_eq!(decode(&b, Width::I32, ValidationMode::Full).unwrap().value, v as i128);
    }
    for v in [0, 1, 5, 99, 298, 87_348_723, 77_777, u64::MAX] {
        let b = encode_unsigned(v, Width::U64).unwrap();
        assert_eq!(decode(&b, Width::U64, ValidationMode::Full).unwrap().value, v as i128);
    }
    let mut u64_bits = 0x3au64;
    while u64_bits != 0 {
        let b = encode_unsigned(u64_bits, Width::U64).unwrap();
        let got = decode(&b, Width::U64, ValidationMode::Full).unwrap();
        assert_eq!(got.value, u64_bits as i128);
        assert_eq!(got.bytes, b.len());
        u64_bits = u64_bits.wrapping_shl(1);
    }
    for v in [
        0,
        1,
        5,
        99,
        298,
        87_348_723,
        77_777,
        -2,
        -4,
        -59,
        -288,
        -87_648_723,
        -77_377,
        i64::MIN,
        i64::MAX,
    ] {
        let b = encode_signed(v, Width::I64).unwrap();
        assert_eq!(decode(&b, Width::I64, ValidationMode::Full).unwrap().value, v as i128);
    }
}

#[test]
fn original_shifted_write_decode_sequences() {
    let mut u32v = 0x3au32;
    while u32v != 0 {
        let bytes = encode_unsigned(u32v as u64, Width::U32).unwrap();
        assert_eq!(bytes.len(), encoded_len_unsigned(u32v as u64));
        let got = decode(&bytes, Width::U32, ValidationMode::Full).unwrap();
        assert_eq!(got.value, u32v as i128);
        assert_eq!(got.bytes, bytes.len());
        u32v = u32v.wrapping_shl(1);
    }

    let mut i32_positive_bits = 0x3au32;
    while i32_positive_bits != 0 {
        let value = i32_positive_bits as i32;
        let bytes = encode_signed(value as i64, Width::I32).unwrap();
        assert_eq!(bytes.len(), encoded_len_signed(value as i64));
        let got = decode(&bytes, Width::I32, ValidationMode::Full).unwrap();
        assert_eq!(got.value, value as i128);
        assert_eq!(got.bytes, bytes.len());
        i32_positive_bits = i32_positive_bits.wrapping_shl(1);
    }
    let mut i32_negative_bits = 0xffff_ff3bu32;
    while i32_negative_bits != 0 {
        let value = i32_negative_bits as i32;
        let bytes = encode_signed(value as i64, Width::I32).unwrap();
        assert_eq!(bytes.len(), encoded_len_signed(value as i64));
        let got = decode(&bytes, Width::I32, ValidationMode::Full).unwrap();
        assert_eq!(got.value, value as i128);
        assert_eq!(got.bytes, bytes.len());
        i32_negative_bits = i32_negative_bits.wrapping_shl(1);
    }

    let mut i64_positive_bits = 0x3au64;
    while i64_positive_bits != 0 {
        let value = i64_positive_bits as i64;
        let bytes = encode_signed(value, Width::I64).unwrap();
        assert_eq!(bytes.len(), encoded_len_signed(value));
        let got = decode(&bytes, Width::I64, ValidationMode::Full).unwrap();
        assert_eq!(got.value, value as i128);
        assert_eq!(got.bytes, bytes.len());
        i64_positive_bits = i64_positive_bits.wrapping_shl(1);
    }
    let mut i64_negative_bits = 0xffff_ffff_ffff_ff3bu64;
    while i64_negative_bits != 0 {
        let value = i64_negative_bits as i64;
        let bytes = encode_signed(value, Width::I64).unwrap();
        assert_eq!(bytes.len(), encoded_len_signed(value));
        let got = decode(&bytes, Width::I64, ValidationMode::Full).unwrap();
        assert_eq!(got.value, value as i128);
        assert_eq!(got.bytes, bytes.len());
        i64_negative_bits = i64_negative_bits.wrapping_shl(1);
    }
}

#[test]
fn accepts_existing_overlong_forms() {
    for (width, bytes, expected) in [
        (Width::U32, &[0x80, 0x00][..], 0),
        (Width::U32, &[0x81, 0x00], 1),
        (Width::I32, &[0xff, 0x7f], -1),
        (Width::I64, &[0xff, 0x7f], -1),
        (Width::I33, &[0xff, 0x7f], -1),
    ] {
        let got = decode(bytes, width, ValidationMode::Full).unwrap();
        assert_eq!(got.value, expected);
        assert_eq!(got.bytes, bytes.len());
    }
}

#[test]
fn final_byte_masks_sign_extension_and_width_boundaries() {
    for (width, values) in [
        (Width::U32, vec![0, 1, (1i128 << 32) - 1]),
        (Width::I32, vec![-(1i128 << 31), -1, 0, (1i128 << 31) - 1]),
        (Width::U64, vec![0, 1, (1i128 << 64) - 1]),
        (Width::I64, vec![-(1i128 << 63), -1, 0, (1i128 << 63) - 1]),
        (Width::I33, vec![-(1i128 << 32), -1, 0, (1i128 << 32) - 1]),
    ] {
        for v in values {
            let bytes = if width.signed() {
                encode_signed(v as i64, width).unwrap()
            } else {
                encode_unsigned(v as u64, width).unwrap()
            };
            let decoded = decode(&bytes, width, ValidationMode::Full).unwrap();
            assert_eq!(decoded.value, v);
            assert_eq!(decoded.bytes, width.max_bytes().min(bytes.len()));
        }
    }
    assert_eq!(
        decode(&[0xff, 0xff, 0xff, 0xff, 0x0f], Width::U32, ValidationMode::Full).unwrap().value,
        u32::MAX as i128
    );
    assert_eq!(
        decode(&[0xff, 0xff, 0xff, 0xff, 0x7f], Width::I32, ValidationMode::Full).unwrap().value,
        -1
    );
    assert_eq!(
        decode(&[0xff, 0xff, 0xff, 0xff, 0x7f], Width::I33, ValidationMode::Full).unwrap().value,
        -1
    );
}

#[test]
fn malformed_offsets_and_native_error_classes() {
    let cases = [
        (&[0x80][..], Width::U32, 1, DecodeErrorKind::ReachedEnd),
        (&[0x80, 0x80, 0x80, 0x80, 0x80][..], Width::U32, 4, DecodeErrorKind::LengthOverflow),
        (&[0x80, 0x80, 0x80, 0x80, 0x10][..], Width::U32, 4, DecodeErrorKind::ExtraBits),
        (&[0x80, 0x80, 0x80, 0x80, 0x08][..], Width::I32, 4, DecodeErrorKind::ExtraBits),
        (
            &[0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80][..],
            Width::U64,
            9,
            DecodeErrorKind::LengthOverflow,
        ),
        (
            &[0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02][..],
            Width::U64,
            9,
            DecodeErrorKind::ExtraBits,
        ),
    ];
    for (bytes, width, offset, kind) in cases {
        let err = decode(bytes, width, ValidationMode::Full).unwrap_err();
        assert_eq!(err.offset, offset);
        assert_eq!(err.kind, kind);
    }
    assert_eq!(
        decode(&[0x80], Width::U32, ValidationMode::Full).unwrap_err().native_message("LEB32"),
        "reached end while decoding LEB32"
    );
    assert_eq!(
        decode(&[0x80, 0x80, 0x80, 0x80, 0x80], Width::U32, ValidationMode::Full)
            .unwrap_err()
            .native_message("LEB32"),
        "length overflow while decoding LEB32"
    );
    assert_eq!(
        decode(&[0x80, 0x80, 0x80, 0x80, 0x10], Width::U32, ValidationMode::Full)
            .unwrap_err()
            .native_message("LEB32"),
        "extra bits in varint"
    );
}

#[test]
fn every_max_length_final_byte_matches_native_validity_classes() {
    // Mirrors ReadLEB_ExhaustiveLastByteValidation's five exposed decoder
    // shapes. Earlier bytes continue; the last byte itself determines either
    // exact decoded sign/value or V8's overflow/extra-bits class.
    for width in [Width::U32, Width::I32, Width::I33, Width::U64, Width::I64] {
        let max = width.max_bytes();
        for last in 0u16..=255 {
            let mut bytes = vec![0x80; max];
            bytes[max - 1] = last as u8;
            // Independent accepted ranges from the frozen decoder contract:
            // used payload bits followed by zero or sign-extension bits.
            let used = width.bits() - ((max - 1) * 7) as u32;
            let valid = last < 128
                && if width.signed() {
                    let half = 1u16 << (used - 1);
                    last < half || last >= 128 - half
                } else {
                    last < (1u16 << used)
                };
            let result = decode(&bytes, width, ValidationMode::Full);
            assert_eq!(result.is_ok(), valid, "{width:?} final={last:02x}");
            if valid {
                let decoded = result.unwrap();
                let payload_bits = used.min(7);
                let payload = (last as i128 & ((1i128 << payload_bits) - 1)) << ((max - 1) * 7);
                let expected = if width.signed() && last >= 64 {
                    payload - (1i128 << width.bits())
                } else {
                    payload
                };
                assert_eq!(decoded.value, expected, "{width:?} final={last:02x}");
                assert_eq!(decoded.bytes, max);
            } else {
                let err = result.unwrap_err();
                assert_eq!(err.offset, max - 1);
                assert_eq!(
                    err.kind,
                    if last >= 128 {
                        DecodeErrorKind::LengthOverflow
                    } else {
                        DecodeErrorKind::ExtraBits
                    }
                );
            }
        }
    }
}

#[test]
fn generated_bit_boundaries_repeated_prefixes_and_all_accepted_lengths() {
    // Mirrors the original bit-width sweeps and overlong-length loops for all
    // exposed widths. Every candidate length from canonical through max is
    // asserted, preserving redundant encodings instead of canonicalizing input.
    for width in [Width::U32, Width::I32, Width::U64, Width::I64, Width::I33] {
        let bits = width.bits();
        let signed = width.signed();
        for significant in 1..=bits {
            let sign_values = if signed {
                vec![0i128, 1, -1, (1i128 << (significant - 1)) - 1, -(1i128 << (significant - 1))]
            } else {
                vec![0i128, 1, (1i128 << significant) - 1]
            };
            for value in sign_values {
                let canonical = if signed {
                    encode_signed(value as i64, width).unwrap()
                } else {
                    encode_unsigned(value as u64, width).unwrap()
                };
                for target_len in canonical.len()..=width.max_bytes() {
                    let mut bytes = canonical.clone();
                    if target_len > bytes.len() {
                        let last = bytes.last_mut().unwrap();
                        *last |= 0x80;
                        let fill = if signed && value < 0 { 0x7f } else { 0x00 };
                        while bytes.len() + 1 < target_len {
                            bytes.push(fill | 0x80);
                        }
                        bytes.push(fill);
                    }
                    let decoded = decode(&bytes, width, ValidationMode::Full).unwrap();
                    assert_eq!(decoded.value, value, "{width:?} {value} {bytes:02x?}");
                    assert_eq!(decoded.bytes, target_len);
                }
            }
        }
    }
}

#[test]
fn original_decoder_padding_and_swar_boundary_slice_semantics() {
    // Reproduces the meaningful read/result/length behavior for V8's padded
    // 2/4/8-byte SWAR thresholds using safe Rust slices. Padding bytes after
    // the terminator must not change the value or consumed length.
    let cases: [(Width, Vec<u8>, i128); 5] = [
        (Width::U32, encode_unsigned(300, Width::U32).unwrap(), 300),
        (Width::I32, encode_signed(-10_000, Width::I32).unwrap(), -10_000),
        (Width::I33, encode_signed(-(1i64 << 32), Width::I33).unwrap(), -(1i128 << 32)),
        (Width::U64, encode_unsigned(u64::MAX, Width::U64).unwrap(), u64::MAX as i128),
        (Width::I64, encode_signed(i64::MIN, Width::I64).unwrap(), i64::MIN as i128),
    ];
    for (width, encoded, expected) in cases {
        for pad in [0x00, 0x7f, 0x80, 0xff] {
            for prefix in [0usize, 1, 7, 8] {
                for trailing in [0usize, 1, 2, 4, 8, 16] {
                    let mut storage = vec![pad; prefix];
                    storage.extend_from_slice(&encoded);
                    storage.extend(std::iter::repeat(pad).take(trailing));
                    let view = &storage[prefix..];
                    let decoded = decode(view, width, ValidationMode::Full).unwrap();
                    assert_eq!(decoded.value, expected);
                    assert_eq!(decoded.bytes, encoded.len());
                    assert_eq!(&storage[prefix..prefix + decoded.bytes], encoded.as_slice());
                }
            }
        }
        for limit in 0..encoded.len() {
            let err = decode(&encoded[..limit], width, ValidationMode::Full).unwrap_err();
            assert_eq!(err.offset, limit);
            assert_eq!(err.kind, DecodeErrorKind::ReachedEnd);
        }
    }
}

#[test]
fn size_helpers_and_encoders_match() {
    for value in [0, 1, 127, 128, 16_383, 16_384, (1u64 << 28) - 1, u32::MAX as u64] {
        let bytes = encode_unsigned(value, Width::U32).unwrap();
        assert_eq!(encoded_len_unsigned(value), bytes.len());
    }
    for value in [i32::MIN as i64, -65, -64, -1, 0, 63, 64, i32::MAX as i64] {
        let bytes = encode_signed(value, Width::I32).unwrap();
        assert_eq!(encoded_len_signed(value), bytes.len());
    }
}
