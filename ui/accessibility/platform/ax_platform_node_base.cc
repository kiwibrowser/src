// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/platform/ax_platform_node_base.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_action_data.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/platform/ax_platform_node_delegate.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace ui {

void AXPlatformNodeBase::Init(AXPlatformNodeDelegate* delegate) {
  delegate_ = delegate;
}

const AXNodeData& AXPlatformNodeBase::GetData() const {
  CR_DEFINE_STATIC_LOCAL(AXNodeData, empty_data, ());
  if (delegate_)
    return delegate_->GetData();
  return empty_data;
}

gfx::NativeViewAccessible AXPlatformNodeBase::GetParent() {
  if (delegate_)
    return delegate_->GetParent();
  return nullptr;
}

int AXPlatformNodeBase::GetChildCount() {
  if (delegate_)
    return delegate_->GetChildCount();
  return 0;
}

gfx::NativeViewAccessible AXPlatformNodeBase::ChildAtIndex(int index) {
  if (delegate_)
    return delegate_->ChildAtIndex(index);
  return nullptr;
}

// AXPlatformNode overrides.

void AXPlatformNodeBase::Destroy() {
  AXPlatformNode::Destroy();
  delegate_ = nullptr;
  Dispose();
}

void AXPlatformNodeBase::Dispose() {
  delete this;
}

gfx::NativeViewAccessible AXPlatformNodeBase::GetNativeViewAccessible() {
  return nullptr;
}

AXPlatformNodeDelegate* AXPlatformNodeBase::GetDelegate() const {
  return delegate_;
}

// Helpers.

AXPlatformNodeBase* AXPlatformNodeBase::GetPreviousSibling() {
  if (!delegate_)
    return nullptr;
  gfx::NativeViewAccessible parent_accessible = GetParent();
  AXPlatformNodeBase* parent = FromNativeViewAccessible(parent_accessible);
  if (!parent)
    return nullptr;

  int previous_index = GetIndexInParent() - 1;
  if (previous_index >= 0 &&
      previous_index < parent->GetChildCount()) {
    return FromNativeViewAccessible(parent->ChildAtIndex(previous_index));
  }
  return nullptr;
}

AXPlatformNodeBase* AXPlatformNodeBase::GetNextSibling() {
  if (!delegate_)
    return nullptr;
  gfx::NativeViewAccessible parent_accessible = GetParent();
  AXPlatformNodeBase* parent = FromNativeViewAccessible(parent_accessible);
  if (!parent)
    return nullptr;

  int next_index = GetIndexInParent() + 1;
  if (next_index >= 0 && next_index < parent->GetChildCount())
    return FromNativeViewAccessible(parent->ChildAtIndex(next_index));
  return nullptr;
}

bool AXPlatformNodeBase::IsDescendant(AXPlatformNodeBase* node) {
  if (!delegate_)
    return false;
  if (!node)
    return false;
  if (node == this)
    return true;
  gfx::NativeViewAccessible native_parent = node->GetParent();
  if (!native_parent)
    return false;
  AXPlatformNodeBase* parent = FromNativeViewAccessible(native_parent);
  return IsDescendant(parent);
}

bool AXPlatformNodeBase::HasBoolAttribute(
    ax::mojom::BoolAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().HasBoolAttribute(attribute);
}

bool AXPlatformNodeBase::GetBoolAttribute(
    ax::mojom::BoolAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().GetBoolAttribute(attribute);
}

bool AXPlatformNodeBase::GetBoolAttribute(ax::mojom::BoolAttribute attribute,
                                          bool* value) const {
  if (!delegate_)
    return false;
  return GetData().GetBoolAttribute(attribute, value);
}

bool AXPlatformNodeBase::HasFloatAttribute(
    ax::mojom::FloatAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().HasFloatAttribute(attribute);
}

float AXPlatformNodeBase::GetFloatAttribute(
    ax::mojom::FloatAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().GetFloatAttribute(attribute);
}

