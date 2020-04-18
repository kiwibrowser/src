// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/accessibility/browser_accessibility_state_impl.h"
#include "content/common/accessibility_messages.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/accessibility/ax_table_info.h"
#include "ui/accessibility/ax_text_utils.h"
#include "ui/accessibility/platform/ax_unique_id.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"

namespace content {

#if !defined(PLATFORM_HAS_NATIVE_ACCESSIBILITY_IMPL)
// static
BrowserAccessibility* BrowserAccessibility::Create() {
  return new BrowserAccessibility();
}
#endif

BrowserAccessibility::BrowserAccessibility()
    : manager_(nullptr), node_(nullptr) {}

BrowserAccessibility::~BrowserAccessibility() {
}

void BrowserAccessibility::Init(BrowserAccessibilityManager* manager,
    ui::AXNode* node) {
  manager_ = manager;
  node_ = node;
}

bool BrowserAccessibility::PlatformIsLeaf() const {
  if (InternalChildCount() == 0)
    return true;

  // These types of objects may have children that we use as internal
  // implementation details, but we want to expose them as leaves to platform
  // accessibility APIs because screen readers might be confused if they find
  // any children.
  if (IsPlainTextField() || IsTextOnlyObject())
    return true;

  // Roles whose children are only presentational according to the ARIA and
  // HTML5 Specs should be hidden from screen readers.
  // (Note that whilst ARIA buttons can have only presentational children, HTML5
  // buttons are allowed to have content.)
  switch (GetRole()) {
    case ax::mojom::Role::kDocCover:
    case ax::mojom::Role::kGraphicsSymbol:
    case ax::mojom::Role::kImage:
    case ax::mojom::Role::kMeter:
    case ax::mojom::Role::kScrollBar:
    case ax::mojom::Role::kSlider:
    case ax::mojom::Role::kSplitter:
    case ax::mojom::Role::kProgressIndicator:
      return true;
    default:
      return false;
  }
}

uint32_t BrowserAccessibility::PlatformChildCount() const {
  if (HasIntAttribute(ax::mojom::IntAttribute::kChildTreeId)) {
    BrowserAccessibilityManager* child_manager =
        BrowserAccessibilityManager::FromID(
            GetIntAttribute(ax::mojom::IntAttribute::kChildTreeId));
    if (child_manager && child_manager->GetRoot()->PlatformGetParent() == this)
      return 1;

    return 0;
  }

  return PlatformIsLeaf() ? 0 : InternalChildCount();
}

bool BrowserAccessibility::IsNative() const {
  return false;
}

bool BrowserAccessibility::IsDescendantOf(
    const BrowserAccessibility* ancestor) const {
  if (!ancestor)
    return false;

  if (this == ancestor)
    return true;

  if (PlatformGetParent())
    return PlatformGetParent()->IsDescendantOf(ancestor);

  return false;
}

bool BrowserAccessibility::IsDocument() const {
  return GetRole() == ax::mojom::Role::kRootWebArea ||
         GetRole() == ax::mojom::Role::kWebArea;
}

bool BrowserAccessibility::IsTextOnlyObject() const {
  return GetRole() == ax::mojom::Role::kStaticText ||
         GetRole() == ax::mojom::Role::kLineBreak ||
         GetRole() == ax::mojom::Role::kInlineTextBox;
}

bool BrowserAccessibility::IsLineBreakObject() const {
  return GetRole() == ax::mojom::Role::kLineBreak ||
         (IsTextOnlyObject() && PlatformGetParent() &&
          PlatformGetParent()->GetRole() == ax::mojom::Role::kLineBreak);
}

BrowserAccessibility* BrowserAccessibility::PlatformGetChild(
    uint32_t child_index) const {
  BrowserAccessibility* result = nullptr;

  if (child_index == 0 &&
      HasIntAttribute(ax::mojom::IntAttribute::kChildTreeId)) {
    BrowserAccessibilityManager* child_manager =
        BrowserAccessibilityManager::FromID(
            GetIntAttribute(ax::mojom::IntAttribute::kChildTreeId));
    if (child_manager && child_manager->GetRoot()->PlatformGetParent() == this)
      result = child_manager->GetRoot();
  } else {
    result = InternalGetChild(child_index);
  }

  return result;
}

bool BrowserAccessibility::PlatformIsChildOfLeaf() const {
  BrowserAccessibility* ancestor = InternalGetParent();

  while (ancestor) {
    if (ancestor->PlatformIsLeaf())
      return true;
    ancestor = ancestor->InternalGetParent();
  }

  return false;
}

BrowserAccessibility* BrowserAccessibility::GetClosestPlatformObject() const {
  BrowserAccessibility* platform_object =
      const_cast<BrowserAccessibility*>(this);
  while (platform_object && platform_object->PlatformIsChildOfLeaf())
    platform_object = platform_object->InternalGetParent();

  DCHECK(platform_object);
  return platform_object;
}

BrowserAccessibility* BrowserAccessibility::GetPreviousSibling() const {
  if (PlatformGetParent() && GetIndexInParent() > 0)
    return PlatformGetParent()->InternalGetChild(GetIndexInParent() - 1);

  return nullptr;
}

BrowserAccessibility* BrowserAccessibility::GetNextSibling() const {
  if (PlatformGetParent() && GetIndexInParent() >= 0 &&
      GetIndexInParent() <
          static_cast<int>(PlatformGetParent()->InternalChildCount() - 1)) {
    return PlatformGetParent()->InternalGetChild(GetIndexInParent() + 1);
  }

  return nullptr;
}

bool BrowserAccessibility::IsPreviousSiblingOnSameLine() const {
  const BrowserAccessibility* previous_sibling = GetPreviousSibling();
  if (!previous_sibling)
    return false;

  // Line linkage information might not be provided on non-leaf objects.
  const BrowserAccessibility* leaf_object = PlatformDeepestFirstChild();
  if (!leaf_object)
    leaf_object = this;

  int32_t previous_on_line_id;
  if (leaf_object->GetIntAttribute(ax::mojom::IntAttribute::kPreviousOnLineId,
                                   &previous_on_line_id)) {
    const BrowserAccessibility* previous_on_line =
        manager()->GetFromID(previous_on_line_id);
    // In the case of a static text sibling, the object designated to be the
    // previous object on this line might be one of its children, i.e. the last
    // inline text box.
    return previous_on_line &&
           previous_on_line->IsDescendantOf(previous_sibling);
  }
  return false;
}

bool BrowserAccessibility::IsNextSiblingOnSameLine() const {
  const BrowserAccessibility* next_sibling = GetNextSibling();
  if (!next_sibling)
    return false;

  // Line linkage information might not be provided on non-leaf objects.
  const BrowserAccessibility* leaf_object = PlatformDeepestLastChild();
  if (!leaf_object)
    leaf_object = this;

  int32_t next_on_line_id;
  if (leaf_object->GetIntAttribute(ax::mojom::IntAttribute::kNextOnLineId,
                                   &next_on_line_id)) {
    const BrowserAccessibility* next_on_line =
        manager()->GetFromID(next_on_line_id);
    // In the case of a static text sibling, the object designated to be the
    // next object on this line might be one of its children, i.e. the first
    // inline text box.
    return next_on_line && next_on_line->IsDescendantOf(next_sibling);
  }
  return false;
}

BrowserAccessibility* BrowserAccessibility::PlatformDeepestFirstChild() const {
  if (!PlatformChildCount())
    return nullptr;

  BrowserAccessibility* deepest_child = PlatformGetChild(0);
  while (deepest_child->PlatformChildCount())
    deepest_child = deepest_child->PlatformGetChild(0);

  return deepest_child;
}

BrowserAccessibility* BrowserAccessibility::PlatformDeepestLastChild() const {
  if (!PlatformChildCount())
    return nullptr;

  BrowserAccessibility* deepest_child =
      PlatformGetChild(PlatformChildCount() - 1);
  while (deepest_child->PlatformChildCount()) {
    deepest_child = deepest_child->PlatformGetChild(
        deepest_child->PlatformChildCount() - 1);
  }

  return deepest_child;
}

BrowserAccessibility* BrowserAccessibility::InternalDeepestFirstChild() const {
  if (!InternalChildCount())
    return nullptr;

  BrowserAccessibility* deepest_child = InternalGetChild(0);
  while (deepest_child->InternalChildCount())
    deepest_child = deepest_child->InternalGetChild(0);

  return deepest_child;
}

BrowserAccessibility* BrowserAccessibility::InternalDeepestLastChild() const {
  if (!InternalChildCount())
    return nullptr;

  BrowserAccessibility* deepest_child =
      InternalGetChild(InternalChildCount() - 1);
  while (deepest_child->InternalChildCount()) {
    deepest_child = deepest_child->InternalGetChild(
        deepest_child->InternalChildCount() - 1);
  }

  return deepest_child;
}

uint32_t BrowserAccessibility::InternalChildCount() const {
  if (!node_ || !manager_)
    return 0;
  return static_cast<uint32_t>(node_->child_count());
}

BrowserAccessibility* BrowserAccessibility::InternalGetChild(
    uint32_t child_index) const {
  if (!node_ || !manager_ || child_index >= InternalChildCount())
    return nullptr;

  auto* child_node = node_->ChildAtIndex(child_index);
  DCHECK(child_node);
  return manager_->GetFromAXNode(child_node);
}

BrowserAccessibility* BrowserAccessibility::PlatformGetParent() const {
  if (!instance_active())
    return nullptr;

  ui::AXNode* parent = node_->parent();
  if (parent)
    return manager_->GetFromAXNode(parent);

  return manager_->GetParentNodeFromParentTree();
}

BrowserAccessibility* BrowserAccessibility::InternalGetParent() const {
  if (!node_ || !manager_)
    return nullptr;
  ui::AXNode* parent = node_->parent();
  if (parent)
    return manager_->GetFromAXNode(parent);

  return nullptr;
}

int32_t BrowserAccessibility::GetId() const {
  return node_ ? node_->id() : -1;
}

gfx::RectF BrowserAccessibility::GetLocation() const {
  return GetData().location;
}

ax::mojom::Role BrowserAccessibility::GetRole() const {
  return GetData().role;
}

int32_t BrowserAccessibility::GetState() const {
  return GetData().state;
}

const BrowserAccessibility::HtmlAttributes&
BrowserAccessibility::GetHtmlAttributes() const {
  return GetData().html_attributes;
}

gfx::Rect BrowserAccessibility::GetFrameBoundsRect() const {
  return RelativeToAbsoluteBounds(gfx::RectF(), true);
}

gfx::Rect BrowserAccessibility::GetPageBoundsRect(bool* offscreen,
                                                  bool clip_bounds) const {
  return RelativeToAbsoluteBounds(gfx::RectF(), false, offscreen, clip_bounds);
}

gfx::Rect BrowserAccessibility::GetPageBoundsForRange(int start, int len)
    const {
  DCHECK_GE(start, 0);
  DCHECK_GE(len, 0);

  // Standard text fields such as textarea have an embedded div inside them that
  // holds all the text.
  // TODO(nektar): This is fragile! Replace with code that flattens tree.
  if (IsPlainTextField() && InternalChildCount() == 1)
    return InternalGetChild(0)->GetPageBoundsForRange(start, len);

  if (GetRole() != ax::mojom::Role::kStaticText) {
    gfx::Rect bounds;
    for (size_t i = 0; i < InternalChildCount() && len > 0; ++i) {
      BrowserAccessibility* child = InternalGetChild(i);
      // Child objects are of length one, since they are represented by a single
      // embedded object character. The exception is text-only objects.
      int child_length_in_parent = 1;
      if (child->IsTextOnlyObject())
        child_length_in_parent = static_cast<int>(child->GetText().size());
      if (start < child_length_in_parent) {
        gfx::Rect child_rect;
        if (child->IsTextOnlyObject()) {
          child_rect = child->GetPageBoundsForRange(start, len);
        } else {
          child_rect = child->GetPageBoundsForRange(
              0, static_cast<int>(child->GetText().size()));
        }
        bounds.Union(child_rect);
        len -= (child_length_in_parent - start);
      }
      if (start > child_length_in_parent)
        start -= child_length_in_parent;
      else
        start = 0;
    }
    return bounds;
  }

  int end = start + len;
  int child_start = 0;
  int child_end = 0;
  gfx::Rect bounds;
  for (size_t i = 0; i < InternalChildCount() && child_end < start + len; ++i) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child->GetRole() != ax::mojom::Role::kInlineTextBox) {
      DLOG(WARNING) << "BrowserAccessibility objects with role STATIC_TEXT " <<
          "should have children of role INLINE_TEXT_BOX.";
      continue;
    }

