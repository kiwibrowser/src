// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/table_row_painter.h"

#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/core/layout/layout_table_row.h"
#include "third_party/blink/renderer/core/paint/adjust_paint_offset_scope.h"
#include "third_party/blink/renderer/core/paint/box_painter.h"
#include "third_party/blink/renderer/core/paint/box_painter_base.h"
#include "third_party/blink/renderer/core/paint/collapsed_border_painter.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/table_cell_painter.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"

namespace blink {

void TableRowPainter::Paint(const PaintInfo& paint_info,
                            const LayoutPoint& paint_offset) {
  DCHECK(layout_table_row_.HasSelfPaintingLayer());

  // TODO(crbug.com/805514): Paint mask for table row.
  if (paint_info.phase == PaintPhase::kMask)
    return;

  // TODO(crbug.com/577282): This painting order is inconsistent with other
  // outlines.
  if (ShouldPaintSelfOutline(paint_info.phase))
    PaintOutline(paint_info, paint_offset);
  if (paint_info.phase == PaintPhase::kSelfOutlineOnly)
    return;

  if (ShouldPaintSelfBlockBackground(paint_info.phase)) {
    PaintBoxDecorationBackground(
        paint_info, paint_offset,
        layout_table_row_.Section()->FullTableEffectiveColumnSpan());
  }

  if (paint_info.phase == PaintPhase::kSelfBlockBackgroundOnly)
    return;

  PaintInfo paint_info_for_cells = paint_info.ForDescendants();
  for (LayoutTableCell* cell = layout_table_row_.FirstCell(); cell;
       cell = cell->NextCell()) {
    if (!cell->HasSelfPaintingLayer())
      cell->Paint(paint_info_for_cells, paint_offset);
  }
}

void TableRowPainter::PaintOutline(const PaintInfo& paint_info,
                                   const LayoutPoint& paint_offset) {
  DCHECK(ShouldPaintSelfOutline(paint_info.phase));
  AdjustPaintOffsetScope adjustment(layout_table_row_, paint_info,
                                    paint_offset);
  ObjectPainter(layout_table_row_)
      .PaintOutline(adjustment.GetPaintInfo(),
                    adjustment.AdjustedPaintOffset());
}

void TableRowPainter::HandleChangedPartialPaint(
    const PaintInfo& paint_info,
    const CellSpan& dirtied_columns) {
  PaintResult paint_result =
      dirtied_columns ==
              layout_table_row_.Section()->FullTableEffectiveColumnSpan()
          ? kFullyPainted
          : kMayBeClippedByPaintDirtyRect;
  layout_table_row_.GetMutableForPainting().UpdatePaintResult(
      paint_result, paint_info.GetCullRect());
}

void TableRowPainter::PaintBoxDecorationBackground(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const CellSpan& dirtied_columns) {
  bool has_background = layout_table_row_.StyleRef().HasBackground();
  bool has_box_shadow = layout_table_row_.StyleRef().BoxShadow();
  if (!has_background && !has_box_shadow)
    return;

  HandleChangedPartialPaint(paint_info, dirtied_columns);

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_table_row_,
          DisplayItem::kBoxDecorationBackground))
    return;

  AdjustPaintOffsetScope adjustment(layout_table_row_, paint_info,
                                    paint_offset);
  const auto& local_paint_info = adjustment.GetPaintInfo();
  auto adjusted_paint_offset = adjustment.AdjustedPaintOffset();
  DrawingRecorder recorder(local_paint_info.context, layout_table_row_,
                           DisplayItem::kBoxDecorationBackground);
  LayoutRect paint_rect(adjusted_paint_offset, layout_table_row_.Size());

  if (has_box_shadow) {
    BoxPainterBase::PaintNormalBoxShadow(local_paint_info, paint_rect,
                                         layout_table_row_.StyleRef());
  }

  if (has_background) {
    const auto* section = layout_table_row_.Section();
    PaintInfo paint_info_for_cells = local_paint_info.ForDescendants();
    for (auto c = dirtied_columns.Start(); c < dirtied_columns.End(); c++) {
      if (const auto* cell =
              section->OriginatingCellAt(layout_table_row_.RowIndex(), c))
        PaintBackgroundBehindCell(*cell, paint_info_for_cells, paint_offset);
    }
  }

  if (has_box_shadow) {
    BoxPainterBase::PaintInsetBoxShadowWithInnerRect(
        local_paint_info, paint_rect, layout_table_row_.StyleRef());
  }
}

void TableRowPainter::PaintBackgroundBehindCell(
    const LayoutTableCell& cell,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  DCHECK(layout_table_row_.StyleRef().HasBackground());
  LayoutPoint cell_point = paint_offset;
  // If the row is self painting, paintOffset is in row's coordinates, so
  // doesn't need to flip in section's blocks direction. A row doesn't have
  // flipped blocks direction.
  if (!layout_table_row_.HasSelfPaintingLayer()) {
    cell_point =
        layout_table_row_.Section()->FlipForWritingModeForChildForPaint(
            &cell, cell_point);
  }
  TableCellPainter(cell).PaintContainerBackgroundBehindCell(
      paint_info, cell_point, layout_table_row_);
}

void TableRowPainter::PaintCollapsedBorders(const PaintInfo& paint_info,
                                            const LayoutPoint& paint_offset,
                                            const CellSpan& dirtied_columns) {
  base::Optional<DrawingRecorder> recorder;

  if (LIKELY(!layout_table_row_.Table()->ShouldPaintAllCollapsedBorders())) {
    HandleChangedPartialPaint(paint_info, dirtied_columns);

    if (DrawingRecorder::UseCachedDrawingIfPossible(
            paint_info.context, layout_table_row_,
            DisplayItem::kTableCollapsedBorders))
      return;

    recorder.emplace(paint_info.context, layout_table_row_,
                     DisplayItem::kTableCollapsedBorders);
  }
  // Otherwise TablePainter should have created the drawing recorder.

  const auto* section = layout_table_row_.Section();
  unsigned row = layout_table_row_.RowIndex();
  for (unsigned c = std::min(dirtied_columns.End(), section->NumCols(row));
       c > dirtied_columns.Start(); c--) {
    if (const auto* cell = section->OriginatingCellAt(row, c - 1)) {
      LayoutPoint cell_point =
          section->FlipForWritingModeForChildForPaint(cell, paint_offset);
      CollapsedBorderPainter(*cell).PaintCollapsedBorders(paint_info,
                                                          cell_point);
    }
  }
}

}  // namespace blink
