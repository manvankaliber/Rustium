//! Candidate for the scalar/point/size/rectangle float-to-integer conversion
//! family. This file deliberately has no Chromium or ABI dependency.
//!
//! Floating arithmetic stays `f32` and keeps the original expression order.
//! Only the final rounded `f32` is widened to `f64` for exact i32 bounds tests;
//! this avoids Rust's saturating float cast's NaN behavior being mistaken for
//! Chromium's behavior and avoids the `i32::MAX as f32 == 2^31` trap.

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Size {
    pub width: i32,
    pub height: i32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct PointF {
    pub x: f32,
    pub y: f32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SizeF {
    pub width: f32,
    pub height: f32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct RectF {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

#[derive(Clone, Copy)]
enum Mode {
    Floor,
    Ceil,
    Round,
}

#[inline]
fn clamp_i32_rounded(v: f32) -> i32 {
    // Chromium saturated_cast<int> maps NaN to 0, infinities/out-of-range to
    // the corresponding limit. `f64` exactly represents both i32 bounds.
    if v.is_nan() {
        return 0;
    }
    let v = v as f64;
    if v <= i32::MIN as f64 {
        i32::MIN
    } else if v >= i32::MAX as f64 {
        i32::MAX
    } else {
        v as i32
    }
}

#[inline]
fn convert(v: f32, mode: Mode) -> i32 {
    // Rust f32::round has the same ties-away-from-zero rule as std::round.
    let rounded = match mode {
        Mode::Floor => v.floor(),
        Mode::Ceil => v.ceil(),
        Mode::Round => v.round(),
    };
    clamp_i32_rounded(rounded)
}

pub fn floor_point(p: PointF) -> Point {
    Point { x: convert(p.x, Mode::Floor), y: convert(p.y, Mode::Floor) }
}
pub fn ceil_point(p: PointF) -> Point {
    Point { x: convert(p.x, Mode::Ceil), y: convert(p.y, Mode::Ceil) }
}
pub fn round_point(p: PointF) -> Point {
    Point { x: convert(p.x, Mode::Round), y: convert(p.y, Mode::Round) }
}
pub fn floor_size(s: SizeF) -> Size {
    Size { width: convert(s.width, Mode::Floor), height: convert(s.height, Mode::Floor) }
}
pub fn ceil_size(s: SizeF) -> Size {
    Size { width: convert(s.width, Mode::Ceil), height: convert(s.height, Mode::Ceil) }
}
pub fn round_size(s: SizeF) -> Size {
    Size { width: convert(s.width, Mode::Round), height: convert(s.height, Mode::Round) }
}

#[inline]
fn width_between(left: i32, right: i32) -> i32 {
    // Mirrors base::ClampSub(right, left), then Rect's nonnegative size rule.
    (right as i64 - left as i64).clamp(0, i32::MAX as i64) as i32
}
pub fn rect_by_bounds(left: i32, top: i32, right: i32, bottom: i32) -> Rect {
    // Rect::SetByBounds may move the origin when a positive extent overflows.
    let width = width_between(left, right);
    let height = width_between(top, bottom);
    let x = (right as i64 - width as i64).max(i32::MIN as i64) as i32;
    let y = (bottom as i64 - height as i64).max(i32::MIN as i64) as i32;
    Rect { x, y, width, height }
}

#[inline]
fn floor_ignoring_error(v: f32, error: f32) -> i32 {
    let nearest = convert(v, Mode::Round);
    if ((nearest as f32) - v).abs() < error {
        nearest
    } else {
        convert(v, Mode::Floor)
    }
}
#[inline]
fn ceil_ignoring_error(v: f32, error: f32) -> i32 {
    let nearest = convert(v, Mode::Round);
    if ((nearest as f32) - v).abs() < error {
        nearest
    } else {
        convert(v, Mode::Ceil)
    }
}

pub fn enclosing_rect(r: RectF) -> Rect {
    let l = convert(r.x, Mode::Floor);
    let t = convert(r.y, Mode::Floor);
    // Preserve C++ truthiness (`-0.0` and both zero signs are false).
    let right = if r.width != 0.0 { convert(r.x + r.width, Mode::Ceil) } else { l };
    let bottom = if r.height != 0.0 { convert(r.y + r.height, Mode::Ceil) } else { t };
    rect_by_bounds(l, t, right, bottom)
}
pub fn enclosed_rect(r: RectF) -> Rect {
    rect_by_bounds(
        convert(r.x, Mode::Ceil),
        convert(r.y, Mode::Ceil),
        convert(r.x + r.width, Mode::Floor),
        convert(r.y + r.height, Mode::Floor),
    )
}
pub fn enclosing_rect_ignoring_error(r: RectF, error: f32) -> Rect {
    let l = floor_ignoring_error(r.x, error);
    let t = floor_ignoring_error(r.y, error);
    let right = if r.width != 0.0 { ceil_ignoring_error(r.x + r.width, error) } else { l };
    let bottom = if r.height != 0.0 { ceil_ignoring_error(r.y + r.height, error) } else { t };
    rect_by_bounds(l, t, right, bottom)
}
pub fn enclosed_rect_ignoring_error(r: RectF, error: f32) -> Rect {
    let l = ceil_ignoring_error(r.x, error);
    let t = ceil_ignoring_error(r.y, error);
    let right = if r.width != 0.0 { floor_ignoring_error(r.x + r.width, error) } else { l };
    let bottom = if r.height != 0.0 { floor_ignoring_error(r.y + r.height, error) } else { t };
    rect_by_bounds(l, t, right, bottom)
}
pub fn nearest_rect(r: RectF) -> Rect {
    rect_by_bounds(
        convert(r.x, Mode::Round),
        convert(r.y, Mode::Round),
        convert(r.x + r.width, Mode::Round),
        convert(r.y + r.height, Mode::Round),
    )
}
pub fn rounded_rect(r: RectF) -> Rect {
    nearest_rect(r)
}
pub fn floored_rect_deprecated(r: RectF) -> Rect {
    Rect {
        x: convert(r.x, Mode::Floor),
        y: convert(r.y, Mode::Floor),
        width: convert(r.width, Mode::Floor).max(0),
        height: convert(r.height, Mode::Floor).max(0),
    }
}

pub fn is_nearest_rect_within_distance(r: RectF, distance: f32) -> bool {
    let edges = [r.x, r.y, r.x + r.width, r.y + r.height];
    edges.into_iter().all(|v| ((convert(v, Mode::Round) as f32) - v).abs() < distance)
}

pub fn scale_enclosing_rect(r: Rect, sx: f32, sy: f32) -> Rect {
    if sx == 1.0 && sy == 1.0 {
        return r;
    }
    let l = convert(r.x as f32 * sx, Mode::Floor);
    let t = convert(r.y as f32 * sy, Mode::Floor);
    let right = if r.width == 0 { l } else { convert((r.x + r.width) as f32 * sx, Mode::Ceil) };
    let bottom = if r.height == 0 { t } else { convert((r.y + r.height) as f32 * sy, Mode::Ceil) };
    rect_by_bounds(l, t, right, bottom)
}
pub fn scale_enclosed_rect(r: Rect, sx: f32, sy: f32) -> Rect {
    if sx == 1.0 && sy == 1.0 {
        return r;
    }
    let l = convert(r.x as f32 * sx, Mode::Ceil);
    let t = convert(r.y as f32 * sy, Mode::Ceil);
    let right = if r.width == 0 { l } else { convert((r.x + r.width) as f32 * sx, Mode::Floor) };
    let bottom = if r.height == 0 { t } else { convert((r.y + r.height) as f32 * sy, Mode::Floor) };
    rect_by_bounds(l, t, right, bottom)
}
pub fn scale_rounded_rect(r: Rect, sx: f32, sy: f32) -> Rect {
    if sx == 1.0 && sy == 1.0 {
        return r;
    }
    let l = convert(r.x as f32 * sx, Mode::Round);
    let t = convert(r.y as f32 * sy, Mode::Round);
    let right = if r.width == 0 { l } else { convert((r.x + r.width) as f32 * sx, Mode::Round) };
    let bottom = if r.height == 0 { t } else { convert((r.y + r.height) as f32 * sy, Mode::Round) };
    rect_by_bounds(l, t, right, bottom)
}
