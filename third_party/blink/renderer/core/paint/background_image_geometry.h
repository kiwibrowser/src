// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BACKGROUND_IMAGE_GEOMETRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BACKGROUND_IMAGE_GEOMETRY_H_

#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/geometry/layout_size.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class FillLayer;
class LayoutBox;
class LayoutBoxModelObject;
class LayoutObject;
class LayoutRect;
class LayoutTableCell;
class LayoutView;
class Document;
class ComputedStyle;
class ImageResourceObserver;

class BackgroundImageGeometry {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  // Constructor for LayoutView where the coordinate space is different.
  BackgroundImageGeometry(const LayoutView&);

  // Constructor for table cells where background_object may be the row or
  // column the background image is attached to.
  BackgroundImageGeometry(const LayoutTableCell&,
                          const LayoutObject* background_object);

  // Generic constructor for all other elements.
  BackgroundImageGeometry(const LayoutBoxModelObject&);

  void Calculate(const LayoutBoxModelObject* container,
                 PaintPhase,
                 GlobalPaintFlags,
                 const FillLayer&,
                 const LayoutRect& paint_rect);

  // destRect() is the rect in the space of the containing box into which to
  // draw the image.
  const LayoutRect& DestRect() const { return dest_rect_; }
  // If the image is repeated via tiling, tileSize() is the size
  // in pixels of the area into which to draw the entire image once.
  //
  // tileSize() need not be the same as the intrinsic size of the image; if not,
  // it means the image will be resized (via an image filter) when painted into
  // that tile region. This may happen because of CSS background-size and
  // background-repeat requirements.
  const LayoutSize& TileSize() const { return tile_size_; }
  // phase() represents the point in the image that will appear at (0,0) in the
  // destination space. The point is defined in tileSize() coordinates.
  const LayoutPoint& Phase() const { return phase_; }
  // Space-size represents extra width and height that may be added to
  // the image if used as a pattern with background-repeat: space.
  const LayoutSize& SpaceSize() const { return repeat_spacing_; }
  // Has background-attachment: fixed. Implies that we can't always cheaply
  // compute destRect.
  bool HasNonLocalGeometry() const { return has_non_local_geometry_; }
  // Whether the background needs to be positioned relative to a container
  // element. Only used for tables.
  bool CellUsingContainerBackground() const {
    return cell_using_container_background_;
  }

  const ImageResourceObserver& ImageClient() const;
  const Document& ImageDocument() const;
  const ComputedStyle& ImageStyle() const;

 private:
  void SetDestRect(const LayoutRect& dest_rect) { dest_rect_ = dest_rect; }
  void SetPhase(const LayoutPoint& phase) { phase_ = phase; }
  void SetTileSize(const LayoutSize& tile_size) { tile_size_ = tile_size; }
  void SetSpaceSize(const LayoutSize& repeat_spacing) {
    repeat_spacing_ = repeat_spacing;
  }
  void SetPhaseX(LayoutUnit x) { phase_.SetX(x); }
  void SetPhaseY(LayoutUnit y) { phase_.SetY(y); }

  void SetNoRepeatX(LayoutUnit x_offset);
  void SetNoRepeatY(LayoutUnit y_offset);
  void SetRepeatX(const FillLayer&,
                  LayoutUnit,
                  LayoutUnit,
                  LayoutUnit,
                  LayoutUnit,
                  LayoutUnit);
  void SetRepeatY(const FillLayer&,
                  LayoutUnit,
                  LayoutUnit,
                  LayoutUnit,
                  LayoutUnit,
                  LayoutUnit);
  void SetSpaceX(LayoutUnit, LayoutUnit, LayoutUnit);
  void SetSpaceY(LayoutUnit, LayoutUnit, LayoutUnit);

  void UseFixedAttachment(const LayoutPoint& attachment_point);
  void SetHasNonLocalGeometry() { has_non_local_geometry_ = true; }
  LayoutPoint GetOffsetForCell(const LayoutTableCell&, const LayoutBox&);
  LayoutSize GetBackgroundObjectDimensions(const LayoutTableCell&,
                                           const LayoutBox&);

  LayoutRectOutsets ComputeDestRectAdjustment(const FillLayer&,
                                              PaintPhase,
                                              LayoutRect&,
                                              LayoutRect&) const;
  void ComputePositioningArea(const LayoutBoxModelObject*,
                              PaintPhase,
                              GlobalPaintFlags,
                              const FillLayer&,
                              const LayoutRect&,
                              LayoutRect&,
                              LayoutPoint&);

  const LayoutBoxModelObject& box_;
  const LayoutBoxModelObject& positioning_box_;
  LayoutSize positioning_size_override_;
  LayoutPoint offset_in_background_;

  // TODO(schenney): Convert these to IntPoints for values that we snap
  LayoutRect dest_rect_;
  LayoutPoint phase_;
  LayoutSize tile_size_;
  LayoutSize repeat_spacing_;
  bool has_non_local_geometry_;
  bool coordinate_offset_by_paint_rect_;
  bool painting_table_cell_;
  bool cell_using_container_background_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BACKGROUND_IMAGE_GEOMETRY_H_
