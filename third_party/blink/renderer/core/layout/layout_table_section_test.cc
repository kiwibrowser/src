// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_table_section.h"

#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

namespace {

class LayoutTableSectionTest : public RenderingTest {
 protected:
  LayoutTableSection* GetSectionByElementId(const char* id) {
    return ToLayoutTableSection(GetLayoutObjectByElementId(id));
  }

  LayoutTableSection* CreateSection(unsigned rows, unsigned columns) {
    auto* table = GetDocument().CreateRawElement(HTMLNames::tableTag);
    GetDocument().body()->appendChild(table);
    auto* section = GetDocument().CreateRawElement(HTMLNames::tbodyTag);
    table->appendChild(section);
    for (unsigned i = 0; i < rows; ++i) {
      auto* row = GetDocument().CreateRawElement(HTMLNames::trTag);
      section->appendChild(row);
      for (unsigned i = 0; i < columns; ++i)
        row->appendChild(GetDocument().CreateRawElement(HTMLNames::tdTag));
    }
    GetDocument().View()->UpdateAllLifecyclePhases();
    return ToLayoutTableSection(section->GetLayoutObject());
  }
};

TEST_F(LayoutTableSectionTest,
       BackgroundIsKnownToBeOpaqueWithLayerAndCollapsedBorder) {
  SetBodyInnerHTML(R"HTML(
    <table style='border-collapse: collapse'>
      <thead id='section' style='will-change: transform;
           background-color: blue'>
        <tr><td>Cell</td></tr>
      </thead>
    </table>
  )HTML");

  auto* section = GetSectionByElementId("section");
  EXPECT_TRUE(section);
  EXPECT_FALSE(
      section->BackgroundIsKnownToBeOpaqueInRect(LayoutRect(0, 0, 1, 1)));
}

TEST_F(LayoutTableSectionTest, BackgroundIsKnownToBeOpaqueWithBorderSpacing) {
  SetBodyInnerHTML(R"HTML(
    <table style='border-spacing: 10px'>
      <thead id='section' style='background-color: blue'>
        <tr><td>Cell</td></tr>
      </thead>
    </table>
  )HTML");

  auto* section = GetSectionByElementId("section");
  EXPECT_TRUE(section);
  EXPECT_FALSE(
      section->BackgroundIsKnownToBeOpaqueInRect(LayoutRect(0, 0, 1, 1)));
}

TEST_F(LayoutTableSectionTest, BackgroundIsKnownToBeOpaqueWithEmptyCell) {
  SetBodyInnerHTML(R"HTML(
    <table style='border-spacing: 10px'>
      <thead id='section' style='background-color: blue'>
        <tr><td>Cell</td></tr>
        <tr><td>Cell</td><td>Cell</td></tr>
      </thead>
    </table>
  )HTML");

  auto* section = GetSectionByElementId("section");
  EXPECT_TRUE(section);
  EXPECT_FALSE(
      section->BackgroundIsKnownToBeOpaqueInRect(LayoutRect(0, 0, 1, 1)));
}

TEST_F(LayoutTableSectionTest, EmptySectionDirtiedRowsAndEffeciveColumns) {
  SetBodyInnerHTML(R"HTML(
    <table style='border: 100px solid red'>
      <thead id='section'></thead>
    </table>
  )HTML");

  auto* section = GetSectionByElementId("section");
  EXPECT_TRUE(section);
  CellSpan rows;
  CellSpan columns;
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(50, 50, 100, 100), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(0u, rows.End());
  // The table has at least 1 column even if there is no cell.
  EXPECT_EQ(1u, section->Table()->NumEffectiveColumns());
  EXPECT_EQ(1u, columns.Start());
  EXPECT_EQ(1u, columns.End());
}

TEST_F(LayoutTableSectionTest, PrimaryCellAtAndOriginatingCellAt) {
  SetBodyInnerHTML(R"HTML(
    <table>
      <tbody id='section'>
        <tr>
          <td id='cell00'></td>
          <td id='cell01' rowspan='2'></td>
        </tr>
        <tr>
          <td id='cell10' colspan='2'></td>
        </tr>
      </tbody>
    </table>
  )HTML");

  // x,yO: A cell originates from this grid slot.
  // x,yS: A cell originating from x,y spans into this slot.
  //       0         1
  // 0   0,0(O)    0,1(O)
  // 1   1,0(O)    1,0/0,1(S)
  auto* section = GetSectionByElementId("section");
  auto* cell00 = GetLayoutObjectByElementId("cell00");
  auto* cell01 = GetLayoutObjectByElementId("cell01");
  auto* cell10 = GetLayoutObjectByElementId("cell10");

  EXPECT_EQ(cell00, section->PrimaryCellAt(0, 0));
  EXPECT_EQ(cell01, section->PrimaryCellAt(0, 1));
  EXPECT_EQ(cell10, section->PrimaryCellAt(1, 0));
  EXPECT_EQ(cell10, section->PrimaryCellAt(1, 1));
  EXPECT_EQ(cell00, section->OriginatingCellAt(0, 0));
  EXPECT_EQ(cell01, section->OriginatingCellAt(0, 1));
  EXPECT_EQ(cell10, section->OriginatingCellAt(1, 0));
  EXPECT_EQ(nullptr, section->OriginatingCellAt(1, 1));
}

TEST_F(LayoutTableSectionTest, DirtiedRowsAndEffectiveColumnsWithSpans) {
  SetBodyInnerHTML(R"HTML(
    <style>
      td { width: 100px; height: 100px; padding: 0 }
      table { border-spacing: 0 }
    </style>
    <table>
      <tbody id='section'>
        <tr>
          <td></td>
          <td rowspan='2'></td>
          <td rowspan='2'></td>
        </tr>
        <tr>
          <td colspan='2'></td>
        </tr>
        <tr>
          <td colspan='3'></td>
        </tr>
      </tbody>
    </table>
  )HTML");

  // x,yO: A cell originates from this grid slot.
  // x,yS: A cell originating from x,y spans into this slot.
  //      0          1           2
  // 0  0,0(O)    0,1(O)      0,2(O)
  // 1  1,0(O)    1,0/0,1(S)  0,2(S)
  // 2  2,0(O)    2,0(S)      2,0(S)
  auto* section = GetSectionByElementId("section");

  // Cell 0,0 only.
  CellSpan rows;
  CellSpan columns;
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(5, 5, 90, 90), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(1u, rows.End());
  EXPECT_EQ(0u, columns.Start());
  EXPECT_EQ(1u, columns.End());

  // Rect intersects the first row and all originating primary cells.
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(5, 5, 290, 90), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(1u, rows.End());
  EXPECT_EQ(0u, columns.Start());
  EXPECT_EQ(3u, columns.End());

  // Rect intersects (1,2). Dirtied rows also cover the first row to cover the
  // primary cell's originating slot.
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(205, 105, 90, 90), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(2u, rows.End());
  EXPECT_EQ(2u, columns.Start());
  EXPECT_EQ(3u, columns.End());

  // Rect intersects (1,1) which has multiple levels of cells (originating from
  // (1,0) and (0,1)). Dirtied columns also cover the first column. Dirtied rows
  // also cover the first row.
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(105, 105, 90, 90), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(2u, rows.End());
  EXPECT_EQ(0u, columns.Start());
  EXPECT_EQ(2u, columns.End());

  // Rect intersects (1,1) and (1,2). Dirtied rows also cover the first row.
  // Dirtied columns also cover the first column.
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(105, 105, 190, 90), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(2u, rows.End());
  EXPECT_EQ(0u, columns.Start());
  EXPECT_EQ(3u, columns.End());

  // Rect intersects (1,2) and (2,2). Dirtied rows and dirtied columns cover all
  // rows and columns.
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(205, 105, 90, 190), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(3u, rows.End());
  EXPECT_EQ(0u, columns.Start());
  EXPECT_EQ(3u, columns.End());
}

TEST_F(LayoutTableSectionTest,
       DirtiedRowsAndEffectiveColumnsWithCollapsedBorders) {
  SetBodyInnerHTML(R"HTML(
    <style>
      td { width: 100px; height: 100px; padding: 0; border: 2px solid; }
      table { border-collapse: collapse }
    </style>
    <table>
      <tbody id='section'>
        <tr><td></td><td></td><td></td><td></td></tr>
        <tr><td></td><td></td><td></td><td></td></tr>
        <tr><td></td><td></td><td></td><td></td></tr>
        <tr><td></td><td></td><td></td><td></td></tr>
      </tbody>
    </table>
  )HTML");

  // Dirtied rows and columns are expanded by 1 cell in each side to ensure
  // collapsed borders are covered.
  auto* section = GetSectionByElementId("section");
  CellSpan rows;
  CellSpan columns;

  // Rect intersects cells (0,0 1x1), expanded to (0,0 2x2)
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(50, 50, 10, 10), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(2u, rows.End());
  EXPECT_EQ(0u, columns.Start());
  EXPECT_EQ(2u, columns.End());

  // Rect intersects cells (2,1 1x1), expanded to (1,0 3x3)
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(250, 150, 10, 10), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(3u, rows.End());
  EXPECT_EQ(1u, columns.Start());
  EXPECT_EQ(4u, columns.End());

  // Rect intersects cells (3,2 1x2), expanded to (2,1 2x3)
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(350, 220, 110, 110), rows,
                                          columns);
  EXPECT_EQ(1u, rows.Start());
  EXPECT_EQ(4u, rows.End());
  EXPECT_EQ(2u, columns.Start());
  EXPECT_EQ(4u, columns.End());

