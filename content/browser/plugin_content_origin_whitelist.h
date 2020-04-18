// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PLUGIN_CONTENT_ORIGIN_WHITELIST_H_
#define CONTENT_BROWSER_PLUGIN_CONTENT_ORIGIN_WHITELIST_H_

#include <set>

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/origin.h"

namespace content {

class WebContents;

// This class manages the tab-wide list of temporarily whitelisted plugin
// content origins that are exempt from power saving.
//
// RenderFrames report content origins that should be whitelisted via IPC.
// This class aggregates those origins and broadcasts the total list to all
// RenderFrames owned by the tab (WebContents). This class also sends these
// origins to any newly created RenderFrames.
//
// Tab-wide whitelists are cleared by top-level navigation. RenderFrames that
// persist across top level navigations are responsible for clearing their own
// whitelists.
class PluginContentOriginWhitelist : public WebContentsObserver {
 public:
  explicit PluginContentOriginWhitelist(WebContents* web_contents);
  ~PluginContentOriginWhitelist() override;

 private:
  // WebContentsObserver implementation.
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;
  bool OnMessageReceived(const IPC::Message& message,
                         RenderFrameHost* render_frame_host) override;
  void DidFinishNavigation(NavigationHandle* navigation_handle) override;

  void OnPluginContentOriginAllowed(const url::Origin& content_origin);

  std::set<url::Origin> whitelist_;

  DISALLOW_COPY_AND_ASSIGN(PluginContentOriginWhitelist);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PLUGIN_CONTENT_ORIGIN_WHITELIST_H_
