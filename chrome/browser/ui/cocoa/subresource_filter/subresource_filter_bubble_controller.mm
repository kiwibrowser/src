// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/subresource_filter/subresource_filter_bubble_controller.h"

#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "components/strings/grit/components_strings.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#import "ui/base/cocoa/controls/hyperlink_button_cell.h"
#include "ui/base/l10n/l10n_util_mac.h"

@interface SubresourceFilterBubbleController () {
  NSButton* manageCheckbox_;
  NSButton* learnMoreLink_;
}
@end

@implementation SubresourceFilterBubbleController

- (id)initWithModel:(ContentSettingBubbleModel*)contentSettingBubbleModel
        webContents:(content::WebContents*)webContents
       parentWindow:(NSWindow*)parentWindow
         decoration:(ContentSettingDecoration*)decoration
         anchoredAt:(NSPoint)anchoredAt {
  NSRect contentRect = NSMakeRect(196, 376, 316, 154);
  base::scoped_nsobject<InfoBubbleWindow> window([[InfoBubbleWindow alloc]
      initWithContentRect:contentRect
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);

  // Disable animations - otherwise, the window/controller will outlive the web
  // contents it's associated with.
  [window setAllowedAnimations:info_bubble::kAnimateNone];

  [super initWithModel:contentSettingBubbleModel
           webContents:webContents
                window:window
          parentWindow:parentWindow
            decoration:decoration
            anchoredAt:anchoredAt];
  return self;
}

- (void)awakeFromNib {
  [self loadView];
  [super awakeFromNib];
}

- (void)layoutView {
  [super layoutView];
  [self initializeManageCheckbox];
}

- (void)loadView {
  messageLabel_ =
      [[NSTextField alloc] initWithFrame:NSMakeRect(18, 96, 282, 28)];
  [messageLabel_ setEditable:NO];
  [messageLabel_ setBordered:NO];
  [self.window.contentView addSubview:messageLabel_];
  [messageLabel_ release];

  // Set up the "Learn more" link.
  // TODO(csharrison): Migrate this to use the help icon to align with
  // Windows/Linux.
  NSString* linkText = l10n_util::GetNSString(IDS_LEARN_MORE);
  learnMoreLink_ = [[NSButton alloc] initWithFrame:NSMakeRect(18, 76, 282, 28)];
  base::scoped_nsobject<HyperlinkButtonCell> cell(
      [[HyperlinkButtonCell alloc] initTextCell:linkText]);
  [cell setAlignment:NSNaturalTextAlignment];
  [cell setControlSize:NSSmallControlSize];
  [learnMoreLink_ setCell:cell];
  [GTMUILocalizerAndLayoutTweaker sizeToFitView:learnMoreLink_];
  [learnMoreLink_ setTarget:self];
  [learnMoreLink_ setAction:@selector(learnMoreLinkClicked:)];
  [self.window.contentView addSubview:learnMoreLink_];
  [learnMoreLink_ release];

  manageCheckbox_ =
      [[NSButton alloc] initWithFrame:NSMakeRect(18, 35, 282, 28)];
  [manageCheckbox_ setButtonType:NSSwitchButton];
  [manageCheckbox_ setState:NSOffState];
  [self.window.contentView addSubview:manageCheckbox_];
  [manageCheckbox_ setAction:@selector(manageCheckboxChecked:)];
  [manageCheckbox_ release];

  doneButton_ = [[NSButton alloc] initWithFrame:NSMakeRect(210, 10, 90, 28)];
  [doneButton_ setBezelStyle:NSRoundedBezelStyle];
  [doneButton_ highlight:YES];
  [doneButton_ setTitle:l10n_util::GetNSString(IDS_OK)];
  [self.window.contentView addSubview:doneButton_];
  [doneButton_ setAction:@selector(closeBubble:)];
  [doneButton_ release];
}

- (void)initializeManageCheckbox {
  if (!manageCheckbox_)
    return;

  NSString* label =
      base::SysUTF16ToNSString([super model]->bubble_content().manage_text);
  [manageCheckbox_ setTitle:label];

  CGFloat deltaY =
      [GTMUILocalizerAndLayoutTweaker sizeToFitView:manageCheckbox_].height;
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += deltaY;
  [[self window] setFrame:windowFrame display:NO];
  NSRect manageCheckboxFrame = [manageCheckbox_ frame];
  manageCheckboxFrame.origin.y -= deltaY;
  [manageCheckbox_ setFrame:manageCheckboxFrame];
}

// Callback for "manage" checkbox button.
- (void)manageCheckboxChecked:(id)sender {
  bool isChecked = [sender state] == NSOnState;
  [super model]->OnManageCheckboxChecked(isChecked);
  [self layoutView];
}

// Callback for clicking on the "Learn more" link.
- (void)learnMoreLinkClicked:(id)sender {
  [super model]->OnLearnMoreClicked();
  [self close];
}

// For testing.

- (id)messageLabel {
  return messageLabel_;
}

- (id)learnMoreLink {
  return learnMoreLink_;
}

- (id)manageCheckbox {
  return manageCheckbox_;
}

- (id)doneButton {
  return doneButton_;
}
@end
