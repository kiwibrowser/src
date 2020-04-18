// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/guest_mode.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "content/browser/browser_plugin/browser_plugin_guest.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"

namespace content {

// static
bool GuestMode::IsCrossProcessFrameGuest(WebContents* web_contents) {
  BrowserPluginGuest* browser_plugin_guest =
      static_cast<WebContentsImpl*>(web_contents)->GetBrowserPluginGuest();

  if (!browser_plugin_guest ||
      !browser_plugin_guest->can_use_cross_process_frames()) {
    return false;
  }

  return base::FeatureList::IsEnabled(::features::kGuestViewCrossProcessFrames);
}

}  // namespace content