bool AXPlatformNodeBase::GetFloatAttribute(ax::mojom::FloatAttribute attribute,
                                           float* value) const {
  if (!delegate_)
    return false;
  return GetData().GetFloatAttribute(attribute, value);
}

bool AXPlatformNodeBase::HasIntAttribute(
    ax::mojom::IntAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().HasIntAttribute(attribute);
}

int AXPlatformNodeBase::GetIntAttribute(
    ax::mojom::IntAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().GetIntAttribute(attribute);
}

bool AXPlatformNodeBase::GetIntAttribute(ax::mojom::IntAttribute attribute,
                                         int* value) const {
  if (!delegate_)
    return false;
  return GetData().GetIntAttribute(attribute, value);
}

bool AXPlatformNodeBase::HasStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().HasStringAttribute(attribute);
}

const std::string& AXPlatformNodeBase::GetStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  CR_DEFINE_STATIC_LOCAL(std::string, empty_data, ());
  if (!delegate_)
    return empty_data;
  return GetData().GetStringAttribute(attribute);
}

bool AXPlatformNodeBase::GetStringAttribute(
    ax::mojom::StringAttribute attribute,
    std::string* value) const {
  if (!delegate_)
    return false;
  return GetData().GetStringAttribute(attribute, value);
}

base::string16 AXPlatformNodeBase::GetString16Attribute(
    ax::mojom::StringAttribute attribute) const {
  if (!delegate_)
    return base::string16();
  return GetData().GetString16Attribute(attribute);
}

bool AXPlatformNodeBase::GetString16Attribute(
    ax::mojom::StringAttribute attribute,
    base::string16* value) const {
  if (!delegate_)
    return false;
  return GetData().GetString16Attribute(attribute, value);
}

bool AXPlatformNodeBase::HasIntListAttribute(
    ax::mojom::IntListAttribute attribute) const {
  if (!delegate_)
    return false;
  return GetData().HasIntListAttribute(attribute);
}

const std::vector<int32_t>& AXPlatformNodeBase::GetIntListAttribute(
    ax::mojom::IntListAttribute attribute) const {
  CR_DEFINE_STATIC_LOCAL(std::vector<int32_t>, empty_data, ());
  if (!delegate_)
    return empty_data;
  return GetData().GetIntListAttribute(attribute);
}

bool AXPlatformNodeBase::GetIntListAttribute(
    ax::mojom::IntListAttribute attribute,
    std::vector<int32_t>* value) const {
  if (!delegate_)
    return false;
  return GetData().GetIntListAttribute(attribute, value);
}

AXPlatformNodeBase::AXPlatformNodeBase() {
}

AXPlatformNodeBase::~AXPlatformNodeBase() {
}

// static
AXPlatformNodeBase* AXPlatformNodeBase::FromNativeViewAccessible(
    gfx::NativeViewAccessible accessible) {
  return static_cast<AXPlatformNodeBase*>(
      AXPlatformNode::FromNativeViewAccessible(accessible));
}

bool AXPlatformNodeBase::SetTextSelection(int start_offset, int end_offset) {
  AXActionData action_data;
  action_data.action = ax::mojom::Action::kSetSelection;
  action_data.anchor_node_id = action_data.focus_node_id = GetData().id;
  action_data.anchor_offset = start_offset;
  action_data.focus_offset = end_offset;
  if (!delegate_)
    return false;

  return delegate_->AccessibilityPerformAction(action_data);
}

bool AXPlatformNodeBase::IsTextOnlyObject() const {
  return GetData().role == ax::mojom::Role::kStaticText ||
         GetData().role == ax::mojom::Role::kLineBreak ||
         GetData().role == ax::mojom::Role::kInlineTextBox;
}

bool AXPlatformNodeBase::IsAutofillField() {
  return IsAutofillShown() && IsPlainTextField() &&
         delegate_->GetFocus() == GetNativeViewAccessible();
}

