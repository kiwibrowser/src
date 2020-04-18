// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_AX_CONTENT_NODE_DATA_H_
#define CONTENT_COMMON_AX_CONTENT_NODE_DATA_H_

#include <stdint.h>

#include "content/common/content_export.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/ax_tree_update.h"

namespace content {

enum AXContentIntAttribute {
  // The routing ID of this node's child tree.
  AX_CONTENT_ATTR_CHILD_ROUTING_ID,

  // The browser plugin instance ID of this node's child tree.
  AX_CONTENT_ATTR_CHILD_BROWSER_PLUGIN_INSTANCE_ID,

  AX_CONTENT_INT_ATTRIBUTE_LAST
};

// A subclass of AXNodeData that contains extra fields for
// content-layer-specific AX attributes.
struct CONTENT_EXPORT AXContentNodeData : public ui::AXNodeData {
  AXContentNodeData();
  AXContentNodeData(const AXNodeData& other);
  AXContentNodeData(const AXContentNodeData& other);
  ~AXContentNodeData() override;

  bool HasContentIntAttribute(AXContentIntAttribute attribute) const;
  int GetContentIntAttribute(AXContentIntAttribute attribute) const;
  bool GetContentIntAttribute(AXContentIntAttribute attribute,
                              int* value) const;
  void AddContentIntAttribute(AXContentIntAttribute attribute, int value);

  // Return a string representation of this data, for debugging.
  std::string ToString() const override;

  // This is a simple serializable struct. All member variables should be
  // public and copyable.
  std::vector<std::pair<AXContentIntAttribute, int32_t>> content_int_attributes;
};

// A subclass of AXTreeData that contains extra fields for
// content-layer-specific AX attributes.
struct CONTENT_EXPORT AXContentTreeData : public ui::AXTreeData {
  AXContentTreeData();
  ~AXContentTreeData() override;

  // Return a string representation of this data, for debugging.
  std::string ToString() const override;

  // The routing ID of this frame.
  int routing_id;

  // The routing ID of the parent frame.
  int parent_routing_id;
};

typedef ui::AXTreeUpdateBase<content::AXContentNodeData,
                             content::AXContentTreeData> AXContentTreeUpdate;

}  // namespace content

#endif  // CONTENT_COMMON_AX_CONTENT_NODE_DATA_H_
