// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_POSITIONER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_POSITIONER_H_

#import <UIKit/UIKit.h>

// TODO(crbug.com/809785): Rename this or merge it into another protocol.
@protocol OmniboxPopupPositioner

// View to which the popup view should be added as subview.
- (UIView*)popupParentView;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_POSITIONER_H_
