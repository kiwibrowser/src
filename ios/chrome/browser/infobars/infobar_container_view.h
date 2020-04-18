// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_VIEW_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_VIEW_H_

#import <UIKit/UIKit.h>

class InfoBarIOS;

@interface InfoBarContainerView : UIView {
}

// Add a new infobar to the container view at position |position|.
- (void)addInfoBar:(InfoBarIOS*)infoBarIOS position:(NSInteger)position;

// Height of the frontmost infobar that is not hidden.
- (CGFloat)topmostVisibleInfoBarHeight;

@end

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_VIEW_H_
