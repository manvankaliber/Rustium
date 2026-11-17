use super::*;

#[test]
fn original_helper_size_tables() {
    for v in 0..128u64 {
        assert_eq!(encoded_len_unsigned(v), 1);
    }
    for shift in (7..64).step_by(7) {
        let low = 1u64 << shift;
        for v in [low, low + 1, low | (low - 1)] {
            assert_eq!(encoded_len_unsigned(v), ((64 - v.leading_zeros() + 6) / 7) as usize);
        }
    }
    for v in [
        0,
        1,
        3,
        63,
        64,
        127,
        8191,
        8192,
        -1,
        -64,
        -65,
        -8192,
        -8193,
        i32::MIN as i64,
        i32::MAX as i64,
        i64::MIN,
        i64::MAX,
    ] {
        assert_eq!(
            encoded_len_signed(v),
            encode_signed(
                v,
                if v >= i32::MIN as i64 && v <= i32::MAX as i64 { Width::I32 } else { Width::I64 }
            )
            .unwrap()
            .len()
        );
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
