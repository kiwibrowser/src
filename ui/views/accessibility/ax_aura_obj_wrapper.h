// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_AX_AURA_OBJ_WRAPPER_H_
#define UI_VIEWS_ACCESSIBILITY_AX_AURA_OBJ_WRAPPER_H_

#include <stdint.h>

#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/views_export.h"

namespace ui {
struct AXActionData;
struct AXNodeData;
class AXUniqueId;
}  // namespace ui

namespace views {

// An interface abstraction for Aura views that exposes the view-tree formed
// by the implementing view types.
class VIEWS_EXPORT AXAuraObjWrapper {
 public:
  virtual ~AXAuraObjWrapper() {}

  // Traversal and serialization.
  virtual AXAuraObjWrapper* GetParent() = 0;
  virtual void GetChildren(
      std::vector<AXAuraObjWrapper*>* out_children) = 0;
  virtual void Serialize(ui::AXNodeData* out_node_data) = 0;
  virtual const ui::AXUniqueId& GetUniqueId() const = 0;

  // Actions.
  virtual bool HandleAccessibleAction(const ui::AXActionData& action);
};

}  // namespace views

#endif  // UI_VIEWS_ACCESSIBILITY_AX_AURA_OBJ_WRAPPER_H_
