// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_COCOA_H_

#include "base/macros.h"
#include "chrome/browser/ui/toolbar/media_router_action_platform_delegate.h"

// The Cocoa platform delegate for the Media Router component action.
class MediaRouterActionPlatformDelegateCocoa :
    public MediaRouterActionPlatformDelegate {
 public:
  explicit MediaRouterActionPlatformDelegateCocoa(Browser* browser);
  ~MediaRouterActionPlatformDelegateCocoa() override;

  // MediaRouterActionPlatformDelegate:
  bool CloseOverflowMenuIfOpen() override;

 private:
  // The corresponding browser.
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterActionPlatformDelegateCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_COCOA_H_