    int child_length = static_cast<int>(child->GetText().size());
    child_start = child_end;
    child_end += child_length;

    if (child_end < start)
      continue;

    int overlap_start = std::max(start, child_start);
    int overlap_end = std::min(end, child_end);

    int local_start = overlap_start - child_start;
    int local_end = overlap_end - child_start;
    // |local_end| and |local_start| may equal |child_length| when the caret is
    // at the end of a text field.
    DCHECK_GE(local_start, 0);
    DCHECK_LE(local_start, child_length);
    DCHECK_GE(local_end, 0);
    DCHECK_LE(local_end, child_length);

    const std::vector<int32_t>& character_offsets = child->GetIntListAttribute(
        ax::mojom::IntListAttribute::kCharacterOffsets);
    int character_offsets_length = static_cast<int>(character_offsets.size());
    if (character_offsets_length < child_length) {
      // Blink might not return pixel offsets for all characters.
      // Clamp the character range to be within the number of provided pixels.
      local_start = std::min(local_start, character_offsets_length);
      local_end = std::min(local_end, character_offsets_length);
    }
    int start_pixel_offset =
        local_start > 0 ? character_offsets[local_start - 1] : 0;
    int end_pixel_offset =
        local_end > 0 ? character_offsets[local_end - 1] : 0;

