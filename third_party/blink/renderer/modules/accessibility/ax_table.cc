/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/accessibility/ax_table.h"

#include "third_party/blink/renderer/core/dom/accessible_node.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/core/html/html_table_caption_element.h"
#include "third_party/blink/renderer/core/html/html_table_cell_element.h"
#include "third_party/blink/renderer/core/html/html_table_col_element.h"
#include "third_party/blink/renderer/core/html/html_table_element.h"
#include "third_party/blink/renderer/core/html/html_table_row_element.h"
#include "third_party/blink/renderer/core/html/html_table_rows_collection.h"
#include "third_party/blink/renderer/core/html/html_table_section_element.h"
#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table_cell.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table_column.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table_row.h"

namespace blink {

using namespace HTMLNames;

AXTable::AXTable(LayoutObject* layout_object,
                 AXObjectCacheImpl& ax_object_cache)
    : AXLayoutObject(layout_object, ax_object_cache),
      header_container_(nullptr),
      is_ax_table_(true) {}

AXTable::~AXTable() = default;

void AXTable::Init() {
  AXLayoutObject::Init();
  // Can it be exposed with any kind of table structure / interface?
  is_ax_table_ = IsTableExposableThroughAccessibility();
  // Data tables are a subset of AX Tables. The rest are layout tables.
  is_data_table_ = is_ax_table_ && ComputeIsDataTable();
}

AXTable* AXTable::Create(LayoutObject* layout_object,
                         AXObjectCacheImpl& ax_object_cache) {
  return new AXTable(layout_object, ax_object_cache);
}

bool AXTable::IsAXTable() const {
  return layout_object_ && is_ax_table_;
}

static AccessibilityRole AriaRole(const Element* element) {
  if (!element)
    return kUnknownRole;

  const AtomicString& aria_role = element->FastGetAttribute(roleAttr);
  if (aria_role.IsEmpty())
    return kUnknownRole;
  return AXObject::AriaRoleToWebCoreRole(aria_role);
}

static bool ElementHasSignificantAriaRole(const Element* element) {
  if (!element)
    return false;

  AccessibilityRole role = AriaRole(element);
  return role != kUnknownRole && role != kNoneRole &&
         role != kPresentationalRole;
}

bool AXTable::IsDataTable() const {
  return layout_object_ && is_data_table_;
}

// The following is a heuristic used to determine if a
// <table> should be with kTableRole or kLayoutTableRole.
bool AXTable::ComputeIsDataTable() const {
  if (!layout_object_ || !GetNode())
    return false;

  // When a section of the document is contentEditable, all tables should be
  // treated as data tables, otherwise users may not be able to work with rich
  // text editors that allow creating and editing tables.
  if (GetNode() && HasEditableStyle(*GetNode()))
    return true;

  // This employs a heuristic to determine if this table should appear.
  // Only "data" tables should be exposed as tables.
  // Unfortunately, there is no good way to determine the difference
  // between a "layout" table and a "data" table.

  LayoutTable* table = ToLayoutTable(layout_object_);
  Node* table_node = table->GetNode();
  DCHECK(IsHTMLTableElement(table_node));
  HTMLTableElement* table_element = ToHTMLTableElement(table_node);

  // If there is a caption element, summary, THEAD, or TFOOT section, it's most
  // certainly a data table
  if (!table_element->Summary().IsEmpty() || table_element->tHead() ||
      table_element->tFoot() || table_element->caption())
    return true;

  // if someone used "rules" attribute than the table should appear
  if (!table_element->Rules().IsEmpty())
    return true;

  // if there's a colgroup or col element, it's probably a data table.
  if (Traversal<HTMLTableColElement>::FirstChild(*table_element))
    return true;

  // go through the cell's and check for tell-tale signs of "data" table status
  // cells have borders, or use attributes like headers, abbr, scope or axis
  table->RecalcSectionsIfNeeded();
  LayoutTableSection* first_body = table->FirstBody();
  if (!first_body)
    return false;

  int num_cols_in_first_body = first_body->NumEffectiveColumns();
  int num_rows = first_body->NumRows();

  // If there's only one cell, it's not a good AXTable candidate.
  if (num_rows == 1 && num_cols_in_first_body == 1)
    return false;

  // If there are at least 20 rows, we'll call it a data table.
  if (num_rows >= 20)
    return true;

  // Store the background color of the table to check against cell's background
  // colors.
  const ComputedStyle* table_style = table->Style();
  if (!table_style)
    return false;
  Color table_bg_color =
      table_style->VisitedDependentColor(GetCSSPropertyBackgroundColor());

  // check enough of the cells to find if the table matches our criteria
  // Criteria:
  //   1) must have at least one valid cell (and)
  //   2) at least half of cells have borders (or)
  //   3) at least half of cells have different bg colors than the table, and
  //      there is cell spacing
  unsigned valid_cell_count = 0;
  unsigned bordered_cell_count = 0;
  unsigned background_difference_cell_count = 0;
  unsigned cells_with_top_border = 0;
  unsigned cells_with_bottom_border = 0;
  unsigned cells_with_left_border = 0;
  unsigned cells_with_right_border = 0;

  Color alternating_row_colors[5];
  int alternating_row_color_count = 0;

  for (int row = 0; row < num_rows; ++row) {
    int n_cols = first_body->NumCols(row);
    for (int col = 0; col < n_cols; ++col) {
      LayoutTableCell* cell = first_body->PrimaryCellAt(row, col);
      if (!cell)
        continue;
      Node* cell_node = cell->GetNode();
      if (!cell_node)
        continue;

      if (cell->Size().Width() < 1 || cell->Size().Height() < 1)
        continue;

      valid_cell_count++;

      // Any <th> tag -> treat as data table.
      if (cell_node->HasTagName(thTag))
        return true;

      // In this case, the developer explicitly assigned a "data" table
      // attribute.
      if (IsHTMLTableCellElement(*cell_node)) {
        HTMLTableCellElement& cell_element = ToHTMLTableCellElement(*cell_node);
        if (!cell_element.Headers().IsEmpty() ||
            !cell_element.Abbr().IsEmpty() || !cell_element.Axis().IsEmpty() ||
            !cell_element.FastGetAttribute(scopeAttr).IsEmpty())
          return true;
      }

      const ComputedStyle* computed_style = cell->Style();
      if (!computed_style)
        continue;

      // If the empty-cells style is set, we'll call it a data table.
      if (computed_style->EmptyCells() == EEmptyCells::kHide)
        return true;

      // If a cell has matching bordered sides, call it a (fully) bordered cell.
      if ((cell->BorderTop() > 0 && cell->BorderBottom() > 0) ||
          (cell->BorderLeft() > 0 && cell->BorderRight() > 0))
        bordered_cell_count++;

      // Also keep track of each individual border, so we can catch tables where
      // most cells have a bottom border, for example.
      if (cell->BorderTop() > 0)
        cells_with_top_border++;
      if (cell->BorderBottom() > 0)
        cells_with_bottom_border++;
      if (cell->BorderLeft() > 0)
        cells_with_left_border++;
      if (cell->BorderRight() > 0)
        cells_with_right_border++;

      // If the cell has a different color from the table and there is cell
      // spacing, then it is probably a data table cell (spacing and colors take
      // the place of borders).
      Color cell_color = computed_style->VisitedDependentColor(
          GetCSSPropertyBackgroundColor());
      if (table->HBorderSpacing() > 0 && table->VBorderSpacing() > 0 &&
          table_bg_color != cell_color && cell_color.Alpha() != 1)
        background_difference_cell_count++;

      // If we've found 10 "good" cells, we don't need to keep searching.
      if (bordered_cell_count >= 10 || background_difference_cell_count >= 10)
        return true;

      // For the first 5 rows, cache the background color so we can check if
      // this table has zebra-striped rows.
      if (row < 5 && row == alternating_row_color_count) {
        LayoutObject* layout_row = cell->Parent();
        if (!layout_row || !layout_row->IsBoxModelObject() ||
            !ToLayoutBoxModelObject(layout_row)->IsTableRow())
          continue;
        const ComputedStyle* row_computed_style = layout_row->Style();
        if (!row_computed_style)
          continue;
        Color row_color = row_computed_style->VisitedDependentColor(
            GetCSSPropertyBackgroundColor());
        alternating_row_colors[alternating_row_color_count] = row_color;
        alternating_row_color_count++;
      }
    }
  }

  // if there is less than two valid cells, it's not a data table
  if (valid_cell_count <= 1)
    return false;

  // half of the cells had borders, it's a data table
  unsigned needed_cell_count = valid_cell_count / 2;
  if (bordered_cell_count >= needed_cell_count ||
      cells_with_top_border >= needed_cell_count ||
      cells_with_bottom_border >= needed_cell_count ||
      cells_with_left_border >= needed_cell_count ||
      cells_with_right_border >= needed_cell_count)
    return true;

  // half had different background colors, it's a data table
  if (background_difference_cell_count >= needed_cell_count)
    return true;

  // Check if there is an alternating row background color indicating a zebra
  // striped style pattern.
  if (alternating_row_color_count > 2) {
    Color first_color = alternating_row_colors[0];
    for (int k = 1; k < alternating_row_color_count; k++) {
      // If an odd row was the same color as the first row, its not alternating.
      if (k % 2 == 1 && alternating_row_colors[k] == first_color)
        return false;
      // If an even row is not the same as the first row, its not alternating.
      if (!(k % 2) && alternating_row_colors[k] != first_color)
        return false;
    }
    return true;
  }

  return false;
}

// If this returns false, the table will be exposed as a generic object
// instead of a kTableRole object with kRowRole children and kCellRole
// grandchildren.
bool AXTable::IsTableExposableThroughAccessibility() const {
  if (!layout_object_)
    return false;

  // If the developer assigned an aria role to this, then we
  // shouldn't expose it as a table, unless, of course, the aria
  // role is a table.
  if (AriaRole(GetElement()) != kUnknownRole)
    return false;

  LayoutTable* table = ToLayoutTable(layout_object_);
  Node* table_node = table->GetNode();
  if (!IsHTMLTableElement(table_node))
    return false;

  // Do not expose as table if any of its child sections or rows has an ARIA
  // role.
  HTMLTableElement* table_element = ToHTMLTableElement(table_node);
  if (ElementHasSignificantAriaRole(table_element->tHead()))
    return false;
  if (ElementHasSignificantAriaRole(table_element->tFoot()))
    return false;

  HTMLCollection* bodies = table_element->tBodies();
  for (unsigned body_index = 0; body_index < bodies->length(); ++body_index) {
    Element* body_element = bodies->item(body_index);
    if (ElementHasSignificantAriaRole(body_element))
      return false;
  }

  HTMLTableRowsCollection* rows = table_element->rows();
  for (unsigned row_index = 0; row_index < rows->length(); ++row_index) {
    Element* row_element = rows->item(row_index);
    if (AriaRole(row_element) != kUnknownRole)
      return false;
  }

  return true;
}

void AXTable::ClearChildren() {
  AXLayoutObject::ClearChildren();
  rows_.clear();
  columns_.clear();

  if (header_container_) {
    header_container_->DetachFromParent();
    header_container_ = nullptr;
  }
}

void AXTable::AddChildren() {
  DCHECK(!IsDetached());
  if (!IsAXTable()) {
    AXLayoutObject::AddChildren();
    return;
  }

  DCHECK(!have_children_);

  have_children_ = true;
  if (!layout_object_ || !layout_object_->IsTable())
    return;

  LayoutTable* table = ToLayoutTable(layout_object_);
  AXObjectCacheImpl& ax_cache = AXObjectCache();

  Node* table_node = table->GetNode();
  if (!IsHTMLTableElement(table_node))
    return;

  // Add caption
  if (HTMLTableCaptionElement* caption =
          ToHTMLTableElement(table_node)->caption()) {
    AXObject* caption_object = ax_cache.GetOrCreate(caption);
    if (caption_object && !caption_object->AccessibilityIsIgnored())
      children_.push_back(caption_object);
  }

  // Go through all the available sections to pull out the rows and add them as
  // children.
  table->RecalcSectionsIfNeeded();
  LayoutTableSection* table_section = table->TopSection();
  if (!table_section)
    return;

  LayoutTableSection* initial_table_section = table_section;
  while (table_section) {
    HeapHashSet<Member<AXObject>> appended_rows;
    unsigned num_rows = table_section->NumRows();
    for (unsigned row_index = 0; row_index < num_rows; ++row_index) {
      LayoutTableRow* layout_row = table_section->RowLayoutObjectAt(row_index);
      if (!layout_row)
        continue;

      AXObject* row_object = ax_cache.GetOrCreate(layout_row);
      if (!row_object || !row_object->IsTableRow())
        continue;

      AXTableRow* row = ToAXTableRow(row_object);
      // We need to check every cell for a new row, because cell spans
      // can cause us to miss rows if we just check the first column.
      if (appended_rows.Contains(row))
        continue;

      row->SetRowIndex(static_cast<int>(rows_.size()));
      rows_.push_back(row);
      if (!row->AccessibilityIsIgnored())
        children_.push_back(row);
      appended_rows.insert(row);
    }

    table_section = table->SectionBelow(table_section, kSkipEmptySections);
  }

  // make the columns based on the number of columns in the first body
  unsigned length = initial_table_section->NumEffectiveColumns();
  for (unsigned i = 0; i < length; ++i) {
    AXTableColumn* column = ToAXTableColumn(ax_cache.GetOrCreate(kColumnRole));
    column->SetColumnIndex((int)i);
    column->SetParent(this);
    columns_.push_back(column);
    if (!column->AccessibilityIsIgnored())
      children_.push_back(column);
  }

  AXObject* header_container_object = HeaderContainer();
  if (header_container_object &&
      !header_container_object->AccessibilityIsIgnored())
    children_.push_back(header_container_object);
}

AXObject* AXTable::HeaderContainer() {
  if (header_container_)
    return header_container_.Get();

  AXMockObject* table_header =
      ToAXMockObject(AXObjectCache().GetOrCreate(kTableHeaderContainerRole));
  table_header->SetParent(this);

  header_container_ = table_header;
  return header_container_.Get();
}

const AXObject::AXObjectVector& AXTable::Columns() {
  UpdateChildrenIfNecessary();

  return columns_;
}

const AXObject::AXObjectVector& AXTable::Rows() {
  UpdateChildrenIfNecessary();

  return rows_;
}

void AXTable::ColumnHeaders(AXObjectVector& headers) {
  if (!layout_object_)
    return;

  UpdateChildrenIfNecessary();
  unsigned column_count = columns_.size();
  for (unsigned c = 0; c < column_count; c++) {
    AXObject* column = columns_[c].Get();
    if (column->IsTableCol())
      ToAXTableColumn(column)->HeaderObjectsForColumn(headers);
  }
}

void AXTable::RowHeaders(AXObjectVector& headers) {
  if (!layout_object_)
    return;

  UpdateChildrenIfNecessary();
  unsigned row_count = rows_.size();
  for (unsigned r = 0; r < row_count; r++) {
    AXObject* row = rows_[r].Get();
    if (row->IsTableRow())
      ToAXTableRow(rows_[r].Get())->HeaderObjectsForRow(headers);
  }
}

int AXTable::AriaColumnCount() {
  int32_t col_count;
  if (!HasAOMPropertyOrARIAAttribute(AOMIntProperty::kColCount, col_count))
    return 0;

  if (col_count > static_cast<int>(ColumnCount()))
    return col_count;

  // Spec says that if all of the columns are present in the DOM, it
  // is not necessary to set this attribute as the user agent can
  // automatically calculate the total number of columns.
  // It returns 0 in order not to set this attribute.
  if (col_count == static_cast<int>(ColumnCount()) || col_count != -1)
    return 0;

  return -1;
}

int AXTable::AriaRowCount() {
  int32_t row_count;
  if (!HasAOMPropertyOrARIAAttribute(AOMIntProperty::kRowCount, row_count))
    return 0;

  if (row_count > static_cast<int>(RowCount()))
    return row_count;

  // Spec says that if all of the rows are present in the DOM, it is
  // not necessary to set this attribute as the user agent can
  // automatically calculate the total number of rows.
  // It returns 0 in order not to set this attribute.
  if (row_count == (int)RowCount() || row_count != -1)
    return 0;

  // In the spec, -1 explicitly means an unknown number of rows.
  return -1;
}

unsigned AXTable::ColumnCount() {
  UpdateChildrenIfNecessary();

  return columns_.size();
}

unsigned AXTable::RowCount() {
  UpdateChildrenIfNecessary();

  return rows_.size();
}

AXTableCell* AXTable::CellForColumnAndRow(unsigned column, unsigned row) {
  UpdateChildrenIfNecessary();
  if (column >= ColumnCount() || row >= RowCount())
    return nullptr;

  // Iterate backwards through the rows in case the desired cell has a rowspan
  // and exists in a previous row.
  for (unsigned row_index_counter = row + 1; row_index_counter > 0;
       --row_index_counter) {
    unsigned row_index = row_index_counter - 1;
    const auto& children = rows_[row_index]->Children();
    // Since some cells may have colspans, we have to check the actual range of
    // each cell to determine which is the right one.
    for (unsigned col_index_counter =
             std::min(static_cast<unsigned>(children.size()), column + 1);
         col_index_counter > 0; --col_index_counter) {
      unsigned col_index = col_index_counter - 1;
      AXObject* child = children[col_index].Get();

      if (!child->IsTableCell())
        continue;

      std::pair<unsigned, unsigned> column_range;
      std::pair<unsigned, unsigned> row_range;
      AXTableCell* table_cell_child = ToAXTableCell(child);
      table_cell_child->ColumnIndexRange(column_range);
      table_cell_child->RowIndexRange(row_range);

      if ((column >= column_range.first &&
           column < (column_range.first + column_range.second)) &&
          (row >= row_range.first &&
           row < (row_range.first + row_range.second)))
        return table_cell_child;
    }
  }

  return nullptr;
}

AccessibilityRole AXTable::RoleValue() const {
  if (!IsAXTable())
    return AXLayoutObject::RoleValue();

  return IsDataTable() ? kTableRole : kLayoutTableRole;
}

bool AXTable::ComputeAccessibilityIsIgnored(
    IgnoredReasons* ignored_reasons) const {
  AXObjectInclusion decision = DefaultObjectInclusion(ignored_reasons);
  if (decision == kIncludeObject)
    return false;
  if (decision == kIgnoreObject)
    return true;

  if (!IsAXTable())
    return AXLayoutObject::ComputeAccessibilityIsIgnored(ignored_reasons);

  return false;
}

void AXTable::Trace(blink::Visitor* visitor) {
  visitor->Trace(rows_);
  visitor->Trace(columns_);
  visitor->Trace(header_container_);
  AXLayoutObject::Trace(visitor);
}

}  // namespace blink
