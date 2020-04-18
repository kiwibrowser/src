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

#include "third_party/blink/renderer/modules/accessibility/ax_table_row.h"

#include "third_party/blink/renderer/core/dom/accessible_node.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table_cell.h"

namespace blink {

using namespace HTMLNames;

AXTableRow::AXTableRow(LayoutObject* layout_object,
                       AXObjectCacheImpl& ax_object_cache)
    : AXLayoutObject(layout_object, ax_object_cache) {}

AXTableRow::~AXTableRow() = default;

AXTableRow* AXTableRow::Create(LayoutObject* layout_object,
                               AXObjectCacheImpl& ax_object_cache) {
  return new AXTableRow(layout_object, ax_object_cache);
}

void AXTableRow::AddChildren() {
  AXLayoutObject::AddChildren();

  // A row is allowed to have a column index, indicating the index of the
  // first cell in that row, and each subsequent cell gets the next index.
  int col_index = AriaColumnIndex();
  if (!col_index)
    return;

  unsigned index = 0;
  for (const auto& cell : Children()) {
    if (cell->IsTableCell())
      ToAXTableCell(cell.Get())->SetARIAColIndexFromRow(col_index + index);
    index++;
  }
}

AccessibilityRole AXTableRow::DetermineAccessibilityRole() {
  if (!IsTableRow())
    return AXLayoutObject::DetermineAccessibilityRole();

  if ((aria_role_ = DetermineAriaRoleAttribute()) != kUnknownRole)
    return aria_role_;

  return ParentTable()->IsDataTable() ? kRowRole : kLayoutTableRowRole;
}

bool AXTableRow::IsTableRow() const {
  AXObject* table = ParentTable();
  if (!table || !table->IsAXTable())
    return false;

  return true;
}

bool AXTableRow::ComputeAccessibilityIsIgnored(
    IgnoredReasons* ignored_reasons) const {
  AXObjectInclusion decision = DefaultObjectInclusion(ignored_reasons);
  if (decision == kIncludeObject)
    return false;
  if (decision == kIgnoreObject)
    return true;

  if (!IsTableRow())
    return AXLayoutObject::ComputeAccessibilityIsIgnored(ignored_reasons);

  return false;
}

AXObject* AXTableRow::ParentTable() const {
  AXObject* parent = ParentObjectUnignored();
  if (!parent || !parent->IsAXTable())
    return nullptr;

  return parent;
}

AXObject* AXTableRow::HeaderObject() {
  AXObjectVector headers;
  HeaderObjectsForRow(headers);
  if (!headers.size())
    return nullptr;

  return headers[0].Get();
}

unsigned AXTableRow::AriaColumnIndex() const {
  uint32_t col_index;
  if (HasAOMPropertyOrARIAAttribute(AOMUIntProperty::kColIndex, col_index) &&
      col_index >= 1) {
    return col_index;
  }

  return 0;
}

unsigned AXTableRow::AriaRowIndex() const {
  uint32_t row_index;
  if (HasAOMPropertyOrARIAAttribute(AOMUIntProperty::kRowIndex, row_index) &&
      row_index >= 1) {
    return row_index;
  }

  return 0;
}

void AXTableRow::HeaderObjectsForRow(AXObjectVector& headers) {
  if (!layout_object_ || !layout_object_->IsTableRow())
    return;

  for (const auto& cell : Children()) {
    if (!cell->IsTableCell())
      continue;

    if (ToAXTableCell(cell.Get())->ScanToDecideHeaderRole() == kRowHeaderRole)
      headers.push_back(cell);
  }
}

}  // namespace blink