    gfx::Rect child_rect = child->GetPageBoundsRect();
    auto text_direction = static_cast<ax::mojom::TextDirection>(
        child->GetIntAttribute(ax::mojom::IntAttribute::kTextDirection));
    gfx::Rect child_overlap_rect;
    switch (text_direction) {
      case ax::mojom::TextDirection::kNone:
      case ax::mojom::TextDirection::kLtr: {
        int left = child_rect.x() + start_pixel_offset;
        int right = child_rect.x() + end_pixel_offset;
        child_overlap_rect = gfx::Rect(left, child_rect.y(),
                                       right - left, child_rect.height());
        break;
      }
      case ax::mojom::TextDirection::kRtl: {
        int right = child_rect.right() - start_pixel_offset;
        int left = child_rect.right() - end_pixel_offset;
        child_overlap_rect = gfx::Rect(left, child_rect.y(),
                                       right - left, child_rect.height());
        break;
      }
      case ax::mojom::TextDirection::kTtb: {
        int top = child_rect.y() + start_pixel_offset;
        int bottom = child_rect.y() + end_pixel_offset;
        child_overlap_rect = gfx::Rect(child_rect.x(), top,
                                       child_rect.width(), bottom - top);
        break;
      }
      case ax::mojom::TextDirection::kBtt: {
        int bottom = child_rect.bottom() - start_pixel_offset;
        int top = child_rect.bottom() - end_pixel_offset;
        child_overlap_rect = gfx::Rect(child_rect.x(), top,
                                       child_rect.width(), bottom - top);
        break;
      }
    }

