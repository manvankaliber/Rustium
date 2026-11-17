//! Standalone executable assertion cases. Run only under the director's
//! validation gate; this worker did not execute a compiler or test command.
#![allow(clippy::approx_constant)]
#[path = "src/lib.rs"]
mod geom;
use geom::*;

#[test]
fn point_rounded_original_assertion_rows() {
    let rows = [
        ((0.0, 0.0), (0, 0)),
        ((0.0001, 0.0001), (0, 0)),
        ((0.4999, 0.4999), (0, 0)),
        ((0.5, 0.5), (1, 1)),
        ((0.9999, 0.9999), (1, 1)),
        ((10.0, 10.0), (10, 10)),
        ((10.0001, 10.0001), (10, 10)),
        ((10.4999, 10.4999), (10, 10)),
        ((10.5, 10.5), (11, 11)),
        ((10.9999, 10.9999), (11, 11)),
        ((-10.0, -10.0), (-10, -10)),
        ((-10.0001, -10.0001), (-10, -10)),
        ((-10.4999, -10.4999), (-10, -10)),
        ((-10.5, -10.5), (-11, -11)),
        ((-10.9999, -10.9999), (-11, -11)),
    ];
    for ((x, y), (ox, oy)) in rows {
        assert_eq!(round_point(PointF { x, y }), Point { x: ox, y: oy });
    }
}

#[test]
fn size_conversion_original_assertion_rows() {
    let floor_rows = [
        (0., 0),
        (0.0001, 0),
        (0.4999, 0),
        (0.5, 0),
        (0.9999, 0),
        (10., 10),
        (10.0001, 10),
        (10.4999, 10),
        (10.5, 10),
        (10.9999, 10),
    ];
    let ceil_rows = [
        (0., 0),
        (0.0001, 1),
        (0.4999, 1),
        (0.5, 1),
        (0.9999, 1),
        (10., 10),
        (10.0001, 11),
        (10.4999, 11),
        (10.5, 11),
        (10.9999, 11),
    ];
    let round_rows = [
        (0., 0),
        (0.0001, 0),
        (0.4999, 0),
        (0.5, 1),
        (0.9999, 1),
        (10., 10),
        (10.0001, 10),
        (10.4999, 10),
        (10.5, 11),
        (10.9999, 11),
    ];
    for (v, o) in floor_rows {
        assert_eq!(floor_size(SizeF { width: v, height: v }), Size { width: o, height: o });
    }
    for (v, o) in ceil_rows {
        assert_eq!(ceil_size(SizeF { width: v, height: v }), Size { width: o, height: o });
    }
    for (v, o) in round_rows {
        assert_eq!(round_size(SizeF { width: v, height: v }), Size { width: o, height: o });
    }
}

