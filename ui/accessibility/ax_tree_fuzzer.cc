// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_tree.h"

class EmptyAXTreeDelegate : public ui::AXTreeDelegate {
 public:
  EmptyAXTreeDelegate() {}

  void OnNodeDataWillChange(ui::AXTree* tree,
                            const ui::AXNodeData& old_node_data,
                            const ui::AXNodeData& new_node_data) override {}
  void OnTreeDataChanged(ui::AXTree* tree,
                         const ui::AXTreeData& old_data,
                         const ui::AXTreeData& new_data) override {}
  void OnNodeWillBeDeleted(ui::AXTree* tree, ui::AXNode* node) override {}
  void OnSubtreeWillBeDeleted(ui::AXTree* tree, ui::AXNode* node) override {}
  void OnNodeWillBeReparented(ui::AXTree* tree, ui::AXNode* node) override {}
  void OnSubtreeWillBeReparented(ui::AXTree* tree, ui::AXNode* node) override {}
  void OnNodeCreated(ui::AXTree* tree, ui::AXNode* node) override {}
  void OnNodeReparented(ui::AXTree* tree, ui::AXNode* node) override {}
  void OnNodeChanged(ui::AXTree* tree, ui::AXNode* node) override {}
  void OnAtomicUpdateFinished(ui::AXTree* tree,
                              bool root_changed,
                              const std::vector<Change>& changes) override {}
};

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size) {
  ui::AXTreeUpdate initial_state;
  size_t i = 0;
  while (i < size) {
    ui::AXNodeData node;
    node.id = data[i++];
    if (i < size) {
      size_t child_count = data[i++];
      for (size_t j = 0; j < child_count && i < size; j++)
        node.child_ids.push_back(data[i++]);
    }
    initial_state.nodes.push_back(node);
  }

  // Run with --v=1 to aid in debugging a specific crash.
  VLOG(1) << "Input accessibility tree:\n" << initial_state.ToString();

  EmptyAXTreeDelegate delegate;
  ui::AXTree tree;
  tree.SetDelegate(&delegate);
  tree.Unserialize(initial_state);

  return 0;
}
