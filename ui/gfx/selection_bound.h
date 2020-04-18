// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SELECTION_BOUND_H_
#define UI_GFX_SELECTION_BOUND_H_

#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/gfx_export.h"

namespace gfx {

class Rect;
class RectF;

// Bound of a selection end-point.
class GFX_EXPORT SelectionBound {
 public:
  enum Type { LEFT, RIGHT, CENTER, EMPTY, LAST = EMPTY };

  SelectionBound();
  SelectionBound(const SelectionBound& other);
  ~SelectionBound();

  Type type() const { return type_; }
  void set_type(Type value) { type_ = value; }

  const gfx::PointF& edge_top() const { return edge_top_; }
  const gfx::Point& edge_top_rounded() const { return edge_top_rounded_; }
  void SetEdgeTop(const gfx::PointF& value);

  const gfx::PointF& edge_bottom() const { return edge_bottom_; }
  const gfx::Point& edge_bottom_rounded() const { return edge_bottom_rounded_; }
  void SetEdgeBottom(const gfx::PointF& value);

  void SetEdge(const gfx::PointF& top, const gfx::PointF& bottom);

  bool visible() const { return visible_; }
  void set_visible(bool value) { visible_ = value; }

  // Returns the vertical difference between rounded top and bottom.
  int GetHeight() const;

  std::string ToString() const;

 private:
  Type type_;
  gfx::PointF edge_top_;
  gfx::PointF edge_bottom_;
  gfx::Point edge_top_rounded_;
  gfx::Point edge_bottom_rounded_;
  bool visible_;
};

GFX_EXPORT bool operator==(const SelectionBound& lhs,
                           const SelectionBound& rhs);
GFX_EXPORT bool operator!=(const SelectionBound& lhs,
                           const SelectionBound& rhs);

GFX_EXPORT gfx::Rect RectBetweenSelectionBounds(const SelectionBound& b1,
                                                const SelectionBound& b2);

GFX_EXPORT gfx::RectF RectFBetweenSelectionBounds(const SelectionBound& b1,
                                                  const SelectionBound& b2);

}  // namespace ui

#endif  // UI_GFX_SELECTION_BOUND_H_