#[test]
fn conversion_edge_matrix() {
    // Native constructor/setter clamp is distinct from later InvScale.
    assert_eq!(SizeF::constructed(f32::EPSILON, -1.0), SizeF { width: 0.0, height: 0.0 });
    assert_eq!(
        RectF::constructed(1.0, 2.0, f32::EPSILON, f32::NAN),
        RectF { x: 1.0, y: 2.0, width: 0.0, height: 0.0 }
    );
    assert_eq!(floor_point(PointF { x: -0.1, y: -1.5 }), Point { x: -1, y: -2 });
    assert_eq!(ceil_point(PointF { x: -0.1, y: -1.5 }), Point { x: 0, y: -1 });
    assert_eq!(round_point(PointF { x: -0.5, y: 0.5 }), Point { x: -1, y: 1 });
    assert_eq!(
        floor_point(PointF { x: f32::NAN, y: f32::NEG_INFINITY }),
        Point { x: 0, y: i32::MIN }
    );
    assert_eq!(ceil_point(PointF { x: f32::INFINITY, y: f32::NAN }), Point { x: i32::MAX, y: 0 });
    assert_eq!(
        round_point(PointF { x: f32::MAX, y: -f32::MAX }),
        Point { x: i32::MAX, y: i32::MIN }
    );
    assert_eq!(
        round_point(PointF { x: 2147483520.0, y: -2147483648.0 }),
        // 2147483520 is the actual f32 value and is below i32::MAX.
        Point { x: 2147483520, y: i32::MIN }
    );
    assert_eq!(
        round_point(PointF { x: 2147483648.0, y: -2147483648.0 }),
        Point { x: i32::MAX, y: i32::MIN }
    );
    assert_eq!(
        enclosing_rect(RectF { x: 5.5, y: 5.5, width: 0.0, height: 0.0 }),
        Rect { x: 5, y: 5, width: 0, height: 0 }
    );
    assert_eq!(
        enclosing_rect(RectF::constructed(3.5, 2.5, f32::EPSILON, -0.0)),
        Rect { x: 3, y: 2, width: 0, height: 0 }
    );
    assert_eq!(
        enclosing_rect(RectF { x: 3.5, y: 2.5, width: 0.0, height: 0.001 }),
        Rect { x: 3, y: 2, width: 0, height: 1 }
    );
    // Native Rect(x,y,w,h) clamps positive extents at positive origins.
    let overflow_floor = floored_rect_deprecated(RectF {
        x: 2147483520.0,
        y: 2147483520.0,
        width: 256.0,
        height: 512.0,
    });
    assert_eq!(overflow_floor, Rect { x: 2147483520, y: 2147483520, width: 127, height: 127 });
    let scaled_overflow_floor = scale_enclosing_rect(overflow_floor, 2.0, 2.0);
    assert_eq!(scaled_overflow_floor, Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 });
    // Native constructor clamps, but a later InvScale preserves tiny fields.
    let constructed = RectF::constructed(0.0, 0.0, 1.0, 1.0);
    let inv_scaled = constructed.inv_scale(2097152.0, 2097152.0);
    assert_eq!(inv_scaled.width, 1.0 / 2097152.0);
    assert_eq!(enclosing_rect(inv_scaled), Rect { x: 0, y: 0, width: 1, height: 1 });
    let negative_inv_scaled = constructed.inv_scale(-2.0, -2.0);
    assert_eq!((negative_inv_scaled.width, negative_inv_scaled.height), (-0.5, -0.5));
    assert_eq!(enclosing_rect(negative_inv_scaled), Rect::default());
    let infinite_inv_scaled = constructed.inv_scale(0.0, 0.0);
    assert!(infinite_inv_scaled.width.is_infinite() && infinite_inv_scaled.height.is_infinite());
    assert_eq!(
        enclosing_rect(infinite_inv_scaled),
        Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX }
    );
    let nan_inv_scaled = RectF::constructed(2.0, 2.0, 0.0, 0.0).inv_scale(0.0, 0.0);
    assert!(nan_inv_scaled.width.is_nan() && nan_inv_scaled.height.is_nan());
    assert_eq!(enclosing_rect(nan_inv_scaled), Rect { x: 2, y: 2, width: 0, height: 0 });
    let size_tiny = SizeF::constructed(1.0, 1.0).inv_scale(2097152.0, 2097152.0);
    assert_eq!(ceil_size(size_tiny), Size { width: 1, height: 1 });
    let size_negative = SizeF::constructed(1.0, 1.0).inv_scale(-2.0, -2.0);
    assert_eq!(floor_size(size_negative), Size { width: 0, height: 0 });
    let size_infinite = SizeF::constructed(1.0, 1.0).inv_scale(0.0, 0.0);
    assert_eq!(ceil_size(size_infinite), Size { width: i32::MAX, height: i32::MAX });
    let size_nan = SizeF::constructed(0.0, 0.0).inv_scale(0.0, 0.0);
    assert!(size_nan.width.is_nan() && size_nan.height.is_nan());
    assert_eq!(ceil_size(size_nan), Size::default());
    assert_eq!(
        enclosing_rect(RectF { x: -1.5, y: -1.5, width: 3.0, height: 3.0 }),
        Rect { x: -2, y: -2, width: 4, height: 4 }
    );
    assert_eq!(
        enclosed_rect(RectF { x: 20000.5, y: 20000.5, width: 0.5, height: 0.5 }),
        Rect { x: 20001, y: 20001, width: 0, height: 0 }
    );
    assert_eq!(
        enclosing_rect_ignoring_error(
            RectF { x: 1.9999, y: 2.0002, width: 5.9998, height: 6.0001 },
            0.001
        ),
        Rect { x: 2, y: 2, width: 6, height: 6 }
    );
    assert_eq!(
        enclosed_rect_ignoring_error(
            RectF { x: 1.9999, y: 2.0002, width: 5.9998, height: 6.0001 },
            0.001
        ),
        Rect { x: 2, y: 2, width: 6, height: 6 }
    );
    assert_eq!(
        nearest_rect(RectF { x: -1.00001, y: -0.999999, width: 3.0000001, height: 2.999999 }),
        Rect { x: -1, y: -1, width: 3, height: 3 }
    );
    assert!(is_nearest_rect_within_distance(
        RectF { x: 0.001, y: 0.001, width: 1.0, height: 1.0 },
        0.01
    ));
    assert!(!is_nearest_rect_within_distance(
        RectF { x: 0.1, y: 0.0, width: 1.0, height: 1.0 },
        0.01
    ));
    assert_eq!(
        scale_enclosing_rect(Rect { x: 1, y: 2, width: 3, height: 4 }, 1.5, 1.5),
        Rect { x: 1, y: 3, width: 5, height: 6 }
    );
    assert_eq!(
        scale_enclosed_rect(Rect { x: 1, y: 2, width: 3, height: 4 }, 1.5, 1.5),
        Rect { x: 2, y: 3, width: 4, height: 6 }
    );
    assert_eq!(
        scale_rounded_rect(Rect { x: 1, y: 2, width: 3, height: 4 }, 1.5, 1.5),
        Rect { x: 2, y: 3, width: 4, height: 6 }
    );
    assert_eq!(
        scale_enclosing_rect(Rect { x: -1, y: -2, width: 0, height: 0 }, 1.5, 1.5),
        Rect { x: -2, y: -3, width: 0, height: 0 }
    );
}

