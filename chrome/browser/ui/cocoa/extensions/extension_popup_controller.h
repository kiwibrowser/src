// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_POPUP_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_POPUP_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <string>

#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#include "content/public/browser/notification_registrar.h"

class Browser;
class ExtensionPopupNotificationBridge;
class ExtensionPopupContainer;

namespace extensions {
class ExtensionViewHost;
}

// This controller manages a single browser action popup that can appear once a
// user has clicked on a browser action button. It instantiates the extension
// popup view showing the content and resizes the window to accomodate any size
// changes as they occur.
//
// There can only be one browser action popup open at a time, so a static
// variable holds a reference to the current popup.
@interface ExtensionPopupController : BaseBubbleController {
 @private
  // The native extension view retrieved from the extension host. Weak.
  NSView* extensionView_;

  // The current frame of the extension view. Cached to prevent setting the
  // frame if the size hasn't changed.
  NSRect extensionFrame_;

  // The extension host object.
  std::unique_ptr<extensions::ExtensionViewHost> host_;

  content::NotificationRegistrar registrar_;
  std::unique_ptr<ExtensionPopupNotificationBridge> notificationBridge_;
  std::unique_ptr<ExtensionPopupContainer> container_;

  std::string extensionId_;

  // Whether the popup has a devtools window attached to it.
  BOOL beingInspected_;

  // There's an extra windowDidResignKey: notification right after a
  // ConstrainedWindow closes that should be ignored.
  BOOL ignoreWindowDidResignKey_;

  // The size once the ExtensionView has loaded.
  NSSize pendingSize_;
}

// Starts the process of showing the given popup URL. Instantiates an
// ExtensionPopupController with the parent window retrieved from |browser|, a
// host for the popup created by the extension process manager specific to the
// browser profile and the remaining arguments |anchoredAt| and with an
// appropriate arrow. |anchoredAt| is expected to be in the window's coordinates
// along the bottom edge of the browser action button.
// The actual display of the popup is delayed until the page contents finish
// loading in order to minimize UI flashing and resizing.
// Passing YES to |devMode| will launch the webkit inspector for the popup,
// and prevent the popup from closing when focus is lost.  It will be closed
// after the inspector is closed, or another popup is opened.
+ (ExtensionPopupController*)
         host:(std::unique_ptr<extensions::ExtensionViewHost>)host
    inBrowser:(Browser*)browser
   anchoredAt:(NSPoint)anchoredAt
      devMode:(BOOL)devMode;

// Returns the controller used to display the popup being shown. If no popup is
// currently open, then nil is returned. Static because only one extension popup
// window can be open at a time.
+ (ExtensionPopupController*)popup;

// Whether the popup is in the process of closing (via Core Animation).
- (BOOL)isClosing;

// Show the dev tools attached to the popup.
- (void)showDevTools;

// Set whether the popup is being inspected or not. If it is being inspected
// it will not be hidden when it loses focus.
- (void)setBeingInspected:(BOOL)beingInspected;

@property(readonly, nonatomic) std::string extensionId;

@end

@interface ExtensionPopupController(TestingAPI)
// Sets whether or not animations are enabled.
+ (void)setAnimationsEnabledForTesting:(BOOL)enabled;
// Returns a weak pointer to the current popup's view.
- (NSView*)view;
// Returns the minimum allowed size for an extension popup.
+ (NSSize)minPopupSize;
// Returns the maximum allowed size for an extension popup.
+ (NSSize)maxPopupSize;
@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_POPUP_CONTROLLER_H_
