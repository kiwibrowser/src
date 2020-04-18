// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/applied_decoration_painter.h"

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"

namespace blink {

namespace {

static StrokeStyle TextDecorationStyleToStrokeStyle(
    ETextDecorationStyle decoration_style) {
  StrokeStyle stroke_style = kSolidStroke;
  switch (decoration_style) {
    case ETextDecorationStyle::kSolid:
      stroke_style = kSolidStroke;
      break;
    case ETextDecorationStyle::kDouble:
      stroke_style = kDoubleStroke;
      break;
    case ETextDecorationStyle::kDotted:
      stroke_style = kDottedStroke;
      break;
    case ETextDecorationStyle::kDashed:
      stroke_style = kDashedStroke;
      break;
    case ETextDecorationStyle::kWavy:
      stroke_style = kWavyStroke;
      break;
  }

  return stroke_style;
}

static void AdjustStepToDecorationLength(float& step,
                                         float& control_point_distance,
                                         float length) {
  DCHECK_GT(step, 0);

  if (length <= 0)
    return;

  unsigned step_count = static_cast<unsigned>(length / step);

  // Each Bezier curve starts at the same pixel that the previous one
  // ended. We need to subtract (stepCount - 1) pixels when calculating the
  // length covered to account for that.
  float uncovered_length = length - (step_count * step - (step_count - 1));
  float adjustment = uncovered_length / step_count;
  step += adjustment;
  control_point_distance += adjustment;
}

}  // anonymous namespace

Path AppliedDecorationPainter::PrepareDottedDashedStrokePath() {
  // These coordinate transforms need to match what's happening in
  // GraphicsContext's drawLineForText and drawLine.
  int y = floorf(start_point_.Y() +
                 std::max<float>(decoration_info_.thickness / 2.0f, 0.5f));
  Path stroke_path;
  FloatPoint rounded_start_point(start_point_.X(), y);
  FloatPoint rounded_end_point(rounded_start_point +
                               FloatPoint(decoration_info_.width, 0));
  context_.AdjustLineToPixelBoundaries(rounded_start_point, rounded_end_point,
                                       roundf(decoration_info_.thickness));
  stroke_path.MoveTo(rounded_start_point);
  stroke_path.AddLineTo(rounded_end_point);
  return stroke_path;
}

FloatRect AppliedDecorationPainter::Bounds() {
  StrokeData stroke_data;
  stroke_data.SetThickness(decoration_info_.thickness);

  switch (decoration_.Style()) {
    case ETextDecorationStyle::kDotted:
    case ETextDecorationStyle::kDashed: {
      stroke_data.SetStyle(
          TextDecorationStyleToStrokeStyle(decoration_.Style()));
      return PrepareDottedDashedStrokePath().StrokeBoundingRect(stroke_data);
    }
    case ETextDecorationStyle::kWavy:
      return PrepareWavyStrokePath().StrokeBoundingRect(stroke_data);
      break;
    case ETextDecorationStyle::kDouble:
      if (double_offset_ > 0) {
        return FloatRect(start_point_.X(), start_point_.Y(),
                         decoration_info_.width,
                         double_offset_ + decoration_info_.thickness);
      }
      return FloatRect(start_point_.X(), start_point_.Y() + double_offset_,
                       decoration_info_.width,
                       -double_offset_ + decoration_info_.thickness);
      break;
    case ETextDecorationStyle::kSolid:
      return FloatRect(start_point_.X(), start_point_.Y(),
                       decoration_info_.width, decoration_info_.thickness);
    default:
      break;
  }
  NOTREACHED();
  return FloatRect();
}

void AppliedDecorationPainter::Paint() {
  context_.SetStrokeStyle(
      TextDecorationStyleToStrokeStyle(decoration_.Style()));
  context_.SetStrokeColor(decoration_.GetColor());

  switch (decoration_.Style()) {
    case ETextDecorationStyle::kWavy:
      StrokeWavyTextDecoration();
      break;
    case ETextDecorationStyle::kDotted:
    case ETextDecorationStyle::kDashed:
      context_.SetShouldAntialias(decoration_info_.antialias);
      FALLTHROUGH;
    default:
      context_.DrawLineForText(start_point_, decoration_info_.width);

      if (decoration_.Style() == ETextDecorationStyle::kDouble) {
        context_.DrawLineForText(start_point_ + FloatPoint(0, double_offset_),
                                 decoration_info_.width);
      }
  }
}

void AppliedDecorationPainter::StrokeWavyTextDecoration() {
  context_.SetShouldAntialias(true);
  context_.StrokePath(PrepareWavyStrokePath());
}

/*
 * Prepare a path for a cubic Bezier curve and repeat the same pattern long the
 * the decoration's axis.  The start point (p1), controlPoint1, controlPoint2
 * and end point (p2) of the Bezier curve form a diamond shape:
 *
 *                              step
 *                         |-----------|
 *
 *                   controlPoint1
 *                         +
 *
 *
 *                  . .
 *                .     .
 *              .         .
 * (x1, y1) p1 +           .            + p2 (x2, y2) - <--- Decoration's axis
 *                          .         .               |
 *                            .     .                 |
 *                              . .                   | controlPointDistance
 *                                                    |
 *                                                    |
 *                         +                          -
 *                   controlPoint2
 *
 *             |-----------|
 *                 step
 */
Path AppliedDecorationPainter::PrepareWavyStrokePath() {
  FloatPoint p1(start_point_ +
                FloatPoint(0, double_offset_ * wavy_offset_factor_));
  FloatPoint p2(
      start_point_ +
      FloatPoint(decoration_info_.width, double_offset_ * wavy_offset_factor_));

  context_.AdjustLineToPixelBoundaries(p1, p2, decoration_info_.thickness);

  Path path;
  path.MoveTo(p1);

  // Distance between decoration's axis and Bezier curve's control points.
  // The height of the curve is based on this distance. Use a minimum of 6
  // pixels distance since
  // the actual curve passes approximately at half of that distance, that is 3
  // pixels.
  // The minimum height of the curve is also approximately 3 pixels. Increases
  // the curve's height
  // as strockThickness increases to make the curve looks better.
  float control_point_distance =
      3 * std::max<float>(2, decoration_info_.thickness);

  // Increment used to form the diamond shape between start point (p1), control
  // points and end point (p2) along the axis of the decoration. Makes the
  // curve wider as strockThickness increases to make the curve looks better.
  float step = 2 * std::max<float>(2, decoration_info_.thickness);

  bool is_vertical_line = (p1.X() == p2.X());

  if (is_vertical_line) {
    DCHECK(p1.X() == p2.X());

    float x_axis = p1.X();
    float y1;
    float y2;

    if (p1.Y() < p2.Y()) {
      y1 = p1.Y();
      y2 = p2.Y();
    } else {
      y1 = p2.Y();
      y2 = p1.Y();
    }

    AdjustStepToDecorationLength(step, control_point_distance, y2 - y1);
    FloatPoint control_point1(x_axis + control_point_distance, 0);
    FloatPoint control_point2(x_axis - control_point_distance, 0);

    for (float y = y1; y + 2 * step <= y2;) {
      control_point1.SetY(y + step);
      control_point2.SetY(y + step);
      y += 2 * step;
      path.AddBezierCurveTo(control_point1, control_point2,
                            FloatPoint(x_axis, y));
    }
  } else {
    DCHECK(p1.Y() == p2.Y());

    float y_axis = p1.Y();
    float x1;
    float x2;

    if (p1.X() < p2.X()) {
      x1 = p1.X();
      x2 = p2.X();
    } else {
      x1 = p2.X();
      x2 = p1.X();
    }

    AdjustStepToDecorationLength(step, control_point_distance, x2 - x1);
    FloatPoint control_point1(0, y_axis + control_point_distance);
    FloatPoint control_point2(0, y_axis - control_point_distance);

    for (float x = x1; x + 2 * step <= x2;) {
      control_point1.SetX(x + step);
      control_point2.SetX(x + step);
      x += 2 * step;
      path.AddBezierCurveTo(control_point1, control_point2,
                            FloatPoint(x, y_axis));
    }
  }
  return path;
}

}  // namespace blink
