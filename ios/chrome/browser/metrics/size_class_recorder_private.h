// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_METRICS_SIZE_CLASS_RECORDER_PRIVATE_H_
#define IOS_CHROME_BROWSER_METRICS_SIZE_CLASS_RECORDER_PRIVATE_H_

// Reported size classes.
enum class SizeClassForReporting {
  UNSPECIFIED = 0,
  COMPACT,
  REGULAR,
  // NOTE: add new size classes above this line.
  COUNT
};

// Converts a UIKit size class to a size class for reporting.
SizeClassForReporting SizeClassForReportingForUIUserInterfaceSizeClass(
    UIUserInterfaceSizeClass sizeClass);

#endif  // IOS_CHROME_BROWSER_METRICS_SIZE_CLASS_RECORDER_PRIVATE_H_
