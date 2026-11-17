# UI geometry conversion candidate

This is a candidate implementation only. Nothing here is wired to the public C++ ABI, GN, exports, platform adapters, or configured build. The Rust module is safe Rust and uses standalone value types so a director can review the semantics before designing a bridge.

## Frozen inputs and identity

`source-before/` contains byte-for-byte copies taken from the clean task checkout at `c46d3f4975af8785d3fc1d733cf25360914ef884`. Git blob IDs identify repository objects; SHA-256 identifies copied raw bytes. They are distinct identity schemes. The three original conversion `.cc` raw hashes exactly match the canonical queue observations. `source-manifest.tsv` records every frozen file's repository blob and copied-byte hash.

The frozen algorithm/helper closure includes point/size/rect conversion sources and declarations, point/size/rect floating and integer value types, `base/numerics/safe_conversions.h` and its implementation helper, and all directly relevant original conversion/scaling assertion suites. `Rect::SetByBounds` and its overflow adjustment are behavior-bearing and must be checked against the copied `rect.h` before integration; no compiler run was made.

## Original assertion inventory

The exact source rows are retained in `source-before/`; the executable Rust cases currently translate these rows:

| Original test rows | Candidate case |
| --- | --- |
| `PointFTest.ToRoundedPoint`: all 15 EXPECT_EQ rows | `point_rounded_original_assertion_rows` |
| `SizeFTest.ToFlooredSize`, `ToCeiledSize`, `ToRoundedSize`: all 30 EXPECT_EQ rows | `size_conversion_original_assertion_rows` |
| `RectConversionsTest.ToEnclosedRect`, `ToEnclosedRectHugeRectF`, `ToEnclosingRect`, `ToEnclosingRectHugeRectF`, `ToEnclosingRectIgnoringError`, `ToNearestRect`, `ToFlooredRect` | edge matrix covers representative rows only; remaining assertion-by-assertion mapping is pending |
| `RectTest.ScaleToEnclosedRect`, `ScaleToEnclosingRect`, `ScaleToRoundedRect`, shared `TestScaleRectOverflowClamp` rows | edge matrix covers ordinary/empty cases only; overflow matrix translation is pending |

The remaining rect assertion translation is an acceptance gap and must be completed before asking for validation or claiming parity. The source suites contain unrelated PointF/SizeF operations; they are preserved for attribution but outside this conversion family.

## Semantics and known gaps

Scalar conversion explicitly maps NaN to zero, clamps infinities and finite overflow, and uses ties-away-from-zero. It widens the rounded `f32` to `f64` before exact i32 bounds checks because `i32::MAX as f32` rounds to 2^31. `f32` arithmetic and edge calculation remain in original order. Empty enclosing rectangles keep the floored origin; enclosed conversion follows the original four-edge calculation. The `IgnoringError` helper uses strict `< error`, including its behavior for nonpositive and NaN errors.

The source audit found callers in canvas sizing/bounds, DIP conversion, text layout, selection bounds, line conversion, transforms, and rounded-rectangle conversion. These callers consume public gfx types/functions; exact ABI, component exports, generated/platform bindings, GN ownership, and FENV/platform behavior remain director-owned. Negative and nonfinite scale factors need explicit cross-platform contract review. Rust and C++ floating environment behavior has not been compared. Debug DCHECK/`ToNearestRect` preconditions require a proposal decision before bridge integration. No unsafe code is present.

## Validation

`tests.rs` is an executable `rustc --test` harness candidate, but has **not** been compiled or run. Compiler, tests, benchmarks, configured build, sanitizer, and fuzz validation remain pending. Sanitizer/fuzz campaigns remain off. No production migration or cutover claim is made.