    if (bounds.width() == 0 && bounds.height() == 0)
      bounds = child_overlap_rect;
    else
      bounds.Union(child_overlap_rect);
  }

  return bounds;
}

gfx::Rect BrowserAccessibility::GetScreenBoundsForRange(int start, int len)
    const {
  gfx::Rect bounds = GetPageBoundsForRange(start, len);

  // Adjust the bounds by the top left corner of the containing view's bounds
  // in screen coordinates.
  bounds.Offset(manager_->GetViewBounds().OffsetFromOrigin());

  return bounds;
}

base::string16 BrowserAccessibility::GetValue() const {
  base::string16 value =
      GetString16Attribute(ax::mojom::StringAttribute::kValue);
  // Some screen readers like Jaws and VoiceOver require a value to be set in
  // text fields with rich content, even though the same information is
  // available on the children.
  if (value.empty() && IsRichTextField())
    return GetInnerText();
  return value;
}

BrowserAccessibility* BrowserAccessibility::ApproximateHitTest(
    const gfx::Point& point) {
  // The best result found that's a child of this object.
  BrowserAccessibility* child_result = nullptr;
  // The best result that's an indirect descendant like grandchild, etc.
  BrowserAccessibility* descendant_result = nullptr;

  // Walk the children recursively looking for the BrowserAccessibility that
  // most tightly encloses the specified point. Walk backwards so that in
  // the absence of any other information, we assume the object that occurs
  // later in the tree is on top of one that comes before it.
  for (int i = static_cast<int>(PlatformChildCount()) - 1; i >= 0; --i) {
    BrowserAccessibility* child = PlatformGetChild(i);

    // Skip table columns because cells are only contained in rows,
    // not columns.
    if (child->GetRole() == ax::mojom::Role::kColumn)
      continue;

    if (child->GetClippedScreenBoundsRect().Contains(point)) {
      BrowserAccessibility* result = child->ApproximateHitTest(point);
      if (result == child && !child_result)
        child_result = result;
      if (result != child && !descendant_result)
        descendant_result = result;
    }

    if (child_result && descendant_result)
      break;
  }

  // Explanation of logic: it's possible that this point overlaps more than
  // one child of this object. If so, as a heuristic we prefer if the point
  // overlaps a descendant of one of the two children and not the other.
  // As an example, suppose you have two rows of buttons - the buttons don't
  // overlap, but the rows do. Without this heuristic, we'd greedily only
  // consider one of the containers.
  if (descendant_result)
    return descendant_result;
  if (child_result)
    return child_result;

  return this;
}