  // All cells.
  section->DirtiedRowsAndEffectiveColumns(LayoutRect(0, 0, 400, 400), rows,
                                          columns);
  EXPECT_EQ(0u, rows.Start());
  EXPECT_EQ(4u, rows.End());
  EXPECT_EQ(0u, columns.Start());
  EXPECT_EQ(4u, columns.End());
}

TEST_F(LayoutTableSectionTest, VisualOverflowWithCollapsedBorders) {
  SetBodyInnerHTML(R"HTML(
    <style>
      table { border-collapse: collapse }
      td { border: 0px solid blue; padding: 0 }
      div { width: 100px; height: 100px }
    </style>
    <table>
      <tbody id='section'>
        <tr>
          <td style='border-bottom-width: 10px;
              outline: 3px solid blue'><div></div></td>
          <td style='border-width: 3px 15px'><div></div></td>
        </tr>
        <tr style='outline: 8px solid green'><td><div></div></td></tr>
      </tbody>
    </table>
  )HTML");

  auto* section = GetSectionByElementId("section");

  // The section's self visual overflow covers the collapsed borders.
  LayoutRect expected_self_visual_overflow = section->BorderBoxRect();
  expected_self_visual_overflow.ExpandEdges(LayoutUnit(1), LayoutUnit(8),
                                            LayoutUnit(0), LayoutUnit(0));
  EXPECT_EQ(expected_self_visual_overflow, section->SelfVisualOverflowRect());

  // The section's visual overflow covers self visual overflow and visual
  // overflows rows.
  LayoutRect expected_visual_overflow = section->BorderBoxRect();
  expected_visual_overflow.ExpandEdges(LayoutUnit(3), LayoutUnit(8),
                                       LayoutUnit(8), LayoutUnit(8));
  EXPECT_EQ(expected_visual_overflow, section->VisualOverflowRect());
}

