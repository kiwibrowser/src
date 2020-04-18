// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <map>
#include <memory>

#include "base/mac/availability.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/omnibox_decoration_bubble_controller.h"
#include "content/public/common/media_stream_request.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

class ContentSettingBubbleModel;
class ContentSettingBubbleModelOwnerBridge;
class ContentSettingBubbleWebContentsObserverBridge;
class ContentSettingDecoration;
class ContentSettingMediaMenuModel;
@class InfoBubbleView;

namespace content {
class WebContents;
}

namespace content_setting_bubble {
// For every "show popup" button, remember the index of the popup tab contents
// it should open when clicked.
using PopupLinks = std::map<NSButton*, int>;

// For every media menu button, remember the components associated with the
// menu button.
struct MediaMenuParts {
  MediaMenuParts(content::MediaStreamType type, NSTextField* label);
  ~MediaMenuParts();

  content::MediaStreamType type;
  NSTextField* label;  // Weak.
  std::unique_ptr<ContentSettingMediaMenuModel> model;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaMenuParts);
};

// Comparator used by MediaMenuPartsMap to order its keys.
struct compare_button {
  bool operator()(NSPopUpButton *const a, NSPopUpButton *const b) const {
    return [a tag] < [b tag];
  }
};
using MediaMenuPartsMap =
    std::map<NSPopUpButton*, std::unique_ptr<MediaMenuParts>, compare_button>;
}  // namespace content_setting_bubble

// Manages a "content blocked" bubble.
@interface ContentSettingBubbleController
    : OmniboxDecorationBubbleController<NSTouchBarDelegate> {
 @protected
  IBOutlet NSTextField* titleLabel_;
  IBOutlet NSTextField* messageLabel_;
  IBOutlet NSMatrix* allowBlockRadioGroup_;

  IBOutlet NSButton* manageButton_;
  IBOutlet NSButton* doneButton_;
  IBOutlet NSButton* loadButton_;
  IBOutlet NSButton* infoButton_;

  std::unique_ptr<ContentSettingBubbleModelOwnerBridge> modelOwnerBridge_;

 @private
  // The container for the bubble contents of the geolocation bubble.
  IBOutlet NSView* contentsContainer_;

  IBOutlet NSTextField* blockedResourcesField_;

  std::unique_ptr<ContentSettingBubbleWebContentsObserverBridge>
      observerBridge_;
  content_setting_bubble::PopupLinks popupLinks_;
  content_setting_bubble::MediaMenuPartsMap mediaMenus_;

  // Y coordinate of the first list item.
  int topLinkY_;

  // The omnibox icon the bubble is anchored to.
  ContentSettingDecoration* decoration_;  // weak
}

// Initializes the controller using the model. Takes ownership of
// |settingsBubbleModel| but not of the other objects. This is intended to be
// invoked by subclasses and most callers should invoke the showForModel
// convenience constructor.
- (id)initWithModel:(ContentSettingBubbleModel*)settingsBubbleModel
        webContents:(content::WebContents*)webContents
             window:(NSWindow*)window
       parentWindow:(NSWindow*)parentWindow
         decoration:(ContentSettingDecoration*)decoration
         anchoredAt:(NSPoint)anchoredAt;

// Creates and shows a content blocked bubble. Takes ownership of
// |contentSettingBubbleModel| but not of the other objects.
+ (ContentSettingBubbleController*)
showForModel:(ContentSettingBubbleModel*)contentSettingBubbleModel
 webContents:(content::WebContents*)webContents
parentWindow:(NSWindow*)parentWindow
  decoration:(ContentSettingDecoration*)decoration
  anchoredAt:(NSPoint)anchoredAt;

// Override to customize the touch bar.
- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2));

// Initializes the layout of all the UI elements.
- (void)layoutView;

// Callback for the "don't block / continue blocking" radio group.
- (IBAction)allowBlockToggled:(id)sender;

// Callback for "close" button.
- (IBAction)closeBubble:(id)sender;

// Callback for "manage" button.
- (IBAction)manageBlocking:(id)sender;

// Callback for "info" link.
- (IBAction)showMoreInfo:(id)sender;

// Callback for "load" (plugins, mixed script) button.
- (IBAction)load:(id)sender;

// Callback for "Learn More" link.
- (IBAction)learnMoreLinkClicked:(id)sender;

// Callback for "media menu" button.
- (IBAction)mediaMenuChanged:(id)sender;

@end

@interface ContentSettingBubbleController (Protected)

- (ContentSettingBubbleModel*)model;

@end

@interface ContentSettingBubbleController (TestingAPI)

// Returns the weak reference to the |mediaMenus_|.
- (content_setting_bubble::MediaMenuPartsMap*)mediaMenus;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_COCOA_H_
