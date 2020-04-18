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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_ARIA_GRID_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_ARIA_GRID_H_

#include "base/macros.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table.h"

namespace blink {

class AXObjectCacheImpl;

class AXARIAGrid final : public AXTable {
 private:
  AXARIAGrid(LayoutObject*, AXObjectCacheImpl&);

 public:
  static AXARIAGrid* Create(LayoutObject*, AXObjectCacheImpl&);
  ~AXARIAGrid() override;

  bool IsAriaTable() const override { return true; }

  AccessibilityRole RoleValue() const final {
    return AXLayoutObject::RoleValue();
  }  // Use ARIA role

  void AddChildren() override;

 private:
  // ARIA treegrids and grids support selected rows.
  bool SupportsSelectedRows() override { return true; }
  bool IsTableExposableThroughAccessibility() const override { return true; }
  bool ComputeIsDataTable() const override { return true; }

  void ComputeRows(AXObjectVector from_child_list);
  bool AddRow(AXObject*);
  unsigned CalculateNumColumns();
  void AddColumnChildren(unsigned num_cols);
  void AddHeaderContainerChild();

  DISALLOW_COPY_AND_ASSIGN(AXARIAGrid);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_ARIA_GRID_H_
