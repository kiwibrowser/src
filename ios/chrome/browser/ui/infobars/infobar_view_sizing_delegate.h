// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_INFOBARS_INFOBAR_VIEW_SIZING_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_INFOBARS_INFOBAR_VIEW_SIZING_DELEGATE_H_

#import <Foundation/Foundation.h>

// A protocol implemented by a delegate of InfoBarViewSizing.
@protocol InfoBarViewSizingDelegate

// Notifies that the target size has been changed (e.g. after rotation).
- (void)didSetInfoBarTargetHeight:(CGFloat)height;

@end

#endif  // IOS_CHROME_BROWSER_UI_INFOBARS_INFOBAR_VIEW_SIZING_DELEGATE_H_
