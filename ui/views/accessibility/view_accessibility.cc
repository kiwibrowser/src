// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/view_accessibility.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/base/ui_features.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

bool IsValidRoleForViews(ax::mojom::Role role) {
  switch (role) {
    // These roles all have special meaning and shouldn't ever be
    // set on a View.
    case ax::mojom::Role::kDesktop:
    case ax::mojom::Role::kNone:
    case ax::mojom::Role::kRootWebArea:
    case ax::mojom::Role::kSvgRoot:
    case ax::mojom::Role::kUnknown:
    case ax::mojom::Role::kWebArea:
      return false;

    default:
      return true;
  }
}

}  // namespace

#if !BUILDFLAG_INTERNAL_HAS_NATIVE_ACCESSIBILITY()
// static
std::unique_ptr<ViewAccessibility> ViewAccessibility::Create(View* view) {
  return base::WrapUnique(new ViewAccessibility(view));
}
#endif

ViewAccessibility::ViewAccessibility(View* view)
    : owner_view_(view), is_leaf_(false) {}

ViewAccessibility::~ViewAccessibility() {}

const ui::AXUniqueId& ViewAccessibility::GetUniqueId() const {
  return unique_id_;
}

void ViewAccessibility::GetAccessibleNodeData(ui::AXNodeData* data) const {
  // Views may misbehave if their widget is closed; return an unknown role
  // rather than possibly crashing.
  views::Widget* widget = owner_view_->GetWidget();
  if (!widget || !widget->widget_delegate() || widget->IsClosed()) {
    data->role = ax::mojom::Role::kUnknown;
    data->SetRestriction(ax::mojom::Restriction::kDisabled);
    return;
  }

  owner_view_->GetAccessibleNodeData(data);
  if (custom_data_.role != ax::mojom::Role::kUnknown)
    data->role = custom_data_.role;

  if (custom_data_.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
    data->SetName(
        custom_data_.GetStringAttribute(ax::mojom::StringAttribute::kName));
  }

  if (custom_data_.HasStringAttribute(
          ax::mojom::StringAttribute::kDescription)) {
    data->SetDescription(custom_data_.GetStringAttribute(
        ax::mojom::StringAttribute::kDescription));
  }

  if (!data->HasStringAttribute(ax::mojom::StringAttribute::kDescription)) {
    base::string16 tooltip;
    owner_view_->GetTooltipText(gfx::Point(), &tooltip);
    // Some screen readers announce the accessible description right after the
    // accessible name. Only use the tooltip as the accessible description if
    // it's different from the name, otherwise users might be puzzled as to why
    // their screen reader is announcing the same thing twice.
    if (tooltip !=
        data->GetString16Attribute(ax::mojom::StringAttribute::kName)) {
      data->AddStringAttribute(ax::mojom::StringAttribute::kDescription,
                               base::UTF16ToUTF8(tooltip));
    }
  }

  data->location = gfx::RectF(owner_view_->GetBoundsInScreen());
  data->AddStringAttribute(ax::mojom::StringAttribute::kClassName,
                           owner_view_->GetClassName());

  if (owner_view_->IsAccessibilityFocusable())
    data->AddState(ax::mojom::State::kFocusable);

  if (!owner_view_->enabled())
    data->SetRestriction(ax::mojom::Restriction::kDisabled);

  if (!owner_view_->visible() && data->role != ax::mojom::Role::kAlert)
    data->AddState(ax::mojom::State::kInvisible);

  if (owner_view_->context_menu_controller())
    data->AddAction(ax::mojom::Action::kShowContextMenu);
}

bool ViewAccessibility::IsLeaf() const {
  return is_leaf_;
}

void ViewAccessibility::OverrideRole(ax::mojom::Role role) {
  DCHECK(IsValidRoleForViews(role));

  custom_data_.role = role;
}

void ViewAccessibility::OverrideName(const std::string& name) {
  custom_data_.SetName(name);
}

void ViewAccessibility::OverrideName(const base::string16& name) {
  custom_data_.SetName(base::UTF16ToUTF8(name));
}

void ViewAccessibility::OverrideDescription(const std::string& description) {
  DCHECK(!custom_data_.HasStringAttribute(
      ax::mojom::StringAttribute::kDescription));
  custom_data_.AddStringAttribute(ax::mojom::StringAttribute::kDescription,
                                  description);
}

void ViewAccessibility::OverrideIsLeaf() {
  is_leaf_ = true;
}

gfx::NativeViewAccessible ViewAccessibility::GetNativeObject() {
  return nullptr;
}

}  // namespace views
