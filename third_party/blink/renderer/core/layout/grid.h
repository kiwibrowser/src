// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_GRID_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_GRID_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/layout/order_iterator.h"
#include "third_party/blink/renderer/core/style/grid_area.h"
#include "third_party/blink/renderer/core/style/grid_positions_resolver.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/linked_hash_set.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// TODO(svillar): Perhaps we should use references here.
typedef Vector<LayoutBox*, 1> GridCell;
typedef Vector<Vector<GridCell>> GridAsMatrix;
typedef LinkedHashSet<size_t> OrderedTrackIndexSet;

class LayoutGrid;
class GridIterator;

// The Grid class represent a generic storage for grid items. It's currently
// implemented as a matrix (vector of vectors) but it can be eventually replaced
// by a more memory efficient representation. This class is used by the
// LayoutGrid object to place the grid items on a grid like structure, so that
// they could be accessed by rows/columns instead of just traversing the DOM or
// Layout trees.
class Grid final {
 public:
  Grid(const LayoutGrid*);

  size_t NumTracks(GridTrackSizingDirection) const;

  void EnsureGridSize(size_t maximum_row_size, size_t maximum_column_size);
  void insert(LayoutBox&, const GridArea&);

  // Note that out of flow children are not grid items.
  bool HasGridItems() const { return !grid_item_area_.IsEmpty(); }

  bool HasAnyOrthogonalGridItem() const {
    return has_any_orthogonal_grid_item_;
  }
  void SetHasAnyOrthogonalGridItem(bool);

  GridArea GridItemArea(const LayoutBox&) const;
  void SetGridItemArea(const LayoutBox&, GridArea);

  GridSpan GridItemSpan(const LayoutBox&, GridTrackSizingDirection) const;

  size_t GridItemPaintOrder(const LayoutBox&) const;
  void SetGridItemPaintOrder(const LayoutBox&, size_t order);

  const GridCell& Cell(size_t row, size_t column) const;

  int SmallestTrackStart(GridTrackSizingDirection) const;
  void SetSmallestTracksStart(int row_start, int column_start);

  size_t AutoRepeatTracks(GridTrackSizingDirection) const;
  void SetAutoRepeatTracks(size_t auto_repeat_rows, size_t auto_repeat_columns);

  typedef LinkedHashSet<size_t> OrderedTrackIndexSet;
  void SetAutoRepeatEmptyColumns(std::unique_ptr<OrderedTrackIndexSet>);
  void SetAutoRepeatEmptyRows(std::unique_ptr<OrderedTrackIndexSet>);

  size_t AutoRepeatEmptyTracksCount(GridTrackSizingDirection) const;
  bool HasAutoRepeatEmptyTracks(GridTrackSizingDirection) const;
  bool IsEmptyAutoRepeatTrack(GridTrackSizingDirection, size_t) const;

  OrderedTrackIndexSet* AutoRepeatEmptyTracks(GridTrackSizingDirection) const;

  OrderIterator& GetOrderIterator() { return order_iterator_; }

  void SetNeedsItemsPlacement(bool);
  bool NeedsItemsPlacement() const { return needs_items_placement_; };

#if DCHECK_IS_ON()
  bool HasAnyGridItemPaintOrder() const;
#endif

 private:
  friend class GridIterator;

  OrderIterator order_iterator_;

  int smallest_column_start_{0};
  int smallest_row_start_{0};

  size_t auto_repeat_columns_{0};
  size_t auto_repeat_rows_{0};

  bool has_any_orthogonal_grid_item_{false};
  bool needs_items_placement_{true};

  GridAsMatrix grid_;

  HashMap<const LayoutBox*, GridArea> grid_item_area_;
  HashMap<const LayoutBox*, size_t> grid_items_indexes_map_;

  std::unique_ptr<OrderedTrackIndexSet> auto_repeat_empty_columns_{nullptr};
  std::unique_ptr<OrderedTrackIndexSet> auto_repeat_empty_rows_{nullptr};
};

// TODO(svillar): ideally the Grid class should be the one returning an iterator
// for its contents.
class GridIterator final {
 public:
  // |direction| is the direction that is fixed to |fixedTrackIndex| so e.g
  // GridIterator(m_grid, ForColumns, 1) will walk over the rows of the 2nd
  // column.
  GridIterator(const Grid&,
               GridTrackSizingDirection,
               size_t fixed_track_index,
               size_t varying_track_index = 0);

  LayoutBox* NextGridItem();

  bool CheckEmptyCells(size_t row_span, size_t column_span) const;

  std::unique_ptr<GridArea> NextEmptyGridArea(size_t fixed_track_span,
                                              size_t varying_track_span);

 private:
  const GridAsMatrix& grid_;
  GridTrackSizingDirection direction_;
  size_t row_index_;
  size_t column_index_;
  size_t child_index_;
  DISALLOW_COPY_AND_ASSIGN(GridIterator);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_GRID_H_
