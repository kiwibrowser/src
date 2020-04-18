// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/aura/accessibility/ax_tree_source_aura.h"

#include <stddef.h>

#include <vector>

#include "chrome/browser/ui/aura/accessibility/automation_manager_aura.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/accessibility/ax_aura_obj_cache.h"
#include "ui/views/accessibility/ax_aura_obj_wrapper.h"
#include "ui/views/accessibility/ax_view_obj_wrapper.h"

#if defined(TOOLKIT_VIEWS)
#include "ui/views/controls/webview/webview.h"  // nogncheck
#endif

using views::AXAuraObjCache;
using views::AXAuraObjWrapper;

AXTreeSourceAura::AXTreeSourceAura() {
  root_.reset(new AXRootObjWrapper());
}

AXTreeSourceAura::~AXTreeSourceAura() {
  root_.reset();
}

bool AXTreeSourceAura::HandleAccessibleAction(const ui::AXActionData& action) {
  int id = action.target_node_id;

  // In Views, we only support setting the selection within a single node,
  // not across multiple nodes like on the web.
  if (action.action == ax::mojom::Action::kSetSelection) {
    if (action.anchor_node_id != action.focus_node_id) {
      NOTREACHED();
      return false;
    }
    id = action.anchor_node_id;
  }

  AXAuraObjWrapper* obj = AXAuraObjCache::GetInstance()->Get(id);
  if (obj)
    return obj->HandleAccessibleAction(action);

  return false;
}

bool AXTreeSourceAura::GetTreeData(ui::AXTreeData* tree_data) const {
  tree_data->tree_id = 0;
  tree_data->loaded = true;
  tree_data->loading_progress = 1.0;
  AXAuraObjWrapper* focus = AXAuraObjCache::GetInstance()->GetFocus();
  if (focus)
    tree_data->focus_id = focus->GetUniqueId().Get();
  return true;
}

AXAuraObjWrapper* AXTreeSourceAura::GetRoot() const {
  return root_.get();
}

AXAuraObjWrapper* AXTreeSourceAura::GetFromId(int32_t id) const {
  if (id == root_->GetUniqueId().Get())
    return root_.get();
  return AXAuraObjCache::GetInstance()->Get(id);
}

int32_t AXTreeSourceAura::GetId(AXAuraObjWrapper* node) const {
  return node->GetUniqueId().Get();
}

void AXTreeSourceAura::GetChildren(
    AXAuraObjWrapper* node,
    std::vector<AXAuraObjWrapper*>* out_children) const {
  node->GetChildren(out_children);
}

AXAuraObjWrapper* AXTreeSourceAura::GetParent(AXAuraObjWrapper* node) const {
  AXAuraObjWrapper* parent = node->GetParent();
  if (!parent && node->GetUniqueId() != root_->GetUniqueId())
    parent = root_.get();
  return parent;
}

bool AXTreeSourceAura::IsValid(AXAuraObjWrapper* node) const {
  return node != nullptr;
}

bool AXTreeSourceAura::IsEqual(AXAuraObjWrapper* node1,
                               AXAuraObjWrapper* node2) const {
  if (!node1 || !node2)
    return false;

  return node1->GetUniqueId() == node2->GetUniqueId();
}

AXAuraObjWrapper* AXTreeSourceAura::GetNull() const {
  return NULL;
}

void AXTreeSourceAura::SerializeNode(AXAuraObjWrapper* node,
                                     ui::AXNodeData* out_data) const {
  node->Serialize(out_data);

  // Convert the global coordinates reported by each AXAuraObjWrapper
  // into parent-relative coordinates to be used in the accessibility
  // tree. That way when any Window, Widget, or View moves (and fires
  // a location changed event), its descendants all move relative to
  // it by default.
  AXAuraObjWrapper* parent = node->GetParent();
  if (parent) {
    ui::AXNodeData parent_data;
    parent->Serialize(&parent_data);
    out_data->location.Offset(-parent_data.location.OffsetFromOrigin());
    out_data->offset_container_id = parent->GetUniqueId().Get();
  }

  if (out_data->role == ax::mojom::Role::kWebView) {
    views::View* view = static_cast<views::AXViewObjWrapper*>(node)->view();
    content::WebContents* contents =
        static_cast<views::WebView*>(view)->GetWebContents();
    content::RenderFrameHost* rfh = contents->GetMainFrame();
    if (rfh) {
      int ax_tree_id = rfh->GetAXTreeID();
      out_data->AddIntAttribute(ax::mojom::IntAttribute::kChildTreeId,
                                ax_tree_id);
    }
  } else if (out_data->role == ax::mojom::Role::kWindow ||
             out_data->role == ax::mojom::Role::kDialog) {
    // Add clips children flag by default to these roles.
    out_data->AddBoolAttribute(ax::mojom::BoolAttribute::kClipsChildren, true);
  }
}

std::string AXTreeSourceAura::ToString(AXAuraObjWrapper* root,
                                       std::string prefix) {
  ui::AXNodeData data;
  root->Serialize(&data);
  std::string output = prefix + data.ToString() + '\n';

  std::vector<AXAuraObjWrapper*> children;
  root->GetChildren(&children);

  prefix += prefix[0];
  for (size_t i = 0; i < children.size(); ++i)
    output += ToString(children[i], prefix);

  return output;
}
