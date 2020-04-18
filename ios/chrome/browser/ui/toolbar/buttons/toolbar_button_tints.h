// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLBAR_BUTTON_TINTS_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLBAR_BUTTON_TINTS_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"

// TODO(crbug.com/800266): Remove this file.
namespace toolbar {

// Returns the tint color for a toolbar button in the normal state.
// |style| describes the style of the toolbar the button belongs to.
UIColor* NormalButtonTint(ToolbarControllerStyle style);

// Returns the tint color for a toolbar button in the highlighted state.
// |style| describes the style of the toolbar the button belongs to.
UIColor* HighlighButtonTint(ToolbarControllerStyle style);

}  // namespace toolbar

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLBAR_BUTTON_TINTS_H_
