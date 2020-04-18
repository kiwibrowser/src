// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/grid.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "third_party/blink/renderer/core/layout/layout_grid.h"

namespace blink {

Grid::Grid(const LayoutGrid* grid) : order_iterator_(grid) {}

size_t Grid::NumTracks(GridTrackSizingDirection direction) const {
  if (direction == kForRows)
    return grid_.size();
  return grid_.size() ? grid_[0].size() : 0;
}

void Grid::EnsureGridSize(size_t maximum_row_size, size_t maximum_column_size) {
  const size_t old_row_size = NumTracks(kForRows);
  if (maximum_row_size > old_row_size) {
    grid_.Grow(maximum_row_size);
    for (size_t row = old_row_size; row < NumTracks(kForRows); ++row)
      grid_[row].Grow(NumTracks(kForColumns));
  }

  if (maximum_column_size > NumTracks(kForColumns)) {
    for (size_t row = 0; row < NumTracks(kForRows); ++row)
      grid_[row].Grow(maximum_column_size);
  }
}

void Grid::insert(LayoutBox& child, const GridArea& area) {
  DCHECK(area.rows.IsTranslatedDefinite());
  DCHECK(area.columns.IsTranslatedDefinite());
  EnsureGridSize(area.rows.EndLine(), area.columns.EndLine());

  for (const auto& row : area.rows) {
    for (const auto& column : area.columns)
      grid_[row][column].push_back(&child);
  }

  SetGridItemArea(child, area);
}

void Grid::SetSmallestTracksStart(int row_start, int column_start) {
  smallest_row_start_ = row_start;
  smallest_column_start_ = column_start;
}

int Grid::SmallestTrackStart(GridTrackSizingDirection direction) const {
  return direction == kForRows ? smallest_row_start_ : smallest_column_start_;
}

GridArea Grid::GridItemArea(const LayoutBox& item) const {
  DCHECK(grid_item_area_.Contains(&item));
  return grid_item_area_.at(&item);
}

void Grid::SetGridItemArea(const LayoutBox& item, GridArea area) {
  grid_item_area_.Set(&item, area);
}

size_t Grid::GridItemPaintOrder(const LayoutBox& item) const {
  return grid_items_indexes_map_.at(&item);
}

void Grid::SetGridItemPaintOrder(const LayoutBox& item, size_t order) {
  grid_items_indexes_map_.Set(&item, order);
}

const GridCell& Grid::Cell(size_t row, size_t column) const {
  return grid_[row][column];
}

#if DCHECK_IS_ON()
bool Grid::HasAnyGridItemPaintOrder() const {
  return !grid_items_indexes_map_.IsEmpty();
}
#endif

void Grid::SetAutoRepeatTracks(size_t auto_repeat_rows,
                               size_t auto_repeat_columns) {
  DCHECK_GE(static_cast<unsigned>(kGridMaxTracks),
            NumTracks(kForRows) + auto_repeat_rows);
  DCHECK_GE(static_cast<unsigned>(kGridMaxTracks),
            NumTracks(kForColumns) + auto_repeat_columns);
  auto_repeat_rows_ = auto_repeat_rows;
  auto_repeat_columns_ = auto_repeat_columns;
}

size_t Grid::AutoRepeatTracks(GridTrackSizingDirection direction) const {
  return direction == kForRows ? auto_repeat_rows_ : auto_repeat_columns_;
}

void Grid::SetAutoRepeatEmptyColumns(
    std::unique_ptr<OrderedTrackIndexSet> auto_repeat_empty_columns) {
  auto_repeat_empty_columns_ = std::move(auto_repeat_empty_columns);
}

void Grid::SetAutoRepeatEmptyRows(
    std::unique_ptr<OrderedTrackIndexSet> auto_repeat_empty_rows) {
  auto_repeat_empty_rows_ = std::move(auto_repeat_empty_rows);
}

bool Grid::HasAutoRepeatEmptyTracks(GridTrackSizingDirection direction) const {
  return direction == kForColumns ? !!auto_repeat_empty_columns_
                                  : !!auto_repeat_empty_rows_;
}

bool Grid::IsEmptyAutoRepeatTrack(GridTrackSizingDirection direction,
                                  size_t line) const {
  DCHECK(HasAutoRepeatEmptyTracks(direction));
  return AutoRepeatEmptyTracks(direction)->Contains(line);
}

OrderedTrackIndexSet* Grid::AutoRepeatEmptyTracks(
    GridTrackSizingDirection direction) const {
  DCHECK(HasAutoRepeatEmptyTracks(direction));
  return direction == kForColumns ? auto_repeat_empty_columns_.get()
                                  : auto_repeat_empty_rows_.get();
}

GridSpan Grid::GridItemSpan(const LayoutBox& grid_item,
                            GridTrackSizingDirection direction) const {
  GridArea area = GridItemArea(grid_item);
  return direction == kForColumns ? area.columns : area.rows;
}

void Grid::SetHasAnyOrthogonalGridItem(bool has_any_orthogonal_grid_item) {
  has_any_orthogonal_grid_item_ = has_any_orthogonal_grid_item;
}

void Grid::SetNeedsItemsPlacement(bool needs_items_placement) {
  needs_items_placement_ = needs_items_placement;

  if (!needs_items_placement) {
    grid_.ShrinkToFit();
    return;
  }

  grid_.resize(0);
  grid_item_area_.clear();
  grid_items_indexes_map_.clear();
  has_any_orthogonal_grid_item_ = false;
  smallest_row_start_ = 0;
  smallest_column_start_ = 0;
  auto_repeat_columns_ = 0;
  auto_repeat_rows_ = 0;
  auto_repeat_empty_columns_ = nullptr;
  auto_repeat_empty_rows_ = nullptr;
}

GridIterator::GridIterator(const Grid& grid,
                           GridTrackSizingDirection direction,
                           size_t fixed_track_index,
                           size_t varying_track_index)
    : grid_(grid.grid_),
      direction_(direction),
      row_index_((direction == kForColumns) ? varying_track_index
                                            : fixed_track_index),
      column_index_((direction == kForColumns) ? fixed_track_index
                                               : varying_track_index),
      child_index_(0) {
  DCHECK(!grid_.IsEmpty());
  DCHECK(!grid_[0].IsEmpty());
  DCHECK_LT(row_index_, grid_.size());
  DCHECK_LT(column_index_, grid_[0].size());
}

LayoutBox* GridIterator::NextGridItem() {
  DCHECK(!grid_.IsEmpty());
  DCHECK(!grid_[0].IsEmpty());

  size_t& varying_track_index =
      (direction_ == kForColumns) ? row_index_ : column_index_;
  const size_t end_of_varying_track_index =
      (direction_ == kForColumns) ? grid_.size() : grid_[0].size();
  for (; varying_track_index < end_of_varying_track_index;
       ++varying_track_index) {
    const GridCell& children = grid_[row_index_][column_index_];
    if (child_index_ < children.size())
      return children[child_index_++];

    child_index_ = 0;
  }
  return nullptr;
}

bool GridIterator::CheckEmptyCells(size_t row_span, size_t column_span) const {
  DCHECK(!grid_.IsEmpty());
  DCHECK(!grid_[0].IsEmpty());

  // Ignore cells outside current grid as we will grow it later if needed.
  size_t max_rows = std::min(row_index_ + row_span, grid_.size());
  size_t max_columns = std::min(column_index_ + column_span, grid_[0].size());

  // This adds a O(N^2) behavior that shouldn't be a big deal as we expect
  // spanning areas to be small.
  for (size_t row = row_index_; row < max_rows; ++row) {
    for (size_t column = column_index_; column < max_columns; ++column) {
      const GridCell& children = grid_[row][column];
      if (!children.IsEmpty())
        return false;
    }
  }

  return true;
}

std::unique_ptr<GridArea> GridIterator::NextEmptyGridArea(
    size_t fixed_track_span,
    size_t varying_track_span) {
  DCHECK(!grid_.IsEmpty());
  DCHECK(!grid_[0].IsEmpty());
  DCHECK_GE(fixed_track_span, 1u);
  DCHECK_GE(varying_track_span, 1u);

  size_t row_span =
      (direction_ == kForColumns) ? varying_track_span : fixed_track_span;
  size_t column_span =
      (direction_ == kForColumns) ? fixed_track_span : varying_track_span;

  size_t& varying_track_index =
      (direction_ == kForColumns) ? row_index_ : column_index_;
  const size_t end_of_varying_track_index =
      (direction_ == kForColumns) ? grid_.size() : grid_[0].size();
  for (; varying_track_index < end_of_varying_track_index;
       ++varying_track_index) {
    if (CheckEmptyCells(row_span, column_span)) {
      std::unique_ptr<GridArea> result = std::make_unique<GridArea>(
          GridSpan::TranslatedDefiniteGridSpan(row_index_,
                                               row_index_ + row_span),
          GridSpan::TranslatedDefiniteGridSpan(column_index_,
                                               column_index_ + column_span));
      // Advance the iterator to avoid an infinite loop where we would return
      // the same grid area over and over.
      ++varying_track_index;
      return result;
    }
  }
  return nullptr;
}

}  // namespace blink
