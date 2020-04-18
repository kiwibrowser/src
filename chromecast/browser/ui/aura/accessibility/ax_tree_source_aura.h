// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_UI_AURA_ACCESSIBILITY_AX_TREE_SOURCE_AURA_H_
#define CHROMECAST_BROWSER_UI_AURA_ACCESSIBILITY_AX_TREE_SOURCE_AURA_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/macros.h"
#include "chromecast/browser/ui/aura/accessibility/ax_root_obj_wrapper.h"
#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/ax_tree_source.h"

namespace ui {
struct AXActionData;
}  // namespace ui

namespace views {
class AXAuraObjWrapper;
}  // namespace views

// This class exposes the views hierarchy as an accessibility tree permitting
// use with other accessibility classes.
class AXTreeSourceAura
    : public ui::AXTreeSource<views::AXAuraObjWrapper*,
                              ui::AXNodeData,
                              ui::AXTreeData> {
 public:
  AXTreeSourceAura();
  ~AXTreeSourceAura() override;

  // Invoke actions on an Aura view.
  bool HandleAccessibleAction(const ui::AXActionData& action);

  // AXTreeSource implementation.
  bool GetTreeData(ui::AXTreeData* data) const override;
  views::AXAuraObjWrapper* GetRoot() const override;
  views::AXAuraObjWrapper* GetFromId(int32_t id) const override;
  int32_t GetId(views::AXAuraObjWrapper* node) const override;
  void GetChildren(
      views::AXAuraObjWrapper* node,
      std::vector<views::AXAuraObjWrapper*>* out_children) const override;
  views::AXAuraObjWrapper* GetParent(
      views::AXAuraObjWrapper* node) const override;
  bool IsValid(views::AXAuraObjWrapper* node) const override;
  bool IsEqual(views::AXAuraObjWrapper* node1,
               views::AXAuraObjWrapper* node2) const override;
  views::AXAuraObjWrapper* GetNull() const override;
  void SerializeNode(views::AXAuraObjWrapper* node,
                     ui::AXNodeData* out_data) const override;

  // Useful for debugging.
  std::string ToString(views::AXAuraObjWrapper* root, std::string prefix);

 private:
  std::unique_ptr<AXRootObjWrapper> root_;

  DISALLOW_COPY_AND_ASSIGN(AXTreeSourceAura);
};

#endif  // CHROMECAST_BROWSER_UI_AURA_ACCESSIBILITY_AX_TREE_SOURCE_AURA_H_
