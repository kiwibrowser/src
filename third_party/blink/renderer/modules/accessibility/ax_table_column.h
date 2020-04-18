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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_TABLE_COLUMN_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_TABLE_COLUMN_H_

#include "base/macros.h"
#include "third_party/blink/renderer/modules/accessibility/ax_mock_object.h"
#include "third_party/blink/renderer/modules/accessibility/ax_table.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class AXObjectCacheImpl;

class MODULES_EXPORT AXTableColumn final : public AXMockObject {
 private:
  explicit AXTableColumn(AXObjectCacheImpl&);

 public:
  static AXTableColumn* Create(AXObjectCacheImpl&);
  ~AXTableColumn() override;

  // retrieves the topmost "column" header (th)
  AXObject* HeaderObject();
  // retrieves the "column" headers (th, scope) from top to bottom
  void HeaderObjectsForColumn(AXObjectVector&);

  void SetColumnIndex(int column_index) { column_index_ = column_index; }
  int ColumnIndex() const { return column_index_; }

  void AddChildren() override;
  void SetParent(AXObject*) override;

 protected:
  bool CanSetSelectedAttribute() const override { return false; }

  // Set the role via RoleValue() instead of DetermineAccessibilityRole(),
  // because the role depends on the parent, and DetermineAccessibilityRole()
  // is called before SetParent().
  AccessibilityRole RoleValue() const final;

 private:
  unsigned column_index_;

  bool IsTableCol() const override { return true; }
  bool ComputeAccessibilityIsIgnored(IgnoredReasons* = nullptr) const override;

  DISALLOW_COPY_AND_ASSIGN(AXTableColumn);
};

DEFINE_AX_OBJECT_TYPE_CASTS(AXTableColumn, IsTableCol());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_TABLE_COLUMN_H_
