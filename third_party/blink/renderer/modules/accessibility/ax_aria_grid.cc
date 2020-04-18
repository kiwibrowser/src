/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/modules/accessibility/ax_aria_grid.h"

#include "third_party/blink/renderer/modules/accessibility/ax_aria_grid_row.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table_column.h"

namespace blink {

AXARIAGrid::AXARIAGrid(LayoutObject* layout_object,
                       AXObjectCacheImpl& ax_object_cache)
    : AXTable(layout_object, ax_object_cache) {}

AXARIAGrid::~AXARIAGrid() = default;

AXARIAGrid* AXARIAGrid::Create(LayoutObject* layout_object,
                               AXObjectCacheImpl& ax_object_cache) {
  return new AXARIAGrid(layout_object, ax_object_cache);
}

bool AXARIAGrid::AddRow(AXObject* possible_row) {
  // This does not yet handle the case where the row is not an AXARIAGridRow or
  // AXTable row because it is in a canvas or is a virtual node, as those
  // do not have a layout object, cannot be an AXARIAGridRow, and cannot
  // currently implement the rest of our table logic.
  if (!possible_row || !possible_row->IsTableRow())
    return false;

  AXTableRow* row = ToAXTableRow(possible_row);
  row->SetRowIndex(static_cast<int>(rows_.size()));
  rows_.push_back(possible_row);
  return true;
}

void AXARIAGrid::ComputeRows(AXObjectVector possible_rows) {
  // Only add children that are actually rows.
  for (const auto& possible_row : possible_rows) {
    if (!AddRow(possible_row)) {
      // Normally with good authoring practices, the rows should be children of
      // the grid. However, if this is not the case, recursively look for rows
      // in the descendants of the non-row child.
      if (!possible_row->HasChildren())
        possible_row->AddChildren();

      ComputeRows(possible_row->Children());
    }
  }
}

unsigned AXARIAGrid::CalculateNumColumns() {
  unsigned num_cols = 0;
  for (const auto& row : rows_) {
    // Store the maximum number of columns.
    DCHECK(row->IsTableRow());
    const unsigned num_cells_in_row = ToAXTableRow(row)->Cells().size();
    if (num_cells_in_row > num_cols)
      num_cols = num_cells_in_row;
  }
  return num_cols;
}

void AXARIAGrid::AddColumnChildren(unsigned num_cols) {
  AXObjectCacheImpl& ax_cache = AXObjectCache();
  for (unsigned i = 0; i < num_cols; ++i) {
    AXTableColumn* column = ToAXTableColumn(ax_cache.GetOrCreate(kColumnRole));
    column->SetColumnIndex((int)i);
    column->SetParent(this);
    columns_.push_back(column);
    if (!column->AccessibilityIsIgnored())  // TODO is this check necessary?
      children_.push_back(column);
  }
}

void AXARIAGrid::AddHeaderContainerChild() {
  AXObject* header_container_object = HeaderContainer();
  if (header_container_object &&
      !header_container_object->AccessibilityIsIgnored())
    children_.push_back(header_container_object);
}

void AXARIAGrid::AddChildren() {
  DCHECK(!IsDetached());
  DCHECK(!have_children_);

  AXLayoutObject::AddChildren();

  if (IsAXTable() && layout_object_) {
    ComputeRows(children_);
    AddColumnChildren(CalculateNumColumns());
    AddHeaderContainerChild();
  }
}

}  // namespace blink