void BrowserAccessibility::Destroy() {
  node_ = nullptr;
  manager_ = nullptr;

  NativeReleaseReference();
}

void BrowserAccessibility::NativeReleaseReference() {
  delete this;
}

bool BrowserAccessibility::HasBoolAttribute(
    ax::mojom::BoolAttribute attribute) const {
  return GetData().HasBoolAttribute(attribute);
}

bool BrowserAccessibility::GetBoolAttribute(
    ax::mojom::BoolAttribute attribute) const {
  return GetData().GetBoolAttribute(attribute);
}

bool BrowserAccessibility::GetBoolAttribute(ax::mojom::BoolAttribute attribute,
                                            bool* value) const {
  return GetData().GetBoolAttribute(attribute, value);
}

bool BrowserAccessibility::HasFloatAttribute(
    ax::mojom::FloatAttribute attribute) const {
  return GetData().HasFloatAttribute(attribute);
}

float BrowserAccessibility::GetFloatAttribute(
    ax::mojom::FloatAttribute attribute) const {
  return GetData().GetFloatAttribute(attribute);
}

bool BrowserAccessibility::GetFloatAttribute(
    ax::mojom::FloatAttribute attribute,
    float* value) const {
  return GetData().GetFloatAttribute(attribute, value);
}

bool BrowserAccessibility::HasInheritedStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  if (!instance_active())
    return false;

  if (GetData().HasStringAttribute(attribute))
    return true;
  return PlatformGetParent() &&
         PlatformGetParent()->HasInheritedStringAttribute(attribute);
}

const std::string& BrowserAccessibility::GetInheritedStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  return node_->GetInheritedStringAttribute(attribute);
}

base::string16 BrowserAccessibility::GetInheritedString16Attribute(
    ax::mojom::StringAttribute attribute) const {
  return node_->GetInheritedString16Attribute(attribute);
}

bool BrowserAccessibility::HasIntAttribute(
    ax::mojom::IntAttribute attribute) const {
  return GetData().HasIntAttribute(attribute);
}

int BrowserAccessibility::GetIntAttribute(
    ax::mojom::IntAttribute attribute) const {
  return GetData().GetIntAttribute(attribute);
}

bool BrowserAccessibility::GetIntAttribute(ax::mojom::IntAttribute attribute,
                                           int* value) const {
  return GetData().GetIntAttribute(attribute, value);
}

bool BrowserAccessibility::HasStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  return GetData().HasStringAttribute(attribute);
}

const std::string& BrowserAccessibility::GetStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  return GetData().GetStringAttribute(attribute);
}

bool BrowserAccessibility::GetStringAttribute(
    ax::mojom::StringAttribute attribute,
    std::string* value) const {
  return GetData().GetStringAttribute(attribute, value);
}

base::string16 BrowserAccessibility::GetString16Attribute(
    ax::mojom::StringAttribute attribute) const {
  return GetData().GetString16Attribute(attribute);
}

bool BrowserAccessibility::GetString16Attribute(
    ax::mojom::StringAttribute attribute,
    base::string16* value) const {
  return GetData().GetString16Attribute(attribute, value);
}

bool BrowserAccessibility::HasIntListAttribute(
    ax::mojom::IntListAttribute attribute) const {
  return GetData().HasIntListAttribute(attribute);
}

const std::vector<int32_t>& BrowserAccessibility::GetIntListAttribute(
    ax::mojom::IntListAttribute attribute) const {
  return GetData().GetIntListAttribute(attribute);
}

bool BrowserAccessibility::GetIntListAttribute(
    ax::mojom::IntListAttribute attribute,
    std::vector<int32_t>* value) const {
  return GetData().GetIntListAttribute(attribute, value);
}

bool BrowserAccessibility::GetHtmlAttribute(
    const char* html_attr, std::string* value) const {
  return GetData().GetHtmlAttribute(html_attr, value);
}

bool BrowserAccessibility::GetHtmlAttribute(
    const char* html_attr, base::string16* value) const {
  return GetData().GetHtmlAttribute(html_attr, value);
}

base::string16 BrowserAccessibility::GetText() const {
  return GetInnerText();
}

bool BrowserAccessibility::HasState(ax::mojom::State state_enum) const {
  return GetData().HasState(state_enum);
}

