// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/ax_widget_obj_wrapper.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/views/accessibility/ax_aura_obj_cache.h"
#include "ui/views/accessibility/ax_aura_obj_wrapper.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {

AXWidgetObjWrapper::AXWidgetObjWrapper(Widget* widget) : widget_(widget) {
  widget->AddObserver(this);
  widget->AddRemovalsObserver(this);
}

AXWidgetObjWrapper::~AXWidgetObjWrapper() {
  if (!AXAuraObjCache::GetInstance()->is_destroying()) {
    widget_->RemoveObserver(this);
    widget_->RemoveRemovalsObserver(this);
  }
  widget_ = NULL;
}

AXAuraObjWrapper* AXWidgetObjWrapper::GetParent() {
  return AXAuraObjCache::GetInstance()->GetOrCreate(widget_->GetNativeView());
}

void AXWidgetObjWrapper::GetChildren(
    std::vector<AXAuraObjWrapper*>* out_children) {
  if (!widget_->IsVisible() || !widget_->GetRootView() ||
      !widget_->GetRootView()->visible()) {
    return;
  }

  out_children->push_back(
      AXAuraObjCache::GetInstance()->GetOrCreate(widget_->GetRootView()));
}

void AXWidgetObjWrapper::Serialize(ui::AXNodeData* out_node_data) {
  out_node_data->id = GetUniqueId().Get();
  out_node_data->role = widget_->widget_delegate()->GetAccessibleWindowRole();
  out_node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kName,
      base::UTF16ToUTF8(
          widget_->widget_delegate()->GetAccessibleWindowTitle()));
  out_node_data->location = gfx::RectF(widget_->GetWindowBoundsInScreen());
  out_node_data->state = 0;
}

const ui::AXUniqueId& AXWidgetObjWrapper::GetUniqueId() const {
  return unique_id_;
}

void AXWidgetObjWrapper::OnWidgetDestroying(Widget* widget) {
  AXAuraObjCache::GetInstance()->Remove(widget);
}

void AXWidgetObjWrapper::OnWidgetClosing(Widget* widget) {
  AXAuraObjCache::GetInstance()->Remove(widget);
}

void AXWidgetObjWrapper::OnWidgetVisibilityChanged(Widget*, bool) {
  // If a widget changes visibility it may affect what's focused, in particular
  // when a widget that contains the focused view gets hidden.
  AXAuraObjCache::GetInstance()->OnFocusedViewChanged();
}

void AXWidgetObjWrapper::OnWillRemoveView(Widget* widget, View* view) {
  AXAuraObjCache::GetInstance()->RemoveViewSubtree(view);
}

}  // namespace views
