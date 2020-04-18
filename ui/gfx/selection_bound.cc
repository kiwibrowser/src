// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/selection_bound.h"

namespace gfx {

SelectionBound::SelectionBound() : type_(EMPTY), visible_(false) {}

SelectionBound::SelectionBound(const SelectionBound& other) = default;

SelectionBound::~SelectionBound() {}

void SelectionBound::SetEdgeTop(const gfx::PointF& value) {
  edge_top_ = value;
  edge_top_rounded_ = gfx::ToRoundedPoint(value);
}

void SelectionBound::SetEdgeBottom(const gfx::PointF& value) {
  edge_bottom_ = value;
  edge_bottom_rounded_ = gfx::ToRoundedPoint(value);
}

void SelectionBound::SetEdge(const gfx::PointF& top,
                             const gfx::PointF& bottom) {
  SetEdgeTop(top);
  SetEdgeBottom(bottom);
}

int SelectionBound::GetHeight() const {
  return edge_bottom_rounded_.y() - edge_top_rounded_.y();
}

std::string SelectionBound::ToString() const {
  return base::StringPrintf(
      "SelectionBound(%s, %s, %s, %s, %d)", edge_top_.ToString().c_str(),
      edge_bottom_.ToString().c_str(), edge_top_rounded_.ToString().c_str(),
      edge_bottom_rounded_.ToString().c_str(), visible_);
}

bool operator==(const SelectionBound& lhs, const SelectionBound& rhs) {
  return lhs.type() == rhs.type() && lhs.visible() == rhs.visible() &&
         lhs.edge_top() == rhs.edge_top() &&
         lhs.edge_bottom() == rhs.edge_bottom();
}

bool operator!=(const SelectionBound& lhs, const SelectionBound& rhs) {
  return !(lhs == rhs);
}

gfx::Rect RectBetweenSelectionBounds(const SelectionBound& b1,
                                     const SelectionBound& b2) {
  gfx::Point top_left(b1.edge_top_rounded());
  top_left.SetToMin(b1.edge_bottom_rounded());
  top_left.SetToMin(b2.edge_top_rounded());
  top_left.SetToMin(b2.edge_bottom_rounded());

  gfx::Point bottom_right(b1.edge_top_rounded());
  bottom_right.SetToMax(b1.edge_bottom_rounded());
  bottom_right.SetToMax(b2.edge_top_rounded());
  bottom_right.SetToMax(b2.edge_bottom_rounded());

  gfx::Vector2d diff = bottom_right - top_left;
  return gfx::Rect(top_left, gfx::Size(diff.x(), diff.y()));
}

gfx::RectF RectFBetweenSelectionBounds(const SelectionBound& b1,
                                       const SelectionBound& b2) {
  gfx::PointF top_left(b1.edge_top());
  top_left.SetToMin(b1.edge_bottom());
  top_left.SetToMin(b2.edge_top());
  top_left.SetToMin(b2.edge_bottom());

  gfx::PointF bottom_right(b1.edge_top());
  bottom_right.SetToMax(b1.edge_bottom());
  bottom_right.SetToMax(b2.edge_top());
  bottom_right.SetToMax(b2.edge_bottom());

  gfx::Vector2dF diff = bottom_right - top_left;
  return gfx::RectF(top_left, gfx::SizeF(diff.x(), diff.y()));
}

}  // namespace gfx