bool BrowserAccessibility::HasAction(ax::mojom::Action action_enum) const {
  return GetData().HasAction(action_enum);
}

bool BrowserAccessibility::HasCaret() const {
  if (IsPlainTextField() &&
      HasIntAttribute(ax::mojom::IntAttribute::kTextSelStart) &&
      HasIntAttribute(ax::mojom::IntAttribute::kTextSelEnd)) {
    return true;
  }

  // The caret is always at the focus of the selection.
  int32_t focus_id = manager()->GetTreeData().sel_focus_object_id;
  BrowserAccessibility* focus_object = manager()->GetFromID(focus_id);
  if (!focus_object)
    return false;

  return focus_object->IsDescendantOf(this);
}

bool BrowserAccessibility::IsWebAreaForPresentationalIframe() const {
  if (GetRole() != ax::mojom::Role::kWebArea &&
      GetRole() != ax::mojom::Role::kRootWebArea) {
    return false;
  }

  BrowserAccessibility* parent = PlatformGetParent();
  if (!parent)
    return false;

  return parent->GetRole() == ax::mojom::Role::kIframePresentational;
}

bool BrowserAccessibility::IsClickable() const {
  return ui::IsRoleClickable(GetRole());
}

bool BrowserAccessibility::IsPlainTextField() const {
  // We need to check both the role and editable state, because some ARIA text
  // fields may in fact not be editable, whilst some editable fields might not
  // have the role.
  return !HasState(ax::mojom::State::kRichlyEditable) &&
         (GetRole() == ax::mojom::Role::kTextField ||
          GetRole() == ax::mojom::Role::kTextFieldWithComboBox ||
          GetRole() == ax::mojom::Role::kSearchBox ||
          GetBoolAttribute(ax::mojom::BoolAttribute::kEditableRoot));
}

bool BrowserAccessibility::IsRichTextField() const {
  return GetBoolAttribute(ax::mojom::BoolAttribute::kEditableRoot) &&
         HasState(ax::mojom::State::kRichlyEditable);
}

bool BrowserAccessibility::HasExplicitlyEmptyName() const {
  return GetData().GetNameFrom() ==
         ax::mojom::NameFrom::kAttributeExplicitlyEmpty;
}

std::string BrowserAccessibility::ComputeAccessibleNameFromDescendants() const {
  std::string name;
  for (size_t i = 0; i < InternalChildCount(); ++i) {
    BrowserAccessibility* child = InternalGetChild(i);
    std::string child_name;
    if (child->GetStringAttribute(ax::mojom::StringAttribute::kName,
                                  &child_name)) {
      if (!name.empty())
        name += " ";
      name += child_name;
    } else if (!child->HasState(ax::mojom::State::kFocusable)) {
      child_name = child->ComputeAccessibleNameFromDescendants();
      if (!child_name.empty()) {
        if (!name.empty())
          name += " ";
        name += child_name;
      }
    }
  }

  return name;
}

std::vector<int> BrowserAccessibility::GetLineStartOffsets() const {
  if (!instance_active())
    return std::vector<int>();
  return node()->GetOrComputeLineStartOffsets();
}

BrowserAccessibilityPosition::AXPositionInstance
BrowserAccessibility::CreatePositionAt(int offset,
                                       ax::mojom::TextAffinity affinity) const {
  DCHECK(manager_);
  return BrowserAccessibilityPosition::CreateTextPosition(
      manager_->ax_tree_id(), GetId(), offset, affinity);
}

base::string16 BrowserAccessibility::GetInnerText() const {
  if (IsTextOnlyObject())
    return GetString16Attribute(ax::mojom::StringAttribute::kName);

  base::string16 text;
  for (size_t i = 0; i < InternalChildCount(); ++i)
    text += InternalGetChild(i)->GetInnerText();
  return text;
}

gfx::Rect BrowserAccessibility::RelativeToAbsoluteBounds(
    gfx::RectF bounds,
    bool frame_only,
    bool* offscreen,
    bool clip_bounds) const {
  const BrowserAccessibility* node = this;
  while (node) {
    bounds = node->manager()->ax_tree()->RelativeToTreeBounds(
        node->node(), bounds, offscreen, clip_bounds);

    // On some platforms we need to unapply root scroll offsets.
    const BrowserAccessibility* root = node->manager()->GetRoot();
    if (!node->manager()->UseRootScrollOffsetsWhenComputingBounds() &&
        !root->PlatformGetParent()) {
      int sx = 0;
      int sy = 0;
      if (root->GetIntAttribute(ax::mojom::IntAttribute::kScrollX, &sx) &&
          root->GetIntAttribute(ax::mojom::IntAttribute::kScrollY, &sy)) {
        bounds.Offset(sx, sy);
      }
    }

    if (frame_only)
      break;

    node = root->PlatformGetParent();
  }

  return gfx::ToEnclosingRect(bounds);
}

