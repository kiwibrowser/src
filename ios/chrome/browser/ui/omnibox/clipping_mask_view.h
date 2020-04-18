// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_MASK_VIEW_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_MASK_VIEW_H_

#import <UIKit/UIKit.h>

// The view used as a mask when the omnibox textfield is being clipped.
// Displays an opaque rectangle with left, right, or both edges being
// faded out with transparent gradient.
@interface ClippingMaskView : UIView

// Whether the left edge is faded.
@property(nonatomic, assign) BOOL fadeLeft;
// Whether the right edge is faded.
@property(nonatomic, assign) BOOL fadeRight;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_CLIPPING_MASK_VIEW_H_
