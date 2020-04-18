// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/grid_painter.h"

#include <algorithm>
#include "third_party/blink/renderer/core/layout/layout_grid.h"
#include "third_party/blink/renderer/core/paint/block_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"

namespace blink {

static GridSpan DirtiedGridAreas(const Vector<LayoutUnit>& coordinates,
                                 LayoutUnit start,
                                 LayoutUnit end) {
  // This function does a binary search over the coordinates.
  // This doesn't work with grid items overflowing their grid areas, but that is
  // managed with m_gridItemsOverflowingGridArea.

  size_t start_grid_area_index =
      std::upper_bound(coordinates.begin(), coordinates.end() - 1, start) -
      coordinates.begin();
  if (start_grid_area_index > 0)
    --start_grid_area_index;

  size_t end_grid_area_index =
      std::upper_bound(coordinates.begin() + start_grid_area_index,
                       coordinates.end() - 1, end) -
      coordinates.begin();
  if (end_grid_area_index > 0)
    --end_grid_area_index;

  // GridSpan stores lines' indexes (not tracks' indexes).
  return GridSpan::TranslatedDefiniteGridSpan(start_grid_area_index,
                                              end_grid_area_index + 1);
}

// Helper for the sorting of grid items following order-modified document order.
// See http://www.w3.org/TR/css-flexbox/#order-modified-document-order
static inline bool CompareOrderModifiedDocumentOrder(
    const std::pair<LayoutBox*, size_t>& first_item,
    const std::pair<LayoutBox*, size_t>& second_item) {
  return first_item.second < second_item.second;
}

void GridPainter::PaintChildren(const PaintInfo& paint_info,
                                const LayoutPoint& paint_offset) {
  DCHECK(!layout_grid_.NeedsLayout());

  LayoutRect local_visual_rect = LayoutRect(paint_info.GetCullRect().rect_);
  local_visual_rect.MoveBy(-paint_offset);

  Vector<LayoutUnit> column_positions = layout_grid_.ColumnPositions();
  if (!layout_grid_.StyleRef().IsLeftToRightDirection()) {
    // Translate columnPositions in RTL as we need the physical coordinates of
    // the columns in order to call dirtiedGridAreas().
    for (size_t i = 0; i < column_positions.size(); i++)
      column_positions[i] =
          layout_grid_.TranslateRTLCoordinate(column_positions[i]);
    // We change the order of tracks in columnPositions, as in RTL the leftmost
    // track will be the last one.
    std::sort(column_positions.begin(), column_positions.end());
  }

  GridSpan dirtied_columns = DirtiedGridAreas(
      column_positions, local_visual_rect.X(), local_visual_rect.MaxX());
  GridSpan dirtied_rows =
      DirtiedGridAreas(layout_grid_.RowPositions(), local_visual_rect.Y(),
                       local_visual_rect.MaxY());

  if (!layout_grid_.StyleRef().IsLeftToRightDirection()) {
    // As we changed the order of tracks previously, we need to swap the dirtied
    // columns in RTL.
    size_t last_line = column_positions.size() - 1;
    dirtied_columns = GridSpan::TranslatedDefiniteGridSpan(
        last_line - dirtied_columns.EndLine(),
        last_line - dirtied_columns.StartLine());
  }

  Vector<std::pair<LayoutBox*, size_t>> grid_items_to_be_painted;

  for (const auto& row : dirtied_rows) {
    for (const auto& column : dirtied_columns) {
      const Vector<LayoutBox*, 1>& children =
          layout_grid_.GetGridCell(row, column);
      for (auto* child : children)
        grid_items_to_be_painted.push_back(
            std::make_pair(child, layout_grid_.PaintIndexForGridItem(child)));
    }
  }

  for (auto* item : layout_grid_.ItemsOverflowingGridArea()) {
    LayoutRect item_overflow_rect = item->FrameRect();
    item_overflow_rect.SetSize(item->VisualOverflowRect().Size());
    if (item_overflow_rect.Intersects(local_visual_rect))
      grid_items_to_be_painted.push_back(
          std::make_pair(item, layout_grid_.PaintIndexForGridItem(item)));
  }

  std::stable_sort(grid_items_to_be_painted.begin(),
                   grid_items_to_be_painted.end(),
                   CompareOrderModifiedDocumentOrder);

  LayoutBox* previous = nullptr;
  for (const auto& grid_item_and_paint_index : grid_items_to_be_painted) {
    // We might have duplicates because of spanning children are included in all
    // cells they span.  Skip them here to avoid painting items several times.
    LayoutBox* current = grid_item_and_paint_index.first;
    if (current == previous)
      continue;

    BlockPainter(layout_grid_)
        .PaintAllChildPhasesAtomically(*current, paint_info, paint_offset);
    previous = current;
  }
}

}  // namespace blink
