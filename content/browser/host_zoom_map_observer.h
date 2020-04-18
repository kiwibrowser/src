// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_HOST_ZOOM_MAP_OBSERVER_H_
#define CONTENT_BROWSER_HOST_ZOOM_MAP_OBSERVER_H_

#include "content/common/host_zoom.mojom.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class RenderFrameHost;

class HostZoomMapObserver : private WebContentsObserver {
 public:
  explicit HostZoomMapObserver(WebContents* web_contents);
  ~HostZoomMapObserver() override;

 private:
  // WebContentsObserver implementation:
  void ReadyToCommitNavigation(NavigationHandle* navigation_handle) override;
  void RenderFrameCreated(RenderFrameHost* rfh) override;
  void RenderFrameDeleted(RenderFrameHost* rfh) override;

  std::map<RenderFrameHost*, mojom::HostZoomAssociatedPtr> host_zoom_ptrs_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_HOST_ZOOM_MAP_OBSERVER_H_
