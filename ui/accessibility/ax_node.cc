// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_node.h"

#include <algorithm>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/gfx/transform.h"

namespace ui {

AXNode::AXNode(AXNode* parent, int32_t id, int32_t index_in_parent)
    : index_in_parent_(index_in_parent), parent_(parent) {
  data_.id = id;
}

AXNode::~AXNode() {
}

int AXNode::GetUnignoredChildCount() const {
  int count = 0;
  for (int i = 0; i < child_count(); i++) {
    AXNode* child = children_[i];
    if (child->data().HasState(ax::mojom::State::kIgnored))
      count += child->GetUnignoredChildCount();
    else
      count++;
  }
  return count;
}

AXNode* AXNode::GetUnignoredChildAtIndex(int index) const {
  int count = 0;
  for (int i = 0; i < child_count(); i++) {
    AXNode* child = children_[i];
    if (child->data().HasState(ax::mojom::State::kIgnored)) {
      int nested_child_count = child->GetUnignoredChildCount();
      if (index < count + nested_child_count)
        return child->GetUnignoredChildAtIndex(index - count);
      else
        count += nested_child_count;
    } else {
      if (count == index)
        return child;
      else
        count++;
    }
  }

  return nullptr;
}

AXNode* AXNode::GetUnignoredParent() const {
  AXNode* result = parent();
  while (result && result->data().HasState(ax::mojom::State::kIgnored))
    result = result->parent();
  return result;
}

int AXNode::GetUnignoredIndexInParent() const {
  AXNode* parent = GetUnignoredParent();
  if (parent) {
    for (int i = 0; i < parent->GetUnignoredChildCount(); ++i) {
      if (parent->GetUnignoredChildAtIndex(i) == this)
        return i;
    }
  }

  return 0;
}

bool AXNode::IsTextNode() const {
  return data().role == ax::mojom::Role::kStaticText ||
         data().role == ax::mojom::Role::kLineBreak ||
         data().role == ax::mojom::Role::kInlineTextBox;
}

void AXNode::SetData(const AXNodeData& src) {
  data_ = src;
}

void AXNode::SetLocation(int32_t offset_container_id,
                         const gfx::RectF& location,
                         gfx::Transform* transform) {
  data_.offset_container_id = offset_container_id;
  data_.location = location;
  if (transform)
    data_.transform.reset(new gfx::Transform(*transform));
  else
    data_.transform.reset(nullptr);
}

void AXNode::SetIndexInParent(int index_in_parent) {
  index_in_parent_ = index_in_parent;
}

void AXNode::SwapChildren(std::vector<AXNode*>& children) {
  children.swap(children_);
}

void AXNode::Destroy() {
  delete this;
}

bool AXNode::IsDescendantOf(AXNode* ancestor) {
  if (this == ancestor)
    return true;
  else if (parent())
    return parent()->IsDescendantOf(ancestor);

  return false;
}

std::vector<int> AXNode::GetOrComputeLineStartOffsets() {
  std::vector<int> line_offsets;
  if (data().GetIntListAttribute(ax::mojom::IntListAttribute::kCachedLineStarts,
                                 &line_offsets))
    return line_offsets;

  int start_offset = 0;
  ComputeLineStartOffsets(&line_offsets, &start_offset);
  data_.AddIntListAttribute(ax::mojom::IntListAttribute::kCachedLineStarts,
                            line_offsets);
  return line_offsets;
}

void AXNode::ComputeLineStartOffsets(std::vector<int>* line_offsets,
                                     int* start_offset) const {
  DCHECK(line_offsets);
  DCHECK(start_offset);
  for (const AXNode* child : children()) {
    DCHECK(child);
    if (child->child_count()) {
      child->ComputeLineStartOffsets(line_offsets, start_offset);
      continue;
    }

    // Don't report if the first piece of text starts a new line or not.
    if (*start_offset && !child->data().HasIntAttribute(
                             ax::mojom::IntAttribute::kPreviousOnLineId)) {
      // If there are multiple objects with an empty accessible label at the
      // start of a line, only include a single line start offset.
      if (line_offsets->empty() || line_offsets->back() != *start_offset)
        line_offsets->push_back(*start_offset);
    }

    base::string16 text =
        child->data().GetString16Attribute(ax::mojom::StringAttribute::kName);
    *start_offset += static_cast<int>(text.length());
  }
}

const std::string& AXNode::GetInheritedStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  const AXNode* current_node = this;
  do {
    if (current_node->data().HasStringAttribute(attribute))
      return current_node->data().GetStringAttribute(attribute);
    current_node = current_node->parent();
  } while (current_node);
  return base::EmptyString();
}

base::string16 AXNode::GetInheritedString16Attribute(
    ax::mojom::StringAttribute attribute) const {
  return base::UTF8ToUTF16(GetInheritedStringAttribute(attribute));
}

std::ostream& operator<<(std::ostream& stream, const AXNode& node) {
  return stream << node.data().ToString();
}

}  // namespace ui
