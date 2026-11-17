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

impl SizeF {
    /// Models a native SizeF constructor/setter. Native InvScale mutates the
    /// stored fields directly and therefore does not reapply this clamp.
    pub fn constructed(width: f32, height: f32) -> Self {
        let threshold = 8.0 * f32::EPSILON;
        Self {
            width: if width > threshold { width } else { 0.0 },
            height: if height > threshold { height } else { 0.0 },
        }
    }

    /// Mirrors SizeF::InvScale: direct division of the stored dimensions.
    pub fn inv_scale(self, x_scale: f32, y_scale: f32) -> Self {
        Self { width: self.width / x_scale, height: self.height / y_scale }
    }
}

impl RectF {
    /// Models the native RectF constructor, which constructs a clamped SizeF.
    pub fn constructed(x: f32, y: f32, width: f32, height: f32) -> Self {
        let size = SizeF::constructed(width, height);
        Self { x, y, width: size.width, height: size.height }
    }

    /// Mirrors RectF::InvScale and retains tiny, negative, infinite and NaN
    /// dimensions produced by that post-construction operation.
    pub fn inv_scale(self, x_scale: f32, y_scale: f32) -> Self {
        Self {
            x: self.x / x_scale,
            y: self.y / y_scale,
            width: self.width / x_scale,
            height: self.height / y_scale,
        }
    }
}

impl Rect {
    /// Scale helpers rely on the invariants maintained by the native Rect
    /// setters. Public fields make invalid values constructible in this draft,
    /// so validate at the boundary before using native-equivalent arithmetic.
    fn is_valid_native(self) -> bool {
        self.width >= 0
            && self.height >= 0
            && (self.x as i64 + self.width as i64) <= i32::MAX as i64
            && (self.y as i64 + self.height as i64) <= i32::MAX as i64
    }
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
    Size {
        width: convert(s.width, Mode::Floor).max(0),
        height: convert(s.height, Mode::Floor).max(0),
    }
}
pub fn ceil_size(s: SizeF) -> Size {
    Size {
        width: convert(s.width, Mode::Ceil).max(0),
        height: convert(s.height, Mode::Ceil).max(0),
    }
}
pub fn round_size(s: SizeF) -> Size {
    Size {
        width: convert(s.width, Mode::Round).max(0),
        height: convert(s.height, Mode::Round).max(0),
    }
}

#[inline]
fn clamp_i32(v: i64) -> i32 {
    v.clamp(i32::MIN as i64, i32::MAX as i64) as i32
}

#[inline]
fn clamp_add(a: i32, b: i32) -> i32 {
    clamp_i32(a as i64 + b as i64)
}

#[inline]
fn clamp_sub(a: i32, b: i32) -> i32 {
    clamp_i32(a as i64 - b as i64)
}

// Exact safe-integer equivalent of Rect's SaturatedClampRange in rect.cc.
// abs is widened first, so i32::MIN is representable.
fn saturated_clamp_range(min: i32, max: i32) -> (i32, i32) {
    if max < min {
        return (min, 0);
    }
    let span = clamp_sub(max, min);
    let span_loss = clamp_sub(max, clamp_add(min, span));
    if span_loss == 0 {
        return (min, span);
    }
    let threshold = (i32::MAX / 2) as u64;
    if (max as i64).unsigned_abs() < threshold {
        (clamp_sub(max, span), span)
    } else if (min as i64).unsigned_abs() < threshold {
        (min, span)
    } else {
        // C++ signed division truncates toward zero; span_loss is nonnegative.
        (clamp_add(min, span_loss / 2), span)
    }
}

fn rect_axis_bounds(min: i32, max: i32) -> (i32, i32) {
    let mut origin = min;
    let mut span = clamp_sub(max, min).max(0);
    // Rect::set_width clamps the dimension so origin + span stays in i32.
    span = span.min(clamp_add(origin, span) - origin);
    if clamp_add(origin, span) != max {
        (origin, span) = saturated_clamp_range(origin, max);
        span = span.min(clamp_add(origin, span) - origin);
    }
    (origin, span)
}