static void SetCellsOverflowInRow(LayoutTableRow* row) {
  for (auto* cell = row->FirstCell(); cell; cell = cell->NextCell())
    ToElement(cell->GetNode())->setAttribute(HTMLNames::classAttr, "overflow");
}

TEST_F(LayoutTableSectionTest, OverflowingCells) {
  SetBodyInnerHTML(R"HTML(
    <style>
      td { width: 10px; height: 10px }
      td.overflow { outline: 10px solid blue }
    </style>
  )HTML");

  LayoutRect paint_rect(50, 50, 50, 50);
  auto* small_section = CreateSection(20, 20);
  EXPECT_FALSE(small_section->HasOverflowingCell());
  CellSpan rows;
  CellSpan columns;
  small_section->DirtiedRowsAndEffectiveColumns(paint_rect, rows, columns);
  EXPECT_NE(small_section->FullSectionRowSpan(), rows);
  EXPECT_NE(small_section->FullTableEffectiveColumnSpan(), columns);

  auto* big_section = CreateSection(80, 80);
  EXPECT_FALSE(big_section->HasOverflowingCell());
  big_section->DirtiedRowsAndEffectiveColumns(paint_rect, rows, columns);
  EXPECT_NE(big_section->FullSectionRowSpan(), rows);
  EXPECT_NE(big_section->FullTableEffectiveColumnSpan(), columns);

  SetCellsOverflowInRow(small_section->FirstRow());
  SetCellsOverflowInRow(big_section->FirstRow());
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Small sections with overflowing cells always use the full paint path.
  EXPECT_TRUE(small_section->HasOverflowingCell());
  EXPECT_EQ(0u, small_section->OverflowingCells().size());
  small_section->DirtiedRowsAndEffectiveColumns(paint_rect, rows, columns);
  EXPECT_EQ(small_section->FullSectionRowSpan(), rows);
  EXPECT_EQ(small_section->FullTableEffectiveColumnSpan(), columns);

  // Big sections with small number of overflowing cells use partial paint path.
  EXPECT_TRUE(big_section->HasOverflowingCell());
  EXPECT_EQ(80u, big_section->OverflowingCells().size());
  big_section->DirtiedRowsAndEffectiveColumns(paint_rect, rows, columns);
  EXPECT_NE(big_section->FullSectionRowSpan(), rows);
  EXPECT_NE(big_section->FullTableEffectiveColumnSpan(), columns);

  for (auto* row = small_section->FirstRow(); row; row = row->NextRow())
    SetCellsOverflowInRow(row);
  for (auto* row = big_section->FirstRow(); row; row = row->NextRow())
    SetCellsOverflowInRow(row);
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Small sections with overflowing cells always use the full paint path.
  EXPECT_TRUE(small_section->HasOverflowingCell());
  EXPECT_EQ(0u, small_section->OverflowingCells().size());
  small_section->DirtiedRowsAndEffectiveColumns(paint_rect, rows, columns);
  EXPECT_EQ(small_section->FullSectionRowSpan(), rows);
  EXPECT_EQ(small_section->FullTableEffectiveColumnSpan(), columns);

  // Big sections with too many overflowing cells are forced to use the full
  // paint path.
  EXPECT_TRUE(big_section->HasOverflowingCell());
  EXPECT_EQ(0u, big_section->OverflowingCells().size());
  big_section->DirtiedRowsAndEffectiveColumns(paint_rect, rows, columns);
  EXPECT_EQ(big_section->FullSectionRowSpan(), rows);
  EXPECT_EQ(big_section->FullTableEffectiveColumnSpan(), columns);
}

}  // anonymous namespace

}  // namespace blink