bool BrowserAccessibility::IsOffscreen() const {
  bool offscreen = false;
  RelativeToAbsoluteBounds(gfx::RectF(), false, &offscreen);
  return offscreen;
}

std::set<int32_t> BrowserAccessibility::GetReverseRelations(
    ax::mojom::IntAttribute attr,
    int32_t dst_id) {
  DCHECK(manager_);
  return manager_->ax_tree()->GetReverseRelations(attr, dst_id);
}

std::set<int32_t> BrowserAccessibility::GetReverseRelations(
    ax::mojom::IntListAttribute attr,
    int32_t dst_id) {
  DCHECK(manager_);
  return manager_->ax_tree()->GetReverseRelations(attr, dst_id);
}

const ui::AXUniqueId& BrowserAccessibility::GetUniqueId() const {
  // This is not the same as GetData().id which comes from Blink, because
  // those ids are only unique within the Blink process. We need one that is
  // unique for the browser process.
  return unique_id_;
}

gfx::NativeViewAccessible BrowserAccessibility::GetNativeViewAccessible() {
  // TODO(703369) On Windows, where we have started to migrate to an
  // AXPlatformNode implementation, the BrowserAccessibilityWin subclass has
  // overridden this method. On all other platforms, this method should not be
  // called yet. In the future, when all subclasses have moved over to be
  // implemented by AXPlatformNode, we may make this method completely virtual.
  NOTREACHED();
  return nullptr;
}

//
// AXPlatformNodeDelegate.
//
const ui::AXNodeData& BrowserAccessibility::GetData() const {
  CR_DEFINE_STATIC_LOCAL(ui::AXNodeData, empty_data, ());
  if (node_)
    return node_->data();
  else
    return empty_data;
}

const ui::AXTreeData& BrowserAccessibility::GetTreeData() const {
  CR_DEFINE_STATIC_LOCAL(ui::AXTreeData, empty_data, ());
  if (manager())
    return manager()->GetTreeData();
  else
    return empty_data;
}

gfx::NativeWindow BrowserAccessibility::GetTopLevelWidget() {
  NOTREACHED();
  return nullptr;
}

gfx::NativeViewAccessible BrowserAccessibility::GetParent() {
  auto* parent = PlatformGetParent();
  if (parent)
    return parent->GetNativeViewAccessible();

  if (!manager_)
    return nullptr;

  BrowserAccessibilityDelegate* delegate =
      manager_->GetDelegateFromRootManager();
  if (!delegate)
    return nullptr;

  return delegate->AccessibilityGetNativeViewAccessible();
}

int BrowserAccessibility::GetChildCount() {
  return PlatformChildCount();
}

gfx::NativeViewAccessible BrowserAccessibility::ChildAtIndex(int index) {
  auto* child = PlatformGetChild(index);
  if (!child)
    return nullptr;

  return child->GetNativeViewAccessible();
}

gfx::Rect BrowserAccessibility::GetClippedScreenBoundsRect() const {
  gfx::Rect bounds = GetPageBoundsRect(nullptr, true);

  // Adjust the bounds by the top left corner of the containing view's bounds
  // in screen coordinates.
  bounds.Offset(manager_->GetViewBounds().OffsetFromOrigin());

  return bounds;
}

gfx::Rect BrowserAccessibility::GetUnclippedScreenBoundsRect() const {
  gfx::Rect bounds = GetPageBoundsRect(nullptr, false);

  // Adjust the bounds by the top left corner of the containing view's bounds
  // in screen coordinates.
  bounds.Offset(manager_->GetViewBounds().OffsetFromOrigin());

  return bounds;
}

gfx::NativeViewAccessible BrowserAccessibility::HitTestSync(int x, int y) {
  auto* accessible = manager_->CachingAsyncHitTest(gfx::Point(x, y));
  if (!accessible)
    return nullptr;

  return accessible->GetNativeViewAccessible();
}

gfx::NativeViewAccessible BrowserAccessibility::GetFocus() {
  auto* focused = manager()->GetFocus();
  if (!focused)
    return nullptr;

  return focused->GetNativeViewAccessible();
}

