// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/native_browser_frame_factory.h"

#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/ui/views/frame/browser_frame_ash.h"
#include "chrome/browser/ui/views/frame/browser_frame_mus.h"
#include "services/service_manager/runner/common/client_util.h"

NativeBrowserFrame* NativeBrowserFrameFactory::Create(
    BrowserFrame* browser_frame,
    BrowserView* browser_view) {
  if (chromeos::GetAshConfig() == ash::Config::MASH)
    return new BrowserFrameMus(browser_frame, browser_view);
  return new BrowserFrameAsh(browser_frame, browser_view);
}
