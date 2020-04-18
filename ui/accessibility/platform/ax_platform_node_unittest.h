// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_PLATFORM_NODE_UNITTEST_H_
#define UI_ACCESSIBILITY_AX_PLATFORM_NODE_UNITTEST_H_

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/accessibility/ax_tree_update.h"

namespace ui {

class AXPlatformNodeTest : public testing::Test {
 public:
  AXPlatformNodeTest();
  ~AXPlatformNodeTest() override;

  // Initialize given an AXTreeUpdate.
  void Init(const AXTreeUpdate& initial_state);

  // Convenience functions to initialize directly from a few AXNodeData objects.
  void Init(const AXNodeData& node1);
  void Init(const AXNodeData& node1, const AXNodeData& node2);
  void Init(const AXNodeData& node1,
            const AXNodeData& node2,
            const AXNodeData& node3);
  void Init(const AXNodeData& node1,
            const AXNodeData& node2,
            const AXNodeData& node3,
            const AXNodeData& node4);

 protected:
  AXNode* GetRootNode() { return tree_->root(); }

  AXTreeUpdate BuildTextField();
  AXTreeUpdate BuildTextFieldWithSelectionRange(int32_t start, int32_t stop);
  AXTreeUpdate BuildContentEditable();
  AXTreeUpdate BuildContentEditableWithSelectionRange(int32_t start,
                                                      int32_t end);
  AXTreeUpdate Build3X3Table();

  std::unique_ptr<AXTree> tree_;
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_PLATFORM_NODE_UNITTEST_H_
