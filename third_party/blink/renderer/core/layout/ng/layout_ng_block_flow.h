// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutNGBlockFlow_h
#define LayoutNGBlockFlow_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_mixin.h"

namespace blink {

// This overrides the default layout block algorithm to use Layout NG.
class CORE_EXPORT LayoutNGBlockFlow : public LayoutNGMixin<LayoutBlockFlow> {
 public:
  explicit LayoutNGBlockFlow(Element*);
  ~LayoutNGBlockFlow() override;

  void UpdateBlockLayout(bool relayout_children) override;

  const char* GetName() const override { return "LayoutNGBlockFlow"; }

 protected:
  bool IsOfType(LayoutObjectType) const override;

 private:
  void UpdateOutOfFlowBlockLayout();
};

DEFINE_LAYOUT_OBJECT_TYPE_CASTS(LayoutNGBlockFlow, IsLayoutNGBlockFlow());

}  // namespace blink

#endif  // LayoutNGBlockFlow_h
