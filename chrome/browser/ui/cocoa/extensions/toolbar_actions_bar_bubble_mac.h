// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_TOOLBAR_ACTIONS_BAR_BUBBLE_MAC_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_TOOLBAR_ACTIONS_BAR_BUBBLE_MAC_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#import "chrome/browser/ui/cocoa/base_bubble_controller.h"

class ToolbarActionsBarBubbleDelegate;

// A bubble that hangs off the toolbar actions bar, with a minimum of a heading,
// some text content, and an "action" button. The bubble can also, optionally,
// have a learn more link and/or a dismiss button.
@interface ToolbarActionsBarBubbleMac : BaseBubbleController {
  // Whether or not the bubble has been acknowledged.
  BOOL acknowledged_;

  // True if the bubble is anchored to an action in the toolbar actions bar.
  BOOL anchoredToAction_;

  // The action button. The exact meaning of this is dependent on the bubble.
  // Required.
  NSButton* actionButton_;

  // The text to display in the body of the bubble, excluding the item list.
  // Optional.
  NSTextField* bodyText_;

  // The list of items to display. Optional.
  NSTextField* itemList_;

  // The dismiss button. Optional.
  NSButton* dismissButton_;

  // The extra view text as a link-style button. Optional.
  NSButton* link_;

  // The extra view text as a label. Optional.
  NSTextField* label_;

  // The extra view icon that can accompany the extra view text. Optional.
  NSImageView* iconView_;

  // This bubble's delegate.
  std::unique_ptr<ToolbarActionsBarBubbleDelegate> delegate_;
}

// Creates the bubble for a parent window but does not show it.
- (id)initWithParentWindow:(NSWindow*)parentWindow
               anchorPoint:(NSPoint)anchorPoint
          anchoredToAction:(BOOL)anchoredToAction
                  delegate:(std::unique_ptr<ToolbarActionsBarBubbleDelegate>)
                               delegate;

// Toggles animation for testing purposes.
+ (void)setAnimationEnabledForTesting:(BOOL)enabled;

@property(readonly, nonatomic) NSButton* actionButton;
@property(readonly, nonatomic) NSTextField* bodyText;
@property(readonly, nonatomic) NSTextField* itemList;
@property(readonly, nonatomic) NSButton* dismissButton;
@property(readonly, nonatomic) NSButton* link;
@property(readonly, nonatomic) NSTextField* label;
@property(readonly, nonatomic) NSImageView* iconView;

@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_TOOLBAR_ACTIONS_BAR_BUBBLE_MAC_H_
