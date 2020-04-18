// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_GLOBAL_ERROR_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_GLOBAL_ERROR_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/memory/weak_ptr.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"

class Browser;
class GlobalErrorBubbleViewBase;
class GlobalErrorWithStandardBubble;
@class GTMUILocalizerAndLayoutTweaker;
@class GTMWidthBasedTweaker;

namespace GlobalErrorBubbleControllerInternal {
class Bridge;
}

// This is a bubble view shown from the app menu to display information
// about a global error.
@interface GlobalErrorBubbleController : BaseBubbleController {
 @private
  base::WeakPtr<GlobalErrorWithStandardBubble> error_;
  std::unique_ptr<GlobalErrorBubbleControllerInternal::Bridge> bridge_;
  Browser* browser_;

  IBOutlet NSImageView* iconView_;
  IBOutlet NSTextField* title_;
  IBOutlet NSTextField* message_;
  IBOutlet NSButton* acceptButton_;
  IBOutlet NSButton* cancelButton_;
  IBOutlet GTMUILocalizerAndLayoutTweaker* layoutTweaker_;
  IBOutlet GTMWidthBasedTweaker* buttonContainer_;
}

- (IBAction)onAccept:(id)sender;
- (IBAction)onCancel:(id)sender;

- (void)close;

@end

// Helper to show a toolkit-views global error bubble. Implemented in a views-
// specific file.
GlobalErrorBubbleViewBase* ShowViewsGlobalErrorBubbleOnCocoaBrowser(
    NSPoint anchor,
    Browser* browser,
    const base::WeakPtr<GlobalErrorWithStandardBubble>& error);

#endif  // CHROME_BROWSER_UI_COCOA_GLOBAL_ERROR_BUBBLE_CONTROLLER_H_
