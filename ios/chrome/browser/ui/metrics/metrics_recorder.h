// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_METRICS_METRICS_RECORDER_H_
#define IOS_CHROME_BROWSER_UI_METRICS_METRICS_RECORDER_H_

#import <Foundation/Foundation.h>

// MetricsRecorder defines a common interface for objects that record metrics
// for a given NSInvocation object.
@protocol MetricsRecorder<NSObject>

// Records metrics for |anInvocation|.
- (void)recordMetricForInvocation:(NSInvocation*)anInvocation;

@end

#endif  // IOS_CHROME_BROWSER_UI_METRICS_METRICS_RECORDER_H_
