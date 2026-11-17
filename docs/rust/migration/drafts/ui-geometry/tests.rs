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
        Point { x: i32::MAX, y: i32::MIN }
    );
    assert_eq!(
        enclosing_rect(RectF { x: 5.5, y: 5.5, width: 0.0, height: 0.0 }),
        Rect { x: 5, y: 5, width: 0, height: 0 }
    );
    assert_eq!(
        enclosing_rect(RectF { x: 3.5, y: 2.5, width: f32::EPSILON, height: -0.0 }),
        Rect { x: 3, y: 2, width: 0, height: 0 }
    );
    assert_eq!(
        enclosing_rect(RectF { x: 3.5, y: 2.5, width: 0.0, height: 0.001 }),
        Rect { x: 3, y: 2, width: 0, height: 1 }
    );
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
        Rect { x: 2, y: 3, width: 5, height: 5 }
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
