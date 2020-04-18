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

#include "third_party/blink/renderer/modules/accessibility/ax_table_column.h"

#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table_cell.h"

namespace blink {

using namespace HTMLNames;

AXTableColumn::AXTableColumn(AXObjectCacheImpl& ax_object_cache)
    : AXMockObject(ax_object_cache) {}

AXTableColumn::~AXTableColumn() = default;

AXTableColumn* AXTableColumn::Create(AXObjectCacheImpl& ax_object_cache) {
  return new AXTableColumn(ax_object_cache);
}

void AXTableColumn::SetParent(AXObject* parent) {
  AXMockObject::SetParent(parent);

  ClearChildren();
}

void AXTableColumn::HeaderObjectsForColumn(AXObjectVector& headers) {
  if (!parent_)
    return;

  LayoutObject* layout_object = parent_->GetLayoutObject();
  if (!layout_object)
    return;

  if (!parent_->IsAXTable())
    return;

  if (ToAXTable(parent_)->IsAriaTable()) {
    for (const auto& cell : Children()) {
      if (cell->RoleValue() == kColumnHeaderRole)
        headers.push_back(cell);
    }
    return;
  }

  if (!layout_object->IsTable())
    return;

  LayoutTable* table = ToLayoutTable(layout_object);
  LayoutTableSection* table_section = table->TopSection();
  for (; table_section;
       table_section = table->SectionBelow(table_section, kSkipEmptySections)) {
    unsigned num_cols = table_section->NumEffectiveColumns();
    if (column_index_ >= num_cols)
      continue;
    unsigned num_rows = table_section->NumRows();
    for (unsigned r = 0; r < num_rows; r++) {
      LayoutTableCell* layout_cell =
          table_section->PrimaryCellAt(r, column_index_);
      if (!layout_cell)
        continue;

      AXObject* cell = AXObjectCache().GetOrCreate(layout_cell->GetNode());
      if (!cell || !cell->IsTableCell() || headers.Contains(cell))
        continue;

      if (ToAXTableCell(cell)->ScanToDecideHeaderRole() == kColumnHeaderRole)
        headers.push_back(cell);
    }
  }
}

AXObject* AXTableColumn::HeaderObject() {
  AXObjectVector headers;
  HeaderObjectsForColumn(headers);
  if (!headers.size())
    return nullptr;

  return headers[0].Get();
}

bool AXTableColumn::ComputeAccessibilityIsIgnored(
    IgnoredReasons* ignored_reasons) const {
  if (!parent_)
    return true;

  if (!parent_->AccessibilityIsIgnored())
    return false;

  if (ignored_reasons)
    parent_->ComputeAccessibilityIsIgnored(ignored_reasons);

  return true;
}

AccessibilityRole AXTableColumn::RoleValue() const {
  return parent_ && parent_->IsAXTable() && ToAXTable(parent_)->IsDataTable()
             ? kColumnRole
             : kLayoutTableColumnRole;
}

void AXTableColumn::AddChildren() {
  DCHECK(!IsDetached());
  DCHECK(!have_children_);

  have_children_ = true;
  if (!parent_ || !parent_->IsAXTable())
    return;

  AXTable* parent_table = ToAXTable(parent_);
  int num_rows = parent_table->RowCount();

  for (int i = 0; i < num_rows; i++) {
    AXTableCell* cell = parent_table->CellForColumnAndRow(column_index_, i);
    if (!cell)
      continue;

    // make sure the last one isn't the same as this one (rowspan cells)
    if (children_.size() > 0 && children_.back() == cell)
      continue;

    children_.push_back(cell);
  }
}

}  // namespace blink
