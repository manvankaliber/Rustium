# UI geometry conversion candidate

This is a candidate implementation only. Nothing here is wired to the public C++ ABI, GN, exports, platform adapters, or configured build. The Rust module is safe Rust and uses standalone value types so a director can review the semantics before designing a bridge.

## Frozen inputs and identity

`source-before/` contains byte-for-byte copies taken from the clean task checkout at `c46d3f4975af8785d3fc1d733cf25360914ef884`. Git blob IDs identify repository objects; SHA-256 identifies copied raw bytes. They are distinct identity schemes. The three original conversion `.cc` raw hashes exactly match the canonical queue observations. `source-manifest.tsv` records every frozen file's repository blob and copied-byte hash.

The frozen algorithm/helper closure includes point/size/rect conversion sources and declarations, point/size/rect floating and integer value types, `rect.cc` (including `SaturatedClampRange` and both saturated-edge adjustments), `base/numerics/safe_conversions.h` and its generic/ARM helpers, `clamped_math.h` and its implementation helper, and all directly relevant original conversion/scaling assertion suites. `Rect::SetByBounds` was audited in the frozen `rect.h`/`rect.cc`: each axis first uses saturating subtraction, clamps the extent to the representable `origin + extent`, and calls `SaturatedClampRange` if the requested far edge still differs. That helper keeps the requested span if representable; otherwise it uses the far edge when close to zero, the near edge when close to zero, or centers the maximal span. The Rust `rect_axis_bounds` implements that order with widened intermediates, saturating i32 operations, unsigned widened absolute values, and truncating nonnegative division, and the exact large-range outputs are asserted below. No compiler run was made.

## Original assertion inventory

The exact source rows are retained in `source-before/`; the executable Rust cases currently translate these rows:

| Original test rows | Candidate case |
| --- | --- |
| `PointFTest.ToRoundedPoint`: all 15 EXPECT_EQ rows | `point_rounded_original_assertion_rows` |
| `SizeFTest.ToFlooredSize`, `ToCeiledSize`, `ToRoundedSize`: all 30 EXPECT_EQ rows | `size_conversion_original_assertion_rows` |
| `RectConversionsTest.ToEnclosedRect` (10), `ToEnclosedRectHugeRectF` (6), `ToEnclosingRect` (14), `ToEnclosingRectHugeRectF` (6), `ToEnclosingRectIgnoringError` (14), `ToNearestRect` (3), `ToFlooredRect` (4) | `every_original_rect_conversion_assertion_row`, `every_original_enclosing_rect_assertion_row`, `every_original_enclosing_ignoring_error_assertion_row`; RC-E/RC-EH/RC-C/RC-CH/RC-I/RC-N/RC-F comments map each row to exact frozen `rect_conversions_unittest.cc` lines. Huge-range relational checks are strengthened to exact origin, far edge, and dimensions. |
| `RectTest` constructor clamp (1), `ScaleToEnclosedRect` (8), `ScaleToEnclosingRect` (8), `ScaleToRoundedRect` (8), shared `TestScaleRectOverflowClamp` (13 per function) | `every_original_rect_scaling_assertion_row`; S01-S08 and O01-O13 comments map source lines; all three scale functions assert each ordinary and overflow case with exact edges, dimensions and emptiness. |

The point conversion suite contributes all 15 `ToRoundedPoint` rows, and the size conversion suite contributes all 30 floor/ceil/round rows. Other PointF/SizeF operations in those files are unrelated to this conversion family and are frozen for context, not claimed as candidate coverage.

## Semantics and known gaps

Scalar conversion explicitly maps NaN to zero, clamps infinities and finite overflow, and uses ties-away-from-zero. It widens the rounded `f32` to `f64` before exact i32 bounds checks because `i32::MAX as f32` rounds to 2^31; the fixture for `2147483520.0f32` expects 2147483520 because that actual float is below the limit, while 2^31 clamps to `i32::MAX`. `f32` arithmetic and edge calculation remain in original order. Frozen `SizeF::clamp` is `f > 8 * epsilon ? f : 0`; candidate size/rect conversions normalize input dimensions before computing edges, so the original tiny-width case follows the empty-width path. Empty enclosing rectangles keep the floored origin; enclosed conversion follows the original four-edge calculation. The `IgnoringError` helper uses strict `< error`, including its behavior for nonpositive and NaN errors.

The source audit found callers in canvas sizing/bounds, DIP conversion, text layout, selection bounds, line conversion, transforms, and rounded-rectangle conversion. These callers consume public gfx types/functions; exact ABI, component exports, generated/platform bindings, GN ownership, and FENV/platform behavior remain director-owned. Negative and nonfinite scale factors need explicit cross-platform contract review. Rust and C++ floating environment behavior has not been compared. Debug DCHECK/`ToNearestRect` preconditions require a proposal decision before bridge integration. No unsafe code is present.

## Validation

`tests.rs` is an executable `rustc --test` harness candidate containing assertion-level translations of all original rows in the named conversion and scale suites, with exact expected rectangles and source line/case labels. It has **not** been compiled or run. Compiler, tests, benchmarks, configured build, sanitizer, and fuzz validation remain pending. Sanitizer/fuzz campaigns remain off. No production migration or cutover claim is made.
