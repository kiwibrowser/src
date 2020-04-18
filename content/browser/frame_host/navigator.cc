// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/navigator.h"

#include "base/time/time.h"
#include "content/public/browser/stream_handle.h"

namespace content {

NavigatorDelegate* Navigator::GetDelegate() {
  return nullptr;
}

NavigationController* Navigator::GetController() {
  return nullptr;
}

bool Navigator::NavigateToPendingEntry(
    FrameTreeNode* frame_tree_node,
    const FrameNavigationEntry& frame_entry,
    ReloadType reload_type,
    bool is_same_document_history_load,
    std::unique_ptr<NavigationUIData> navigation_ui_data) {
  return false;
}

bool Navigator::NavigateToEntry(
    FrameTreeNode* frame_tree_node,
    const FrameNavigationEntry& frame_entry,
    const NavigationEntryImpl& entry,
    ReloadType reload_type,
    bool is_same_document_history_load,
    bool is_history_navigation_in_new_child,
    bool is_pending_entry,
    const scoped_refptr<network::ResourceRequestBody>& post_body,
    std::unique_ptr<NavigationUIData> navigation_ui_data) {
  return false;
}

bool Navigator::StartHistoryNavigationInNewSubframe(
    RenderFrameHostImpl* render_frame_host,
    const GURL& default_url) {
  return false;
}

base::TimeTicks Navigator::GetCurrentLoadStart() {
  return base::TimeTicks::Now();
}

void Navigator::OnBeginNavigation(
    FrameTreeNode* frame_tree_node,
    const CommonNavigationParams& common_params,
    mojom::BeginNavigationParamsPtr begin_params,
    scoped_refptr<network::SharedURLLoaderFactory> blob_url_loader_factory) {}

}  // namespace content