pub fn rect_by_bounds(left: i32, top: i32, right: i32, bottom: i32) -> Rect {
    // Mirrors SetHorizontalBounds/SetVerticalBounds and their saturation repair.
    let (x, width) = rect_axis_bounds(left, right);
    let (y, height) = rect_axis_bounds(top, bottom);
    Rect { x, y, width, height }
}

/// Mirrors Rect(x, y, width, height), including Size's nonnegative dimensions
/// and Rect::ClampWidthOrHeight's representable right/bottom constraint.
pub fn rect_from_xywh(x: i32, y: i32, width: i32, height: i32) -> Rect {
    let width = clamp_add(x, width) - x;
    let height = clamp_add(y, height) - y;
    Rect { x, y, width: width.max(0), height: height.max(0) }
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
    // Preserve C++ truthiness (`-0.0` and both zero signs are false).
    let right = if r.width != 0.0 { convert(r.x + r.width, Mode::Ceil) } else { l };
    let t = convert(r.y, Mode::Floor);
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
    let right = if r.width != 0.0 { ceil_ignoring_error(r.x + r.width, error) } else { l };
    let t = floor_ignoring_error(r.y, error);
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
    let left_value = r.x;
    let top_value = r.y;
    let right_value = r.x + r.width;
    let bottom_value = r.y + r.height;
    let left = convert(r.x, Mode::Round);
    let top = convert(r.y, Mode::Round);
    let right = convert(right_value, Mode::Round);
    let bottom = convert(bottom_value, Mode::Round);
    // Mirrors ToNearestRect's strict DCHECK precondition. RoundedRect remains
    // separate and intentionally remains usable outside this tolerance.
    debug_assert!(((left as f32) - left_value).abs() < 0.01);
    debug_assert!(((top as f32) - top_value).abs() < 0.01);
    debug_assert!(((right as f32) - right_value).abs() < 0.01);
    debug_assert!(((bottom as f32) - bottom_value).abs() < 0.01);
    rect_by_bounds(left, top, right, bottom)
}
pub fn rounded_rect(r: RectF) -> Rect {
    let left = convert(r.x, Mode::Round);
    let top = convert(r.y, Mode::Round);
    let right = convert(r.x + r.width, Mode::Round);
    let bottom = convert(r.y + r.height, Mode::Round);
    rect_by_bounds(left, top, right, bottom)
}
pub fn floored_rect_deprecated(r: RectF) -> Rect {
    rect_from_xywh(
        convert(r.x, Mode::Floor),
        convert(r.y, Mode::Floor),
        convert(r.width, Mode::Floor),
        convert(r.height, Mode::Floor),
    )
}

pub fn is_nearest_rect_within_distance(r: RectF, distance: f32) -> bool {
    let left_value = r.x;
    let top_value = r.y;
    let right_value = r.x + r.width;
    let bottom_value = r.y + r.height;
    let left = convert(left_value, Mode::Round);
    let top = convert(top_value, Mode::Round);
    let right = convert(right_value, Mode::Round);
    let bottom = convert(bottom_value, Mode::Round);
    ((left as f32) - left_value).abs() < distance
        && ((top as f32) - top_value).abs() < distance
        && ((right as f32) - right_value).abs() < distance
        && ((bottom as f32) - bottom_value).abs() < distance
}

pub fn scale_enclosing_rect(r: Rect, sx: f32, sy: f32) -> Rect {
    assert!(r.is_valid_native(), "scale input must satisfy native Rect invariants");
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
    assert!(r.is_valid_native(), "scale input must satisfy native Rect invariants");
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
    assert!(r.is_valid_native(), "scale input must satisfy native Rect invariants");
    if sx == 1.0 && sy == 1.0 {
        return r;
    }
    let l = convert(r.x as f32 * sx, Mode::Round);
    let t = convert(r.y as f32 * sy, Mode::Round);
    let right = if r.width == 0 { l } else { convert((r.x + r.width) as f32 * sx, Mode::Round) };
    let bottom = if r.height == 0 { t } else { convert((r.y + r.height) as f32 * sy, Mode::Round) };
    rect_by_bounds(l, t, right, bottom)
}
