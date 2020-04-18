// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_METRICS_DRAG_AND_DROP_RECORDER_H_
#define IOS_CHROME_BROWSER_METRICS_DRAG_AND_DROP_RECORDER_H_

#import <UIKit/UIKit.h>

// Reports metrics for the drag and drop events on iOS 11+ for a given view.
// Should be attached to the top most UIWindow to not miss any event.
@interface DragAndDropRecorder : NSObject

// Default initializer. Does not retain |view|.
- (instancetype)initWithView:(UIView*)view NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_METRICS_DRAG_AND_DROP_RECORDER_H_
