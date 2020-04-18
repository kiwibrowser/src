// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_H_
#define CHROME_BROWSER_UI_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_H_

#include <memory>

#include "build/build_config.h"

class Browser;

class MediaRouterActionPlatformDelegate {
 public:
  MediaRouterActionPlatformDelegate() {}
  virtual ~MediaRouterActionPlatformDelegate() {}

  // Returns a created MediaRouterActionPlatformDelegate. This is defined in the
  // platform-specific implementation for the class.
  static std::unique_ptr<MediaRouterActionPlatformDelegate> Create(
      Browser* browser);
#if defined(OS_MACOSX)
  // Temporary shim for Polychrome. See bottom of first comment in
  // https://crbug.com/80495 for details.
  static std::unique_ptr<MediaRouterActionPlatformDelegate> CreateCocoa(
      Browser* browser);
#endif

  // Closes the overflow menu, if it was open. Returns whether or not the
  // overflow menu was closed.
  virtual bool CloseOverflowMenuIfOpen() = 0;
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_H_
