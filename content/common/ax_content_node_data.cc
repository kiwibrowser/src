// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/ax_content_node_data.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"

using base::IntToString;

namespace content {

namespace {

// Helper function that finds a key in a vector of pairs by matching on the
// first value, and returns an iterator.
template<typename FirstType, typename SecondType>
typename std::vector<std::pair<FirstType, SecondType>>::const_iterator
    FindInVectorOfPairs(
        FirstType first,
        const std::vector<std::pair<FirstType, SecondType>>& vector) {
  return std::find_if(vector.begin(),
                      vector.end(),
                      [first](std::pair<FirstType, SecondType> const& p) {
                        return p.first == first;
                      });
}

}  // namespace

AXContentNodeData::AXContentNodeData() {
}

AXContentNodeData::AXContentNodeData(const AXContentNodeData& other) = default;

AXContentNodeData::AXContentNodeData(const AXNodeData& other)
    : AXNodeData(other) {
}

AXContentNodeData::~AXContentNodeData() {
}

bool AXContentNodeData::HasContentIntAttribute(
    AXContentIntAttribute attribute) const {
  auto iter = FindInVectorOfPairs(attribute, content_int_attributes);
  return iter != content_int_attributes.end();
}

int AXContentNodeData::GetContentIntAttribute(
    AXContentIntAttribute attribute) const {
  int result;
  if (GetContentIntAttribute(attribute, &result))
    return result;
  return 0;
}

bool AXContentNodeData::GetContentIntAttribute(
    AXContentIntAttribute attribute, int* value) const {
  auto iter = FindInVectorOfPairs(attribute, content_int_attributes);
  if (iter != content_int_attributes.end()) {
    *value = iter->second;
    return true;
  }

  return false;
}

void AXContentNodeData::AddContentIntAttribute(AXContentIntAttribute attribute,
                                               int32_t value) {
  content_int_attributes.push_back(std::make_pair(attribute, value));
}

std::string AXContentNodeData::ToString() const {
  std::string result = AXNodeData::ToString();

  for (auto iter : content_int_attributes) {
    std::string value = IntToString(iter.second);
    switch (iter.first) {
      case AX_CONTENT_ATTR_CHILD_ROUTING_ID:
        result += " child_routing_id=" + value;
        break;
      case AX_CONTENT_ATTR_CHILD_BROWSER_PLUGIN_INSTANCE_ID:
        result += " child_browser_plugin_instance_id=" + value;
        break;
      case AX_CONTENT_INT_ATTRIBUTE_LAST:
        NOTREACHED();
        break;
    }
  }

  return result;
}

AXContentTreeData::AXContentTreeData()
    : AXTreeData(),
      routing_id(-1),
      parent_routing_id(-1) {
}

AXContentTreeData::~AXContentTreeData() {
}

std::string AXContentTreeData::ToString() const {
  std::string result = AXTreeData::ToString();

  if (routing_id != -1)
    result += " routing_id=" + IntToString(routing_id);
  if (parent_routing_id != -1)
    result += " parent_routing_id=" + IntToString(parent_routing_id);

  return result;
}

}  // namespace ui
