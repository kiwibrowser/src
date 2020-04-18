// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/host_zoom_map_observer.h"

#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/host_zoom_map_impl.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/storage_partition.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace content {

HostZoomMapObserver::HostZoomMapObserver(WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

HostZoomMapObserver::~HostZoomMapObserver() {}

void HostZoomMapObserver::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;

  RenderFrameHost* render_frame_host =
      navigation_handle->GetRenderFrameHost();
  const auto& entry = host_zoom_ptrs_.find(render_frame_host);
  if (entry == host_zoom_ptrs_.end())
    return;

  const mojom::HostZoomAssociatedPtr& host_zoom = entry->second;
  DCHECK(host_zoom.is_bound());
  if (host_zoom.encountered_error())
    return;

  RenderProcessHost* render_process_host = render_frame_host->GetProcess();
  HostZoomMapImpl* host_zoom_map = static_cast<HostZoomMapImpl*>(
      render_process_host->GetStoragePartition()->GetHostZoomMap());
  double zoom_level = host_zoom_map->GetZoomLevelForView(
      navigation_handle->GetURL(), render_process_host->GetID(),
      render_frame_host->GetRenderViewHost()->GetRoutingID());
  host_zoom->SetHostZoomLevel(navigation_handle->GetURL(), zoom_level);
}

void HostZoomMapObserver::RenderFrameCreated(RenderFrameHost* rfh) {
  mojom::HostZoomAssociatedPtr host_zoom;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&host_zoom);
  host_zoom_ptrs_[rfh] = std::move(host_zoom);
}

void HostZoomMapObserver::RenderFrameDeleted(RenderFrameHost* rfh) {
  const auto& entry = host_zoom_ptrs_.find(rfh);
  DCHECK(entry != host_zoom_ptrs_.end());
  host_zoom_ptrs_.erase(entry);
}

}  // namespace content
