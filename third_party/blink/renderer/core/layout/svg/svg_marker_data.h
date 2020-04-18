/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_SVG_MARKER_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_SVG_MARKER_DATA_H_

#include "third_party/blink/renderer/platform/graphics/path.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

namespace blink {

enum SVGMarkerType { kStartMarker, kMidMarker, kEndMarker };

struct MarkerPosition {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
  MarkerPosition(SVGMarkerType use_type,
                 const FloatPoint& use_origin,
                 float use_angle)
      : type(use_type), origin(use_origin), angle(use_angle) {}

  SVGMarkerType type;
  FloatPoint origin;
  float angle;
};

class LayoutSVGResourceMarker;

class SVGMarkerData {
  STACK_ALLOCATED();

 public:
  SVGMarkerData(Vector<MarkerPosition>& positions, bool auto_start_reverse)
      : positions_(positions),
        element_index_(0),
        auto_start_reverse_(auto_start_reverse) {}

  static void UpdateFromPathElement(void* info, const PathElement* element) {
    static_cast<SVGMarkerData*>(info)->UpdateFromPathElement(*element);
  }

  void PathIsDone() {
    positions_.push_back(
        MarkerPosition(kEndMarker, origin_, CurrentAngle(kEndMarker)));
  }

  static inline LayoutSVGResourceMarker* MarkerForType(
      const SVGMarkerType& type,
      LayoutSVGResourceMarker* marker_start,
      LayoutSVGResourceMarker* marker_mid,
      LayoutSVGResourceMarker* marker_end) {
    switch (type) {
      case kStartMarker:
        return marker_start;
      case kMidMarker:
        return marker_mid;
      case kEndMarker:
        return marker_end;
    }

    NOTREACHED();
    return nullptr;
  }

 private:
  float CurrentAngle(SVGMarkerType type) const {
    // For details of this calculation, see:
    // http://www.w3.org/TR/SVG/single-page.html#painting-MarkerElement
    FloatPoint in_slope(inslope_points_[1] - inslope_points_[0]);
    FloatPoint out_slope(outslope_points_[1] - outslope_points_[0]);

    double in_angle = rad2deg(in_slope.SlopeAngleRadians());
    double out_angle = rad2deg(out_slope.SlopeAngleRadians());

    switch (type) {
      case kStartMarker:
        if (auto_start_reverse_)
          out_angle += 180;
        return clampTo<float>(out_angle);
      case kMidMarker:
        // WK193015: Prevent bugs due to angles being non-continuous.
        if (fabs(in_angle - out_angle) > 180)
          in_angle += 360;
        return clampTo<float>((in_angle + out_angle) / 2);
      case kEndMarker:
        return clampTo<float>(in_angle);
    }

    NOTREACHED();
    return 0;
  }

  void UpdateOutslope(const PathElement& element) {
    outslope_points_[0] = origin_;
    FloatPoint point = element.type == kPathElementCloseSubpath
                           ? subpath_start_
                           : element.points[0];
    outslope_points_[1] = point;
  }

  void UpdateFromPathElement(const PathElement& element) {
    // First update the outslope for the previous element.
    UpdateOutslope(element);

    // Record the marker for the previous element.
    if (element_index_ > 0) {
      SVGMarkerType marker_type =
          element_index_ == 1 ? kStartMarker : kMidMarker;
      positions_.push_back(
          MarkerPosition(marker_type, origin_, CurrentAngle(marker_type)));
    }

    // Update our marker data for this element.
    UpdateMarkerDataForPathElement(element);
    ++element_index_;
  }

  void UpdateMarkerDataForPathElement(const PathElement& element) {
    const FloatPoint* points = element.points;

    switch (element.type) {
      case kPathElementAddQuadCurveToPoint:
        inslope_points_[0] = points[0];
        inslope_points_[1] = points[1];
        origin_ = points[1];
        break;
      case kPathElementAddCurveToPoint:
        inslope_points_[0] = points[1];
        inslope_points_[1] = points[2];
        origin_ = points[2];
        break;
      case kPathElementMoveToPoint:
        subpath_start_ = points[0];
        FALLTHROUGH;
      case kPathElementAddLineToPoint:
        UpdateInslope(points[0]);
        origin_ = points[0];
        break;
      case kPathElementCloseSubpath:
        UpdateInslope(subpath_start_);
        origin_ = subpath_start_;
        subpath_start_ = FloatPoint();
    }
  }

  void UpdateInslope(const FloatPoint& point) {
    inslope_points_[0] = origin_;
    inslope_points_[1] = point;
  }

  Vector<MarkerPosition>& positions_;
  unsigned element_index_;
  FloatPoint origin_;
  FloatPoint subpath_start_;
  FloatPoint inslope_points_[2];
  FloatPoint outslope_points_[2];
  bool auto_start_reverse_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_SVG_SVG_MARKER_DATA_H_
