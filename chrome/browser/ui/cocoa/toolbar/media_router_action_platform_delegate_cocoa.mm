// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/toolbar/media_router_action_platform_delegate_cocoa.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "ui/base/ui_features.h"

// static
std::unique_ptr<MediaRouterActionPlatformDelegate>
MediaRouterActionPlatformDelegate::CreateCocoa(Browser* browser) {
  return base::WrapUnique(new MediaRouterActionPlatformDelegateCocoa(browser));
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
std::unique_ptr<MediaRouterActionPlatformDelegate>
MediaRouterActionPlatformDelegate::Create(Browser* browser) {
  return CreateCocoa(browser);
}
#endif

MediaRouterActionPlatformDelegateCocoa::MediaRouterActionPlatformDelegateCocoa(
    Browser* browser)
    : MediaRouterActionPlatformDelegate(),
      browser_(browser) {
  DCHECK(browser_);
}

MediaRouterActionPlatformDelegateCocoa::
    ~MediaRouterActionPlatformDelegateCocoa() {
}

bool MediaRouterActionPlatformDelegateCocoa::CloseOverflowMenuIfOpen() {
  // TODO(apacible): This should be factored to share code with extension
  // actions.
  AppMenuController* appMenuController =
      [[[BrowserWindowController
          browserWindowControllerForWindow:
              browser_->window()->GetNativeWindow()]
          toolbarController] appMenuController];
  if (![appMenuController isMenuOpen])
    return false;

  [appMenuController cancel];
  return true;
}
