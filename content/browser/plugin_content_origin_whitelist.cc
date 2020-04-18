// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/plugin_content_origin_whitelist.h"

#include "content/common/frame_messages.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

PluginContentOriginWhitelist::PluginContentOriginWhitelist(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {
}

PluginContentOriginWhitelist::~PluginContentOriginWhitelist() {
}

void PluginContentOriginWhitelist::RenderFrameCreated(
    RenderFrameHost* render_frame_host) {
  if (!whitelist_.empty()) {
    render_frame_host->Send(new FrameMsg_UpdatePluginContentOriginWhitelist(
        render_frame_host->GetRoutingID(), whitelist_));
  }
}

bool PluginContentOriginWhitelist::OnMessageReceived(
    const IPC::Message& message,
    RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PluginContentOriginWhitelist, message)
    IPC_MESSAGE_HANDLER(FrameHostMsg_PluginContentOriginAllowed,
                        OnPluginContentOriginAllowed)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void PluginContentOriginWhitelist::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // We expect RenderFrames to clear their replicated whitelist independently.
  whitelist_.clear();
}

void PluginContentOriginWhitelist::OnPluginContentOriginAllowed(
    const url::Origin& content_origin) {
  whitelist_.insert(content_origin);

  web_contents()->SendToAllFrames(
      new FrameMsg_UpdatePluginContentOriginWhitelist(
          MSG_ROUTING_NONE, whitelist_));
}

}  // namespace content