// Rectangle conversion parity. Each numbered case cites the exact frozen C++
// EXPECT row, so future edits can compare changes without relying on names.
#[test]
fn every_original_rect_conversion_assertion_row() {
    // RC-E01..E10: rect_conversions_unittest.cc:23-41, ToEnclosedRect.
    let enclosed = [
        (RectF::default(), Rect::default()), // RC-E01 :23
        (
            RectF { x: -1.5, y: -1.5, width: 3.0, height: 3.0 },
            Rect { x: -1, y: -1, width: 2, height: 2 },
        ), // RC-E02 :24-25
        (
            RectF { x: -1.5, y: -1.5, width: 3.5, height: 3.5 },
            Rect { x: -1, y: -1, width: 3, height: 3 },
        ), // RC-E03 :26-27
        (
            RectF { x: f32::MAX, y: f32::MAX, width: 2.0, height: 2.0 },
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // RC-E04 :28-29
        (
            RectF { x: 0.0, y: 0.0, width: f32::MAX, height: f32::MAX },
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
        ), // RC-E05 :30-31
        (
            RectF { x: 20000.5, y: 20000.5, width: 0.5, height: 0.5 },
            Rect { x: 20001, y: 20001, width: 0, height: 0 },
        ), // RC-E06 :32-33
        (
            RectF { x: 2147483648.0, y: 2147483648.0, width: 2147483648.0, height: 2147483648.0 },
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // RC-E07 :34-35
        (
            RectF { x: 1.9999, y: 2.0002, width: 5.9998, height: 6.0001 },
            Rect { x: 2, y: 3, width: 5, height: 5 },
        ), // RC-E08 :36-37
        (
            RectF { x: 1.9999, y: 2.0001, width: 6.0002, height: 5.9998 },
            Rect { x: 2, y: 3, width: 6, height: 4 },
        ), // RC-E09 :38-39
        (
            RectF { x: 1.9998, y: 2.0002, width: 6.0001, height: 5.9999 },
            Rect { x: 2, y: 3, width: 5, height: 5 },
        ), // RC-E10 :40-41
    ];
    for (input, expected) in enclosed {
        assert_eq!(enclosed_rect(input), expected);
    }

    // RC-EH: rect_conversions_unittest.cc:45-56. C++ specifies max spans and
    // near-origin/positive far edge; assert exact safe-range repair as well.
    let huge =
        RectF { x: -2147483648.0, y: -2147483648.0, width: 6442450944.0, height: 6442450944.0 };
    let huge_out = enclosed_rect(huge);
    assert_eq!(
        huge_out,
        Rect { x: -1073741825, y: -1073741825, width: i32::MAX, height: i32::MAX }
    );
    assert_eq!(huge_out.width, i32::MAX); // RC-EH01 original :49
    assert_eq!(huge_out.height, i32::MAX); // RC-EH02 original :50
    assert_eq!(huge_out.x, -1073741825); // RC-EH03 original :52, exact strengthening of x < 1
    assert_eq!(huge_out.y, -1073741825); // RC-EH04 original :53, exact strengthening of y < 1
    assert_eq!(huge_out.x + huge_out.width, 1073741822); // RC-EH05 original :55, exact positive right
    assert_eq!(huge_out.y + huge_out.height, 1073741822); // RC-EH06 original :56, exact positive bottom

    // RC-N01..N03: rect_conversions_unittest.cc:144-150, ToNearestRect.
    assert_eq!(nearest_rect(RectF::default()), Rect::default()); // RC-N01 :144
    assert_eq!(
        nearest_rect(RectF { x: -1.0, y: -1.0, width: 3.0, height: 3.0 }),
        Rect { x: -1, y: -1, width: 3, height: 3 }
    ); // RC-N02 :146-147
    assert_eq!(
        nearest_rect(RectF { x: -1.00001, y: -0.999999, width: 3.0000001, height: 2.999999 }),
        Rect { x: -1, y: -1, width: 3, height: 3 }
    ); // RC-N03 :149-150
    let off_grid = RectF { x: 0.49, y: 0.49, width: 0.49, height: 0.49 };
    assert!(!is_nearest_rect_within_distance(off_grid, 0.01));
    // RoundedRect is intentionally usable outside ToNearestRect's DCHECK band.
    assert_eq!(rounded_rect(off_grid), Rect { x: 0, y: 0, width: 1, height: 1 });

    // RC-F01..F04: rect_conversions_unittest.cc:154-160, deprecated floor.
    let floored = [
        (RectF::default(), Rect::default()), // RC-F01 :154
        (
            RectF { x: -1.5, y: -1.5, width: 3.0, height: 3.0 },
            Rect { x: -2, y: -2, width: 3, height: 3 },
        ), // RC-F02 :155-156
        (
            RectF { x: -1.5, y: -1.5, width: 3.5, height: 3.5 },
            Rect { x: -2, y: -2, width: 3, height: 3 },
        ), // RC-F03 :157-158
        (
            RectF { x: 20000.5, y: 20000.5, width: 0.5, height: 0.5 },
            Rect { x: 20000, y: 20000, width: 0, height: 0 },
        ), // RC-F04 :159-160
    ];
    for (input, expected) in floored {
        assert_eq!(floored_rect_deprecated(input), expected);
    }
}

#[test]
fn every_original_enclosing_rect_assertion_row() {
    // RC-C01..C14: rect_conversions_unittest.cc:60-84.
    let rows = [
        (RectF::default(), Rect::default()), // RC-C01 :60
        (
            RectF { x: 5.5, y: 5.5, width: 0.0, height: 0.0 },
            Rect { x: 5, y: 5, width: 0, height: 0 },
        ), // RC-C02 :61
        (
            RectF::constructed(3.5, 2.5, f32::EPSILON, -0.0),
            Rect { x: 3, y: 2, width: 0, height: 0 },
        ), // RC-C03 :62-63
        (
            RectF { x: 3.5, y: 2.5, width: 0.0, height: 0.001 },
            Rect { x: 3, y: 2, width: 0, height: 1 },
        ), // RC-C04 :64
        (
            RectF { x: -1.5, y: -1.5, width: 3.0, height: 3.0 },
            Rect { x: -2, y: -2, width: 4, height: 4 },
        ), // RC-C05 :65-66
        (
            RectF { x: -1.5, y: -1.5, width: 3.5, height: 3.5 },
            Rect { x: -2, y: -2, width: 4, height: 4 },
        ), // RC-C06 :67-68
        (
            RectF { x: f32::MAX, y: f32::MAX, width: 2.0, height: 2.0 },
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // RC-C07 :69-70
        (
            RectF { x: 0.0, y: 0.0, width: f32::MAX, height: f32::MAX },
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
        ), // RC-C08 :71-72
        (
            RectF { x: 20000.5, y: 20000.5, width: 0.5, height: 0.5 },
            Rect { x: 20000, y: 20000, width: 1, height: 1 },
        ), // RC-C09 :73-74
        (
            RectF { x: 2147483648.0, y: 2147483648.0, width: 2147483648.0, height: 2147483648.0 },
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // RC-C10 :75-76
        (
            RectF { x: -0.5, y: -0.5, width: 22777712.0, height: 1.0 },
            Rect { x: -1, y: -1, width: 22777713, height: 2 },
        ), // RC-C11 :77-78
        (
            RectF { x: 1.9999, y: 2.0002, width: 5.9998, height: 6.0001 },
            Rect { x: 1, y: 2, width: 7, height: 7 },
        ), // RC-C12 :79-80
        (
            RectF { x: 1.9999, y: 2.0001, width: 6.0002, height: 5.9998 },
            Rect { x: 1, y: 2, width: 8, height: 6 },
        ), // RC-C13 :81-82
        (
            RectF { x: 1.9998, y: 2.0002, width: 6.0001, height: 5.9999 },
            Rect { x: 1, y: 2, width: 7, height: 7 },
        ), // RC-C14 :83-84
    ];
    for (input, expected) in rows {
        assert_eq!(enclosing_rect(input), expected);
    }

    // RC-CH: rect_conversions_unittest.cc:88-99. Exact saturated coordinates.
    let r = enclosing_rect(RectF {
        x: -2147483648.0,
        y: -2147483648.0,
        width: 6442450944.0,
        height: 6442450944.0,
    });
    assert_eq!(r, Rect { x: -1073741825, y: -1073741825, width: i32::MAX, height: i32::MAX });
    assert_eq!(r.width, i32::MAX); // RC-CH01 original :92
    assert_eq!(r.height, i32::MAX); // RC-CH02 original :93
    assert_eq!(r.x, -1073741825); // RC-CH03 original :95
    assert_eq!(r.y, -1073741825); // RC-CH04 original :96
    assert_eq!(r.x + r.width, 1073741822); // RC-CH05 original :98
    assert_eq!(r.y + r.height, 1073741822); // RC-CH06 original :99
}

#[test]
fn every_original_enclosing_ignoring_error_assertion_row() {
    // RC-I01..I14: rect_conversions_unittest.cc:104-139; kError = 0.001f.
    let rows = [
        (RectF::default(), Rect::default()), // RC-I01 :104
        (
            RectF { x: 5.5, y: 5.5, width: 0.0, height: 0.0 },
            Rect { x: 5, y: 5, width: 0, height: 0 },
        ), // RC-I02 :105-106
        (
            RectF::constructed(3.5, 2.5, f32::EPSILON, -0.0),
            Rect { x: 3, y: 2, width: 0, height: 0 },
        ), // RC-I03 :107-109
        (
            RectF { x: 3.5, y: 2.5, width: 0.0, height: 0.001 },
            Rect { x: 3, y: 2, width: 0, height: 1 },
        ), // RC-I04 :110-111
        (
            RectF { x: -1.5, y: -1.5, width: 3.0, height: 3.0 },
            Rect { x: -2, y: -2, width: 4, height: 4 },
        ), // RC-I05 :112-113
        (
            RectF { x: -1.5, y: -1.5, width: 3.5, height: 3.5 },
            Rect { x: -2, y: -2, width: 4, height: 4 },
        ), // RC-I06 :114-115
        (
            RectF { x: f32::MAX, y: f32::MAX, width: 2.0, height: 2.0 },
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // RC-I07 :116-118
        (
            RectF { x: 0.0, y: 0.0, width: f32::MAX, height: f32::MAX },
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
        ), // RC-I08 :119-121
        (
            RectF { x: 20000.5, y: 20000.5, width: 0.5, height: 0.5 },
            Rect { x: 20000, y: 20000, width: 1, height: 1 },
        ), // RC-I09 :122-124
        (
            RectF { x: 2147483648.0, y: 2147483648.0, width: 2147483648.0, height: 2147483648.0 },
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // RC-I10 :125-127
        (
            RectF { x: -0.5, y: -0.5, width: 22777712.0, height: 1.0 },
            Rect { x: -1, y: -1, width: 22777713, height: 2 },
        ), // RC-I11 :128-130
        (
            RectF { x: 1.9999, y: 2.0002, width: 5.9998, height: 6.0001 },
            Rect { x: 2, y: 2, width: 6, height: 6 },
        ), // RC-I12 :131-133
        (
            RectF { x: 1.9999, y: 2.0001, width: 6.0002, height: 5.9998 },
            Rect { x: 2, y: 2, width: 6, height: 6 },
        ), // RC-I13 :134-136
        (
            RectF { x: 1.9998, y: 2.0002, width: 6.0001, height: 5.9999 },
            Rect { x: 2, y: 2, width: 6, height: 6 },
        ), // RC-I14 :137-139
    ];
    for (input, expected) in rows {
        assert_eq!(enclosing_rect_ignoring_error(input, 0.001), expected);
    }
}

#[test]
fn every_original_rect_scaling_assertion_row() {
    type Scale = fn(Rect, f32, f32) -> Rect;
    let funcs: [(&str, Scale); 3] = [
        ("ScaleToEnclosedRect", scale_enclosed_rect),
        ("ScaleToEnclosingRect", scale_enclosing_rect),
        ("ScaleToRoundedRect", scale_rounded_rect),
    ];
    // Each S row is present for all three functions, matching rect_unittest.cc
    // lines 455-487; the code asserts exact origin, dimensions, including empty.
    let regular = [
        (Rect::default(), 5.0, 5.0, Rect::default(), Rect::default(), Rect::default()), // S01 :455/467/479
        (
            Rect { x: 1, y: 1, width: 1, height: 1 },
            5.0,
            5.0,
            Rect { x: 5, y: 5, width: 5, height: 5 },
            Rect { x: 5, y: 5, width: 5, height: 5 },
            Rect { x: 5, y: 5, width: 5, height: 5 },
        ), // S02 :456/468/480
        (
            Rect { x: -1, y: -1, width: 0, height: 0 },
            5.0,
            5.0,
            Rect { x: -5, y: -5, width: 0, height: 0 },
            Rect { x: -5, y: -5, width: 0, height: 0 },
            Rect { x: -5, y: -5, width: 0, height: 0 },
        ), // S03 :457/469/481
        (
            Rect { x: 1, y: -1, width: 0, height: 1 },
            5.0,
            5.0,
            Rect { x: 5, y: -5, width: 0, height: 5 },
            Rect { x: 5, y: -5, width: 0, height: 5 },
            Rect { x: 5, y: -5, width: 0, height: 5 },
        ), // S04 :458/470/482
        (
            Rect { x: -1, y: 1, width: 1, height: 0 },
            5.0,
            5.0,
            Rect { x: -5, y: 5, width: 5, height: 0 },
            Rect { x: -5, y: 5, width: 5, height: 0 },
            Rect { x: -5, y: 5, width: 5, height: 0 },
        ), // S05 :459/471/483
        (
            Rect { x: 1, y: 2, width: 3, height: 4 },
            1.5,
            1.5,
            Rect { x: 2, y: 3, width: 4, height: 6 },
            Rect { x: 1, y: 3, width: 5, height: 6 },
            Rect { x: 2, y: 3, width: 4, height: 6 },
        ), // S06 :460/472/484
        (
            Rect { x: -1, y: -2, width: 0, height: 0 },
            1.5,
            1.5,
            Rect { x: -1, y: -3, width: 0, height: 0 },
            Rect { x: -2, y: -3, width: 0, height: 0 },
            Rect { x: -2, y: -3, width: 0, height: 0 },
        ), // S07 :461/473/485
        (
            Rect { x: 2, y: 4, width: 9, height: 8 },
            0.3,
            0.3,
            Rect { x: 1, y: 2, width: 2, height: 1 },
            Rect { x: 0, y: 1, width: 4, height: 3 },
            Rect { x: 1, y: 1, width: 2, height: 3 },
        ), // S08 :462/474/486
    ];
    for (name, f, slot) in [
        ("ScaleToEnclosedRect", scale_enclosed_rect as fn(Rect, f32, f32) -> Rect, 0usize),
        ("ScaleToEnclosingRect", scale_enclosing_rect, 1usize),
        ("ScaleToRoundedRect", scale_rounded_rect, 2usize),
    ] {
        for (rect, sx, sy, expected_enclosed, expected_enclosing, expected_rounded) in regular {
            let expected = [expected_enclosed, expected_enclosing, expected_rounded][slot];
            assert_eq!(f(rect, sx, sy), expected, "{name}: {rect:?}");
        }
    }

    // Shared TestScaleRectOverflowClamp: rect_unittest.cc:401-451, invoked at
    // lines 463, 475 and 487. These 13 exact expected Rects are asserted for
    // every conversion function (39 translated assertions).
    let maxf = i32::MAX as f32;
    // Constructor clamp assertion at rect_unittest.cc:432-434.
    let constructed_max_origin = rect_from_xywh(i32::MAX, i32::MAX, i32::MAX, i32::MAX);
    assert_eq!(constructed_max_origin, Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 });
    let common = [
        (
            Rect { x: -100000, y: -123456, width: 10, height: 20 },
            100000.0,
            100000.0,
            Rect { x: i32::MIN, y: i32::MIN, width: 0, height: 0 },
        ), // O01 :401-403
        (
            Rect { x: -100000, y: -123456, width: 100000, height: 123456 },
            100000.0,
            100000.0,
            Rect { x: -i32::MAX, y: -i32::MAX, width: i32::MAX, height: i32::MAX },
        ), // O02 :407-409
        (
            Rect { x: 100000, y: 123456, width: 10, height: 20 },
            100000.0,
            100000.0,
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // O03 :413-415
        (
            Rect { x: -1, y: -2, width: 100000, height: 100000 },
            -100000.0,
            -100000.0,
            Rect { x: 100000, y: 200000, width: 0, height: 0 },
        ), // O04 :419-421
        (
            Rect { x: -1, y: -2, width: 123456, height: 234567 },
            100000.0,
            100000.0,
            Rect { x: -100000, y: -200000, width: i32::MAX, height: i32::MAX },
        ), // O05 :423-425
        (
            Rect { x: 1, y: 2, width: 123456, height: 234567 },
            100000.0,
            100000.0,
            Rect { x: 100000, y: 200000, width: i32::MAX - 100000, height: i32::MAX - 200000 },
        ), // O06 :427-429
        (
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
            maxf,
            maxf,
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // O07 :432-436
        (
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
            2.0,
            3.5,
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
        ), // O08 :438-440
        (
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
            maxf,
            maxf,
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
        ), // O09 :441-442
        (
            Rect { x: 0, y: 0, width: i32::MAX, height: i32::MAX },
            i32::MIN as f32,
            i32::MIN as f32,
            Rect::default(),
        ), // O10 :443-444
        (
            Rect { x: -i32::MAX, y: -i32::MAX, width: i32::MAX, height: i32::MAX },
            2.0,
            3.5,
            Rect { x: -i32::MAX, y: -i32::MAX, width: i32::MAX, height: i32::MAX },
        ), // O11 :446-448
        (
            Rect { x: -i32::MAX, y: -i32::MAX, width: i32::MAX, height: i32::MAX },
            maxf,
            maxf,
            Rect { x: -i32::MAX, y: -i32::MAX, width: i32::MAX, height: i32::MAX },
        ), // O12 :449
        (
            Rect { x: -i32::MAX, y: -i32::MAX, width: i32::MAX, height: i32::MAX },
            i32::MIN as f32,
            i32::MIN as f32,
            Rect { x: i32::MAX, y: i32::MAX, width: 0, height: 0 },
        ), // O13 :450-451
    ];
    for (name, f) in funcs {
        for (rect, sx, sy, expected) in common {
            assert_eq!(f(rect, sx, sy), expected, "{name}: {rect:?} scale {sx:?},{sy:?}");
        }
    }
}
