// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_VIEWS_H_

#include "base/macros.h"
#include "chrome/browser/ui/toolbar/media_router_action_platform_delegate.h"

// The Views platform delegate for the Media Router component action.
class MediaRouterActionPlatformDelegateViews :
    public MediaRouterActionPlatformDelegate {
 public:
  explicit MediaRouterActionPlatformDelegateViews(Browser* browser);
  ~MediaRouterActionPlatformDelegateViews() override;

  // MediaRouterActionPlatformDelegate:
  bool CloseOverflowMenuIfOpen() override;

 private:
  // The corresponding browser.
  Browser* const browser_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterActionPlatformDelegateViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_MEDIA_ROUTER_ACTION_PLATFORM_DELEGATE_VIEWS_H_
