// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DRAG_AND_DROP_DROP_AND_NAVIGATE_DELEGATE_H_
#define IOS_CHROME_BROWSER_DRAG_AND_DROP_DROP_AND_NAVIGATE_DELEGATE_H_

class GURL;

// Protocol to implement to be notified by a DropAndNavigationInteraction that
// a drop event that can trigger a navigation occurs.
@protocol DropAndNavigateDelegate

// Called when a drop event containing |url| occured.
- (void)URLWasDropped:(GURL const&)url;

@end

#endif  // IOS_CHROME_BROWSER_DRAG_AND_DROP_DROP_AND_NAVIGATE_DELEGATE_H_