bool AXPlatformNodeBase::IsPlainTextField() const {
  // We need to check both the role and editable state, because some ARIA text
  // fields may in fact not be editable, whilst some editable fields might not
  // have the role.
  return !GetData().HasState(ax::mojom::State::kRichlyEditable) &&
         (GetData().role == ax::mojom::Role::kTextField ||
          GetData().role == ax::mojom::Role::kTextFieldWithComboBox ||
          GetData().role == ax::mojom::Role::kSearchBox ||
          GetBoolAttribute(ax::mojom::BoolAttribute::kEditableRoot));
}

bool AXPlatformNodeBase::IsRichTextField() const {
  return GetBoolAttribute(ax::mojom::BoolAttribute::kEditableRoot) &&
         GetData().HasState(ax::mojom::State::kRichlyEditable);
}

base::string16 AXPlatformNodeBase::GetInnerText() {
  if (IsTextOnlyObject())
    return GetString16Attribute(ax::mojom::StringAttribute::kName);

  base::string16 text;
  for (int i = 0; i < GetChildCount(); ++i) {
    gfx::NativeViewAccessible child_accessible = ChildAtIndex(i);
    AXPlatformNodeBase* child = FromNativeViewAccessible(child_accessible);
    if (!child)
      continue;

    text += child->GetInnerText();
  }
  return text;
}

bool AXPlatformNodeBase::IsRangeValueSupported() const {
  switch (GetData().role) {
    case ax::mojom::Role::kMeter:
    case ax::mojom::Role::kProgressIndicator:
    case ax::mojom::Role::kSlider:
    case ax::mojom::Role::kSpinButton:
    case ax::mojom::Role::kScrollBar:
      return true;
    case ax::mojom::Role::kSplitter:
      return GetData().HasState(ax::mojom::State::kFocusable);
    default:
      return false;
  }
}

base::string16 AXPlatformNodeBase::GetRangeValueText() {
  float fval;
  base::string16 value =
      GetString16Attribute(ax::mojom::StringAttribute::kValue);

  if (value.empty() &&
      GetFloatAttribute(ax::mojom::FloatAttribute::kValueForRange, &fval)) {
    value = base::NumberToString16(fval);
  }
  return value;
}

AXPlatformNodeBase* AXPlatformNodeBase::GetTable() const {
  if (!delegate_)
    return nullptr;
  AXPlatformNodeBase* table = const_cast<AXPlatformNodeBase*>(this);
  while (table && !IsTableLikeRole(table->GetData().role)) {
    gfx::NativeViewAccessible parent_accessible = table->GetParent();
    AXPlatformNodeBase* parent = FromNativeViewAccessible(parent_accessible);

    table = parent;
  }
  return table;
}

AXPlatformNodeBase* AXPlatformNodeBase::GetTableCell(int index) const {
  if (!delegate_)
    return nullptr;
  if (!IsTableLikeRole(GetData().role) &&
      !IsCellOrTableHeaderRole(GetData().role))
    return nullptr;

  AXPlatformNodeBase* table = GetTable();
  if (!table)
    return nullptr;

  return static_cast<AXPlatformNodeBase*>(
      table->delegate_->GetFromNodeID(table->delegate_->CellIndexToId(index)));
}

AXPlatformNodeBase* AXPlatformNodeBase::GetTableCell(int row,
                                                     int column) const {
  if (!IsTableLikeRole(GetData().role) &&
      !IsCellOrTableHeaderRole(GetData().role))
    return nullptr;

  if (row < 0 || row >= GetTableRowCount() || column < 0 ||
      column >= GetTableColumnCount()) {
    return nullptr;
  }

  AXPlatformNodeBase* table = GetTable();
  if (!table)
    return nullptr;

  int32_t cell_id = table->delegate_->GetCellId(row, column);
  return static_cast<AXPlatformNodeBase*>(
      table->delegate_->GetFromNodeID(cell_id));
}

