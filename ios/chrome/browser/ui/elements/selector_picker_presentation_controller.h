// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_PICKER_PRESENTATION_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_PICKER_PRESENTATION_CONTROLLER_H_

#import <UIKit/UIKit.h>

// Presentation controller for the selector picker. Positions the presented view
// at the bottom of the screen, using the view's system layout fitting size to
// determine its height. Also adds a dimming view behind the presented view.
@interface SelectorPickerPresentationController : UIPresentationController
@end

#endif  // IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_PICKER_PRESENTATION_CONTROLLER_H_
