// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALLED_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALLED_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"

class Browser;
class ExtensionInstalledBubble;
@class HyperlinkTextView;
@class HoverCloseButton;
@class BubbleSyncPromoController;

namespace extensions {
class Extension;
}

namespace extension_installed_bubble {

// Maximum height or width of extension's icon (corresponds to Windows & GTK).
const int kIconSize = 43;

// Outer vertical margin for text, icon, and closing x.
const int kOuterVerticalMargin = 15;

// Inner vertical margin for text messages.
const int kInnerVerticalMargin = 10;

// An offset we apply to position the point of the bubble's arrow pointing at
// the NewTabButton.
const int kAppsBubbleArrowOffset = 4;

// We use a different kind of notification for each of these extension types.
enum ExtensionType {
  kApp,
  kExtension,
};

}  // namespace extension_installed_bubble

// Controller for the extension installed bubble.  This bubble pops up after
// an extension has been installed to inform the user that the install happened
// properly, and to let the user know how to manage this extension in the
// future.
@interface ExtensionInstalledBubbleController : BaseBubbleController {
 @private
  const extensions::Extension* extension_;  // weak
  Browser* browser_;  // weak
  base::scoped_nsobject<NSImage> icon_;

  extension_installed_bubble::ExtensionType type_;

  // A weak reference to the bubble. It's owned by the BubbleManager.
  ExtensionInstalledBubble* installedBubble_;

  // The controller for the sync promo.
  base::scoped_nsobject<BubbleSyncPromoController> syncPromoController_;

  // References below are weak, being obtained from the nib.
  IBOutlet HoverCloseButton* closeButton_;
  IBOutlet NSImageView* iconImage_;
  IBOutlet NSTextField* heading_;
  // Only shown for browser actions, page actions and omnibox keywords.
  IBOutlet NSTextField* howToUse_;
  IBOutlet NSTextField* howToManage_;
  // Only shown for app installs.
  IBOutlet NSButton* appShortcutLink_;
  // Only shown for extensions with commands.
  IBOutlet NSButton* manageShortcutLink_;
  // Only shown if the sign-in promo is active.
  IBOutlet NSView* promoContainer_;
}

@property(nonatomic, readonly) ExtensionInstalledBubble* installedBubble;
@property(nonatomic, readonly) NSView* heading;
@property(nonatomic, readonly) NSView* closeButton;
@property(nonatomic, readonly) NSView* howToUse;
@property(nonatomic, readonly) NSView* howToManage;
@property(nonatomic, readonly) NSView* appInstalledShortcutLink;
@property(nonatomic, readonly) NSView* manageShortcutLink;
@property(nonatomic, readonly) NSView* promoContainer;
@property(nonatomic, readonly) NSView* iconImage;

// Initialize the window. It will be shown by the BubbleManager.
- (id)initWithParentWindow:(NSWindow*)parentWindow
           extensionBubble:(ExtensionInstalledBubble*)extensionBubble;

// Action for close button.
- (IBAction)closeWindow:(id)sender;

// Displays the extension installed bubble. This callback is triggered by
// the extensionObserver when the extension has completed loading.
- (void)showWindow:(id)sender;

// Opens the shortcut configuration UI.
- (IBAction)onManageShortcutClicked:(id)sender;

// Shows the new app installed animation.
- (IBAction)onAppShortcutClicked:(id)sender;

// Should be called by the extension bridge to close this window.
- (void)doClose;

@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALLED_BUBBLE_CONTROLLER_H_
