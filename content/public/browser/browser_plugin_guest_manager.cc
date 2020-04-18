// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/browser_plugin_guest_manager.h"

namespace content {

WebContents* BrowserPluginGuestManager::GetGuestByInstanceID(
    int owner_process_id,
    int browser_plugin_instance_id) {
  return nullptr;
}

bool BrowserPluginGuestManager::ForEachGuest(WebContents* embedder_web_contents,
                                             const GuestCallback& callback) {
  return false;
}

WebContents* BrowserPluginGuestManager::GetFullPageGuest(
    WebContents* embedder_web_contents) {
  return nullptr;
}

}  // content

