# Candidate decimal provider API

This trait is public within the standalone candidate Rust API so its public tokenizer and serialization methods have a usable provider bound. It is not a bridge, shared export, or ABI/FFI contract. It has no default implementation.

1. `parse_ascii_decimal(source)` accepts exactly the already grammar-selected ASCII slice and returns binary64 bits, consumed byte count, and status. Rust owns grammar selection and requires full consumption. The provider owns WTF/double-conversion decimal-to-binary behavior, including ties-to-even, overflow infinity, signed underflow zero, invalid status, and raw precision retention.
2. `format_ecmascript_shortest(bits)` returns the exact ASCII from Blink's configured shortest `DoubleToStringConverter`, including unique-zero behavior, notation thresholds, exponent sign/case, and shortest tie behavior.
3. `format_wtf_six_significant(bits)` returns the exact ASCII from `StringBuilder::AppendNumber(double)` / WTF precision-six path, including half-away-from-zero precision rounding, exponent padding, and trailing-zero handling.

Rust applies the <=14 digit 8-bit integer fast path before parse, applies sign after unsigned accumulation to preserve negative zero, clamps construction to `+/-f32::MAX` as an f64, stores numeric sign and integer-vs-number type, and performs integer serialization saturation to i64. Serialization chooses integer saturation, shortest formatting plus the number-only `.0` rule, WTF6 percentage formatting plus `%`, or shortest dimension formatting plus CSS-escaped unit. Preserve the existing dimension exponent/unit serialization defect.

The exact C++/vendor closure is frozen in `reference-cpp/numeric-closure-b1d5.json`; original numeric test references are in `reference-cpp/numeric-validation-reference-b1d5.json`. Unit tests with fixed provider fixtures exercise Rust policy only. Provider parity requires a director-scheduled oracle comparison of binary64 bits/consumed lengths and exact formatter bytes. Such fixtures are not provider validation.
