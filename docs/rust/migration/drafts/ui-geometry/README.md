# UI geometry conversion candidate

This is a candidate implementation only. Nothing here is wired to the public C++ ABI, GN, exports, platform adapters, or configured build. The Rust module is safe Rust and uses standalone value types so a director can review the semantics before designing a bridge.

## Frozen inputs and identity

`source-before/` contains byte-for-byte copies taken from the clean task checkout at `c46d3f4975af8785d3fc1d733cf25360914ef884`. Git blob IDs identify repository objects; SHA-256 identifies copied raw bytes. They are distinct identity schemes. The three original conversion `.cc` raw hashes exactly match the canonical queue observations. `source-manifest.tsv` records every frozen file's repository blob and copied-byte hash.

The frozen, hash-bound sources now also include `point.h`, `size.h`, and `vector2d.h`; the relevant numeric chain includes `safe_math.h`, `checked_math.h`/`checked_math_impl.h`, `clamped_math.h`/`clamped_math_impl.h`, `safe_math_shared_impl.h`, `safe_math_clang_gcc_impl.h`, the ARM-selected `safe_math_arm_impl.h`, `safe_conversions.h` plus generic/ARM implementations, `basic_ops_impl.h`, and `integral_constant_like.h`. Every listed blob and copied-byte SHA-256 is pinned to base `c46d3f4975af8785d3fc1d733cf25360914ef884`. This closes the inspected integer point/size and numeric conversion/checked/clamped helper paths; unrelated platform adapters, build configuration, generated bindings and all broader geometry callers are outside this frozen closure. `Rect::SetByBounds` was audited in frozen `rect.h`/`rect.cc`: each axis first uses saturating subtraction, clamps the extent to the representable `origin + extent`, then calls `SaturatedClampRange` if the requested far edge still differs. That helper keeps the span if representable; otherwise it prefers a far edge near zero, then a near edge near zero, or centers the maximal span. Rust implements that order with widened intermediates and exact source-derived regression cases.

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

Scalar conversion explicitly maps NaN to zero, clamps infinities and finite overflow, and uses ties-away-from-zero. It widens the rounded `f32` to `f64` before exact i32 bounds checks because `i32::MAX as f32` rounds to 2^31; `2147483520.0f32` remains below the limit, while 2^31 clamps to `i32::MAX`. Constructor/setter normalization is explicit in `SizeF::constructed`/`RectF::constructed` and is not reapplied by conversions: native `InvScale` divides stored fields directly, so tiny, negative, infinite and NaN dimensions remain observable. Integer `Size` conversion separately applies its nonnegative output clamp. Deprecated floor conversion now routes through `rect_from_xywh`, matching the native Rect constructor's positive-origin extent clamp. Scale inputs validate the public candidate Rect fields against native nonnegative extent and representable far-edge invariants.

`ToNearestRect` has four strict `< 0.01f` debug assertions; the candidate preserves those at `nearest_rect`, while `rounded_rect` rounds independently without the nearest-only precondition. Enclosing routines retain the frozen source's left/right-before-top/bottom sequence. Nearest/distance routines calculate all four float edges before rounding them; the distance predicate then uses short-circuit comparisons. Some C++ argument expressions in enclosed conversion are unsequenced by the language. Exception/trap/flag effects and supported cross-platform FENV behavior remain unvalidated; preserving ordinary float expressions does not establish FENV equivalence. The `IgnoringError` helper uses strict `< error`, including nonpositive and NaN errors.

The source audit found callers in canvas sizing/bounds, DIP conversion, text layout, selection bounds, line conversion, transforms, and rounded-rectangle conversion. These callers consume public gfx types/functions; exact ABI, component exports, generated/platform bindings, GN ownership, and FENV/platform behavior remain director-owned. Negative and nonfinite scale factors need explicit cross-platform contract review. Rust and C++ floating environment behavior has not been compared. Debug DCHECK/`ToNearestRect` preconditions require a proposal decision before bridge integration. No unsafe code is present.

## Validation

The retained host evidence at `docs/rust/migration/evidence/candidate-tests-20261117T071422095094Z/ui-geometry-result.json` records **7/7 tests passed** for candidate commit `e4687848f9f05328364a419e87184afc6eef0b0f`, with earlier failed runs retained. It is historical evidence for that exact identity; this repair changes the candidate and requires a new director-gated host rerun. The host harness does not establish configured production, ABI, native-platform, cross-platform FENV, performance, sanitizer or fuzz acceptance. Sanitizer/fuzz campaigns remain off. No production migration or cutover claim is made.
