// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_STATE_DELEGATE_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_STATE_DELEGATE_H_

#import <Foundation/Foundation.h>

// Delegate protocol for an object that is informed of events relating to the
// state of the infobar container.
@protocol InfobarContainerStateDelegate

// Tells the delegate that the infobar container's state changed. If |animated|
// is YES, the state change happened as part of an animation.
- (void)infoBarContainerStateDidChangeAnimated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTAINER_STATE_DELEGATE_H_