int AXPlatformNodeBase::GetTableCellIndex() const {
  if (!IsCellOrTableHeaderRole(GetData().role))
    return -1;

  AXPlatformNodeBase* table = GetTable();
  if (!table)
    return -1;

  return table->delegate_->CellIdToIndex(GetData().id);
}

int AXPlatformNodeBase::GetTableColumn() const {
  return GetIntAttribute(ax::mojom::IntAttribute::kTableCellColumnIndex);
}

int AXPlatformNodeBase::GetTableColumnCount() const {
  AXPlatformNodeBase* table = GetTable();
  if (!table)
    return 0;

  return table->GetIntAttribute(ax::mojom::IntAttribute::kTableColumnCount);
}

int AXPlatformNodeBase::GetTableColumnSpan() const {
  if (!IsCellOrTableHeaderRole(GetData().role))
    return 0;

  int column_span;
  if (GetIntAttribute(ax::mojom::IntAttribute::kTableCellColumnSpan,
                      &column_span))
    return column_span;
  return 1;
}

int AXPlatformNodeBase::GetTableRow() const {
  return GetIntAttribute(ax::mojom::IntAttribute::kTableCellRowIndex);
}

int AXPlatformNodeBase::GetTableRowCount() const {
  AXPlatformNodeBase* table = GetTable();
  if (!table)
    return 0;

  return table->GetIntAttribute(ax::mojom::IntAttribute::kTableRowCount);
}

int AXPlatformNodeBase::GetTableRowSpan() const {
  if (!IsCellOrTableHeaderRole(GetData().role))
    return 0;

  int row_span;
  if (GetIntAttribute(ax::mojom::IntAttribute::kTableCellRowSpan, &row_span))
    return row_span;
  return 1;
}

bool AXPlatformNodeBase::HasCaret() {
  if (IsPlainTextField() &&
      HasIntAttribute(ax::mojom::IntAttribute::kTextSelStart) &&
      HasIntAttribute(ax::mojom::IntAttribute::kTextSelEnd)) {
    return true;
  }

  // The caret is always at the focus of the selection.
  int32_t focus_id = delegate_->GetTreeData().sel_focus_object_id;
  AXPlatformNodeBase* focus_object =
      static_cast<AXPlatformNodeBase*>(delegate_->GetFromNodeID(focus_id));

  if (!focus_object)
    return false;

  return focus_object->IsDescendantOf(this);
}

bool AXPlatformNodeBase::IsDescendantOf(AXPlatformNodeBase* ancestor) {
  if (!ancestor)
    return false;

  if (this == ancestor)
    return true;

  AXPlatformNodeBase* parent = FromNativeViewAccessible(GetParent());
  if (!parent)
    return false;

  return parent->IsDescendantOf(ancestor);
}

bool AXPlatformNodeBase::IsLeaf() {
  if (GetChildCount() == 0)
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
  switch (GetData().role) {
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

bool AXPlatformNodeBase::IsChildOfLeaf() {
  AXPlatformNodeBase* ancestor = FromNativeViewAccessible(GetParent());

  while (ancestor) {
    if (ancestor->IsLeaf())
      return true;
    ancestor = FromNativeViewAccessible(ancestor->GetParent());
  }

  return false;
}

base::string16 AXPlatformNodeBase::GetText() {
  return GetInnerText();
}

base::string16 AXPlatformNodeBase::GetValue() {
  // Expose slider value.
  if (IsRangeValueSupported()) {
    return GetRangeValueText();
  } else if (ui::IsDocument(GetData().role)) {
    // On Windows, the value of a document should be its URL.
    return base::UTF8ToUTF16(delegate_->GetTreeData().url);
  }
  base::string16 value =
      GetString16Attribute(ax::mojom::StringAttribute::kValue);

  // Some screen readers like Jaws and VoiceOver require a
  // value to be set in text fields with rich content, even though the same
  // information is available on the children.
  if (value.empty() && IsRichTextField())
    return GetInnerText();

  return value;
}

}  // namespace ui
