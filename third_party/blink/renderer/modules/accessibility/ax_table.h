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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_TABLE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_TABLE_H_

#include "base/macros.h"
#include "third_party/blink/renderer/modules/accessibility/ax_layout_object.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class AXObjectCacheImpl;
class AXTableCell;

class MODULES_EXPORT AXTable : public AXLayoutObject {
 protected:
  AXTable(LayoutObject*, AXObjectCacheImpl&);

 public:
  static AXTable* Create(LayoutObject*, AXObjectCacheImpl&);
  ~AXTable() override;
  void Trace(blink::Visitor*) override;

  void Init() final;

  bool IsAXTable() const final;
  bool IsDataTable() const final;

  AccessibilityRole RoleValue() const override;

  void AddChildren() override;
  void ClearChildren() final;

  // To be overridden by AXARIAGrid.
  virtual bool IsAriaTable() const { return false; }
  virtual bool SupportsSelectedRows() { return false; }

  const AXObjectVector& Columns();
  const AXObjectVector& Rows();

  unsigned ColumnCount();
  unsigned RowCount();
  AXTableCell* CellForColumnAndRow(unsigned column, unsigned row);

  int AriaColumnCount();
  int AriaRowCount();

  void ColumnHeaders(AXObjectVector&);
  void RowHeaders(AXObjectVector&);

  // an object that contains, as children, all the objects that act as headers
  AXObject* HeaderContainer();

 protected:
  AXObjectVector rows_;
  AXObjectVector columns_;

  Member<AXObject> header_container_;

  bool HasARIARole() const;
  bool ComputeAccessibilityIsIgnored(IgnoredReasons* = nullptr) const final;

  virtual bool ComputeIsDataTable() const;
  virtual bool IsTableExposableThroughAccessibility() const;

 private:
  bool is_ax_table_;
  bool is_data_table_;

  DISALLOW_COPY_AND_ASSIGN(AXTable);
};

DEFINE_AX_OBJECT_TYPE_CASTS(AXTable, IsAXTable());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_TABLE_H_
