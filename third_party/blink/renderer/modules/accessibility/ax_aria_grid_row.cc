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

#include "third_party/blink/renderer/modules/accessibility/ax_aria_grid_row.h"

#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table.h"

namespace blink {

AXARIAGridRow::AXARIAGridRow(LayoutObject* layout_object,
                             AXObjectCacheImpl& ax_object_cache)
    : AXTableRow(layout_object, ax_object_cache) {}

AXARIAGridRow::~AXARIAGridRow() = default;

AXARIAGridRow* AXARIAGridRow::Create(LayoutObject* layout_object,
                                     AXObjectCacheImpl& ax_object_cache) {
  return new AXARIAGridRow(layout_object, ax_object_cache);
}

bool AXARIAGridRow::IsARIARow() const {
  AXObject* parent = ParentTable();
  if (!parent)
    return false;

  AccessibilityRole parent_role = parent->AriaRoleAttribute();
  return parent_role == kTreeGridRole || parent_role == kGridRole;
}

void AXARIAGridRow::HeaderObjectsForRow(AXObjectVector& headers) {
  for (const auto& cell : Children()) {
    if (cell->RoleValue() == kRowHeaderRole)
      headers.push_back(cell);
  }
}

bool AXARIAGridRow::AddCell(AXObject* possible_cell) {
  if (!possible_cell)
    return false;

  AccessibilityRole role = possible_cell->RoleValue();
  if (role != kCellRole && role != kRowHeaderRole && role != kColumnHeaderRole)
    return false;

  cells_.push_back(possible_cell);
  return true;
}

void AXARIAGridRow::ComputeCells(AXObjectVector possible_cells) {
  // Only add children that are actually rows.
  for (const auto& possible_cell : possible_cells) {
    if (!AddCell(possible_cell)) {
      // Normally with good authoring practices, the cells should be children of
      // the row. However, if this is not the case, recursively look for cells
      // in the descendants of the non-row child.
      if (!possible_cell->HasChildren())
        possible_cell->AddChildren();

      ComputeCells(possible_cell->Children());
    }
  }
}

void AXARIAGridRow::AddChildren() {
  DCHECK(!IsDetached());
  DCHECK(!have_children_);

  AXTableRow::AddChildren();

  if (IsTableRow() && layout_object_) {
    ComputeCells(children_);
  }
}

AXObject* AXARIAGridRow::ParentTable() const {
  // A poorly-authored ARIA grid row could be nested within wrapper elements.
  AXObject* ancestor = static_cast<AXObject*>(const_cast<AXARIAGridRow*>(this));
  do {
    ancestor = ancestor->ParentObjectUnignored();
  } while (ancestor && !ancestor->IsAXTable());

  return ancestor;
}

void AXARIAGridRow::Trace(blink::Visitor* visitor) {
  visitor->Trace(cells_);
  AXTableRow::Trace(visitor);
}

}  // namespace blink
