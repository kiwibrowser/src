// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/config.h"
#include "build/build_config.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view_ash.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view_mus.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace chrome {

BrowserNonClientFrameView* CreateBrowserNonClientFrameView(
    BrowserFrame* frame,
    BrowserView* browser_view) {
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    BrowserNonClientFrameViewMus* frame_view =
        new BrowserNonClientFrameViewMus(frame, browser_view);
    frame_view->Init();
    return frame_view;
  }
  BrowserNonClientFrameViewAsh* frame_view =
      new BrowserNonClientFrameViewAsh(frame, browser_view);
  frame_view->Init();
  return frame_view;
}

}  // namespace chrome
