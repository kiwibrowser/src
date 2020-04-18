// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_SNAPSHOT_UTIL_H_
#define IOS_CHROME_BROWSER_UI_UTIL_SNAPSHOT_UTIL_H_

#import <UIKit/UIKit.h>

namespace snapshot_util {

// Returns an autoreleased UIView containing a snapshot of |view|.
UIView* GenerateSnapshot(UIView* view);

}  // namespace snapshot_util

#endif  // IOS_CHROME_BROWSER_UI_UTIL_SNAPSHOT_UTIL_H_
