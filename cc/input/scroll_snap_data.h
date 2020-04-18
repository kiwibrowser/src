// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_SCROLL_SNAP_DATA_H_
#define CC_INPUT_SCROLL_SNAP_DATA_H_

#include <vector>

#include "cc/cc_export.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/scroll_offset.h"

namespace cc {

// See https://www.w3.org/TR/css-scroll-snap-1/#snap-axis
enum class SnapAxis : unsigned {
  kBoth,
  kX,
  kY,
  kBlock,
  kInline,
};

// A helper enum to specify the the axis when doing calculations.
enum class SearchAxis : unsigned { kX, kY };

// See https://www.w3.org/TR/css-scroll-snap-1/#snap-strictness
// TODO(sunyunjia): Add kNone for SnapStrictness to match the spec.
// crbug.com/791663
enum class SnapStrictness : unsigned { kProximity, kMandatory };

// See https://www.w3.org/TR/css-scroll-snap-1/#scroll-snap-align
enum class SnapAlignment : unsigned { kNone, kStart, kEnd, kCenter };

struct ScrollSnapType {
  ScrollSnapType()
      : is_none(true),
        axis(SnapAxis::kBoth),
        strictness(SnapStrictness::kProximity) {}

  ScrollSnapType(bool snap_type_none, SnapAxis axis, SnapStrictness strictness)
      : is_none(snap_type_none), axis(axis), strictness(strictness) {}

  bool operator==(const ScrollSnapType& other) const {
    return is_none == other.is_none && axis == other.axis &&
           strictness == other.strictness;
  }

  bool operator!=(const ScrollSnapType& other) const {
    return !(*this == other);
  }

  // Whether the scroll-snap-type is none or the snap-strictness field has the
  // value None.
  // TODO(sunyunjia): Consider combining is_none with SnapStrictness.
  bool is_none;

  SnapAxis axis;
  SnapStrictness strictness;
};

struct ScrollSnapAlign {
  ScrollSnapAlign()
      : alignment_inline(SnapAlignment::kNone),
        alignment_block(SnapAlignment::kNone) {}

  explicit ScrollSnapAlign(SnapAlignment alignment)
      : alignment_inline(alignment), alignment_block(alignment) {}

  ScrollSnapAlign(SnapAlignment i, SnapAlignment b)
      : alignment_inline(i), alignment_block(b) {}

  bool operator==(const ScrollSnapAlign& other) const {
    return alignment_inline == other.alignment_inline &&
           alignment_block == other.alignment_block;
  }

  bool operator!=(const ScrollSnapAlign& other) const {
    return !(*this == other);
  }

  SnapAlignment alignment_inline;
  SnapAlignment alignment_block;
};

// Snap area is a bounding box that could be snapped to when a scroll happens in
// its scroll container.
// This data structure describes the data needed for SnapCoordinator if we want
// to snap to this snap area.
struct SnapAreaData {
  // kInvalidScrollOffset is used to mark that the snap_position on a specific
  // axis is not applicable, thus should not be considered when snapping on that
  // axis. This is because the snap area has SnapAlignmentNone on that axis.
  static const int kInvalidScrollPosition = -1;

  SnapAreaData() {}

  SnapAreaData(SnapAxis axis,
               gfx::ScrollOffset position,
               gfx::RectF visible,
               bool msnap)
      : snap_axis(axis),
        snap_position(position),
        visible_region(visible),
        must_snap(msnap) {}

  bool operator==(const SnapAreaData& other) const {
    return (other.snap_axis == snap_axis) &&
           (other.snap_position == snap_position) &&
           (other.visible_region == visible_region) &&
           (other.must_snap == must_snap);
  }

  bool operator!=(const SnapAreaData& other) const { return !(*this == other); }

  // The axes along which the area has specified snap positions.
  SnapAxis snap_axis;

  // The scroll_position to snap the area at the specified alignment in that
  // axis.
  // This is in the same coordinate with blink's scroll position, which is the
  // location of the top/left of the scroll viewport in the top/left of the
  // overflow rect.
  gfx::ScrollOffset snap_position;

  // The area is only visible when the current scroll offset is within
  // |visible_region|.
  // See https://drafts.csswg.org/css-scroll-snap-1/#snap-scope
  gfx::RectF visible_region;

  // Whether this area has scroll-snap-stop: always.
  // See https://www.w3.org/TR/css-scroll-snap-1/#scroll-snap-stop
  bool must_snap;

  // TODO(sunyunjia): Add fields for visibility requirement and large area
  // snapping.
};

typedef std::vector<SnapAreaData> SnapAreaList;

// Snap container is a scroll container that has non-'none' value for
// scroll-snap-type. It can be snapped to one of its snap areas when a scroll
// happens.
// This data structure describes the data needed for SnapCoordinator to perform
// snapping in the snap container.
class CC_EXPORT SnapContainerData {
 public:
  SnapContainerData();
  explicit SnapContainerData(ScrollSnapType type);
  SnapContainerData(ScrollSnapType type, gfx::ScrollOffset max);
  SnapContainerData(const SnapContainerData& other);
  SnapContainerData(SnapContainerData&& other);
  ~SnapContainerData();

  SnapContainerData& operator=(const SnapContainerData& other);
  SnapContainerData& operator=(SnapContainerData&& other);

  bool operator==(const SnapContainerData& other) const {
    return (other.scroll_snap_type_ == scroll_snap_type_) &&
           (other.max_position_ == max_position_) &&
           (other.proximity_range_ == proximity_range_) &&
           (other.snap_area_list_ == snap_area_list_);
  }

  bool operator!=(const SnapContainerData& other) const {
    return !(*this == other);
  }

  bool FindSnapPosition(const gfx::ScrollOffset& current_position,
                        bool should_snap_on_x,
                        bool should_snap_on_y,
                        gfx::ScrollOffset* snap_position) const;

  void AddSnapAreaData(SnapAreaData snap_area_data);
  size_t size() const { return snap_area_list_.size(); }
  const SnapAreaData& at(int index) const { return snap_area_list_[index]; }

  void set_scroll_snap_type(ScrollSnapType type) { scroll_snap_type_ = type; }
  ScrollSnapType scroll_snap_type() const { return scroll_snap_type_; }

  void set_max_position(gfx::ScrollOffset position) {
    max_position_ = position;
  }
  gfx::ScrollOffset max_position() const { return max_position_; }

  void set_proximity_range(const gfx::ScrollOffset& range) {
    proximity_range_ = range;
  }
  gfx::ScrollOffset proximity_range() const { return proximity_range_; }

 private:
  // Specifies whether a scroll container is a scroll snap container, how
  // strictly it snaps, and which axes are considered.
  // See https://www.w3.org/TR/css-scroll-snap-1/#scroll-snap-type for details.
  ScrollSnapType scroll_snap_type_;

  // The maximal scroll position of the SnapContainer, in the same coordinate
  // with blink's scroll position.
  gfx::ScrollOffset max_position_;

  // A valid snap position should be within the |proximity_range_| of the
  // current offset on the snapping axis.
  gfx::ScrollOffset proximity_range_;

  // The SnapAreaData for the snap areas in this snap container. When a scroll
  // happens, we iterate through the snap_area_list to find the best snap
  // position.
  std::vector<SnapAreaData> snap_area_list_;
};

CC_EXPORT std::ostream& operator<<(std::ostream&, const SnapAreaData&);
CC_EXPORT std::ostream& operator<<(std::ostream&, const SnapContainerData&);

}  // namespace cc

#endif  // CC_INPUT_SCROLL_SNAP_DATA_H_
