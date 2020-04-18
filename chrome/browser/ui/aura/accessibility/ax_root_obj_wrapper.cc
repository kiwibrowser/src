// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/aura/accessibility/ax_root_obj_wrapper.h"

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/channel_info.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/aura/window.h"
#include "ui/views/accessibility/ax_aura_obj_cache.h"
#include "ui/views/accessibility/ax_window_obj_wrapper.h"

AXRootObjWrapper::AXRootObjWrapper() : alert_window_(new aura::Window(NULL)) {
  alert_window_->Init(ui::LAYER_NOT_DRAWN);
}

AXRootObjWrapper::~AXRootObjWrapper() {
  if (alert_window_) {
    delete alert_window_;
    alert_window_ = NULL;
  }
}

views::AXAuraObjWrapper* AXRootObjWrapper::GetAlertForText(
    const std::string& text) {
  alert_window_->SetTitle(base::UTF8ToUTF16((text)));
  views::AXWindowObjWrapper* window_obj =
      static_cast<views::AXWindowObjWrapper*>(
          views::AXAuraObjCache::GetInstance()->GetOrCreate(alert_window_));
  window_obj->set_is_alert(true);
  return window_obj;
}

bool AXRootObjWrapper::HasChild(views::AXAuraObjWrapper* child) {
  std::vector<views::AXAuraObjWrapper*> children;
  GetChildren(&children);
  return base::ContainsValue(children, child);
}

views::AXAuraObjWrapper* AXRootObjWrapper::GetParent() {
  return NULL;
}

void AXRootObjWrapper::GetChildren(
    std::vector<views::AXAuraObjWrapper*>* out_children) {
  views::AXAuraObjCache::GetInstance()->GetTopLevelWindows(out_children);
  out_children->push_back(
      views::AXAuraObjCache::GetInstance()->GetOrCreate(alert_window_));
}

void AXRootObjWrapper::Serialize(ui::AXNodeData* out_node_data) {
  out_node_data->id = unique_id_.Get();
  out_node_data->role = ax::mojom::Role::kDesktop;
  out_node_data->AddStringAttribute(ax::mojom::StringAttribute::kChromeChannel,
                                    chrome::GetChannelName());
}

const ui::AXUniqueId& AXRootObjWrapper::GetUniqueId() const {
  return unique_id_;
}