ui::AXPlatformNode* BrowserAccessibility::GetFromNodeID(int32_t id) {
  // Not all BrowserAccessibility subclasses can return an AXPlatformNode yet.
  // So, here we just return nullptr.
  return nullptr;
}

int BrowserAccessibility::GetIndexInParent() const {
  return node_ ? node_->index_in_parent() : -1;
}

gfx::AcceleratedWidget
BrowserAccessibility::GetTargetForNativeAccessibilityEvent() {
  BrowserAccessibilityDelegate* root_delegate =
      manager()->GetDelegateFromRootManager();
  if (!root_delegate)
    return gfx::kNullAcceleratedWidget;
  return root_delegate->AccessibilityGetAcceleratedWidget();
}

int BrowserAccessibility::GetTableRowCount() const {
  ui::AXTableInfo* table_info = manager()->ax_tree()->GetTableInfo(node());
  if (!table_info)
    return 0;

  return table_info->row_count;
}

int BrowserAccessibility::GetTableColCount() const {
  ui::AXTableInfo* table_info = manager()->ax_tree()->GetTableInfo(node());
  if (!table_info)
    return 0;

  return table_info->col_count;
}

std::vector<int32_t> BrowserAccessibility::GetColHeaderNodeIds(
    int32_t col_index) const {
  ui::AXTableInfo* table_info = manager()->ax_tree()->GetTableInfo(node());
  if (!table_info)
    return std::vector<int32_t>();

  if (col_index < 0 || col_index >= table_info->col_count)
    return std::vector<int32_t>();

  return table_info->col_headers[col_index];
}

std::vector<int32_t> BrowserAccessibility::GetRowHeaderNodeIds(
    int32_t row_index) const {
  ui::AXTableInfo* table_info = manager()->ax_tree()->GetTableInfo(node());
  if (!table_info)
    return std::vector<int32_t>();

  if (row_index < 0 || row_index >= table_info->row_count)
    return std::vector<int32_t>();

  return table_info->row_headers[row_index];
}

int32_t BrowserAccessibility::GetCellId(int32_t row_index,
                                        int32_t col_index) const {
  ui::AXTableInfo* table_info = manager()->ax_tree()->GetTableInfo(node());
  if (!table_info)
    return -1;

  if (row_index < 0 || row_index >= table_info->row_count || col_index < 0 ||
      col_index >= table_info->col_count)
    return -1;

  return table_info->cell_ids[row_index][col_index];
}

int32_t BrowserAccessibility::CellIdToIndex(int32_t cell_id) const {
  ui::AXTableInfo* table_info = manager()->ax_tree()->GetTableInfo(node());
  if (!table_info)
    return -1;

  const auto& iter = table_info->cell_id_to_index.find(cell_id);
  if (iter != table_info->cell_id_to_index.end())
    return iter->second;

  return -1;
}

int32_t BrowserAccessibility::CellIndexToId(int32_t cell_index) const {
  ui::AXTableInfo* table_info = manager()->ax_tree()->GetTableInfo(node());
  if (!table_info)
    return -1;

  if (cell_index < 0 ||
      cell_index >= static_cast<int32_t>(table_info->unique_cell_ids.size()))
    return -1;

  return table_info->unique_cell_ids[cell_index];
}

bool BrowserAccessibility::AccessibilityPerformAction(
    const ui::AXActionData& data) {
  switch (data.action) {
    case ax::mojom::Action::kDoDefault:
      manager_->DoDefaultAction(*this);
      return true;
    case ax::mojom::Action::kFocus:
      manager_->SetFocus(*this);
      return true;
    case ax::mojom::Action::kScrollToPoint: {
      // target_point is in screen coordinates.  We need to convert this to
      // frame coordinates because that's what BrowserAccessiblity cares about.
      gfx::Point target =
          data.target_point -
          manager_->GetRootManager()->GetViewBounds().OffsetFromOrigin();

      manager_->ScrollToPoint(*this, target);
      return true;
    }
    case ax::mojom::Action::kScrollToMakeVisible:
      manager_->ScrollToMakeVisible(*this, data.target_rect);
      return true;
    case ax::mojom::Action::kSetValue:
      manager_->SetValue(*this, data.value);
      return true;
    default:
      return false;
  }
}

bool BrowserAccessibility::ShouldIgnoreHoveredStateForTesting() {
  BrowserAccessibilityStateImpl* accessibility_state =
      BrowserAccessibilityStateImpl::GetInstance();
  return accessibility_state->disable_hot_tracking_for_testing();
}

}  // namespace content
