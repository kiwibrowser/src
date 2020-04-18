// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_platform_specific.h"

#include "build/build_config.h"

bool OpaqueBrowserFrameViewPlatformSpecific::IsUsingSystemTheme() {
  return false;
}

#if !defined(OS_LINUX)

// static
OpaqueBrowserFrameViewPlatformSpecific*
OpaqueBrowserFrameViewPlatformSpecific::Create(
    OpaqueBrowserFrameView* view,
    OpaqueBrowserFrameViewLayout* layout,
    ThemeService* theme_service) {
  return new OpaqueBrowserFrameViewPlatformSpecific();
}

#endif
