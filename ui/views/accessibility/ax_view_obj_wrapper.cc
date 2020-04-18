// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/ax_view_obj_wrapper.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_action_data.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/events/event_utils.h"
#include "ui/views/accessibility/ax_aura_obj_cache.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

AXViewObjWrapper::AXViewObjWrapper(View* view)  : view_(view) {
  if (view->GetWidget())
    AXAuraObjCache::GetInstance()->GetOrCreate(view->GetWidget());
}

AXViewObjWrapper::~AXViewObjWrapper() {}

AXAuraObjWrapper* AXViewObjWrapper::GetParent() {
  AXAuraObjCache* cache = AXAuraObjCache::GetInstance();
  if (view_->parent())
    return cache->GetOrCreate(view_->parent());

  if (view_->GetWidget())
    return cache->GetOrCreate(view_->GetWidget());

  return NULL;
}

void AXViewObjWrapper::GetChildren(
    std::vector<AXAuraObjWrapper*>* out_children) {
  if (view_->GetViewAccessibility().IsLeaf())
    return;

  // TODO(dtseng): Need to handle |Widget| child of |View|.
  for (int i = 0; i < view_->child_count(); ++i) {
    if (!view_->child_at(i)->visible())
      continue;

    AXAuraObjWrapper* child =
        AXAuraObjCache::GetInstance()->GetOrCreate(view_->child_at(i));
    out_children->push_back(child);
  }
}

void AXViewObjWrapper::Serialize(ui::AXNodeData* out_node_data) {
  view_->GetViewAccessibility().GetAccessibleNodeData(out_node_data);
  out_node_data->id = GetUniqueId().Get();
}

const ui::AXUniqueId& AXViewObjWrapper::GetUniqueId() const {
  return view_->GetViewAccessibility().GetUniqueId();
}

bool AXViewObjWrapper::HandleAccessibleAction(const ui::AXActionData& action) {
  return view_->HandleAccessibleAction(action);
}

}  // namespace views
