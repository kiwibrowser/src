// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_ACTION_PLATFORM_DELEGATE_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_ACTION_PLATFORM_DELEGATE_COCOA_H_

#import <Foundation/Foundation.h>

#include "base/macros.h"
#include "chrome/browser/ui/extensions/extension_action_platform_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

@class ExtensionPopupController;

// The Cocoa-specific implementation for ExtensionActionPlatformDelegate.
class ExtensionActionPlatformDelegateCocoa
    : public ExtensionActionPlatformDelegate,
      public content::NotificationObserver {
 public:
  ExtensionActionPlatformDelegateCocoa(
      ExtensionActionViewController* controller);
  ~ExtensionActionPlatformDelegateCocoa() override;

 private:
  // ExtensionActionPlatformDelegate:
  void RegisterCommand() override;
  void OnDelegateSet() override;
  void ShowPopup(
      std::unique_ptr<extensions::ExtensionViewHost> host,
      bool grant_tab_permissions,
      ExtensionActionViewController::PopupShowAction show_action) override;
  void CloseOverflowMenu() override;
  void ShowContextMenu() override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Returns the point at which the popup should be shown in window coordinates.
  NSPoint GetPopupPoint() const;

  // The main controller for this extension action.
  ExtensionActionViewController* controller_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionActionPlatformDelegateCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_ACTION_PLATFORM_DELEGATE_COCOA_H_
