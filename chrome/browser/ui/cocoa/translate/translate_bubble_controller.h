// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TRANSLATE_TRANSLATE_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_TRANSLATE_TRANSLATE_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#import "chrome/browser/ui/cocoa/omnibox_decoration_bubble_controller.h"
#include "components/translate/core/common/translate_errors.h"

@class BrowserWindowController;

class TranslateBubbleModel;

namespace content {
class WebContents;
}

// Displays the Translate bubble. The Translate bubble is a bubble which
// pops up when clicking the Translate icon on Omnibox. This bubble
// allows us to translate a foreign page into user-selected language,
// revert this, and configure the translate setting.
@interface TranslateBubbleController
    : OmniboxDecorationBubbleController<NSTextViewDelegate> {
  // The views of each state. The keys are TranslateBubbleModel::ViewState,
  // and the values are NSView*.
  base::scoped_nsobject<NSDictionary<NSNumber*, NSView*>> views_;

  // The 'Try again' button on the error panel.
  NSButton* tryAgainButton_;
}

@property(readonly, nonatomic) const content::WebContents* webContents;
@property(readonly, nonatomic) const TranslateBubbleModel* model;

- (id)initWithParentWindow:(BrowserWindowController*)controller
                     model:(std::unique_ptr<TranslateBubbleModel>)model
               webContents:(content::WebContents*)webContents;
- (void)switchView:(TranslateBubbleModel::ViewState)viewState;
- (void)switchToErrorView:(translate::TranslateErrors::Type)errorType;

@end

// The methods on this category are used internally by the controller and are
// only exposed for testing purposes. DO NOT USE OTHERWISE.
@interface TranslateBubbleController (ExposedForTesting)
- (IBAction)handleCloseButtonPressed:(id)sender;
- (IBAction)handleTranslateButtonPressed:(id)sender;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TRANSLATE_TRANSLATE_BUBBLE_CONTROLLER_H_
