// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_INTERNAL_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_INTERNAL_H_

#import "ios/chrome/browser/snapshots/snapshot_cache_internal.h"

namespace base {
class FilePath;
}

@class NSString;

@interface SnapshotCache (Internal)
// Returns filepath to the color snapshot of |sessionID|.
- (base::FilePath)imagePathForSessionID:(NSString*)sessionID;
// Returns filepath to the greyscale snapshot of |sessionID|.
- (base::FilePath)greyImagePathForSessionID:(NSString*)sessionID;
@end

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_CACHE_INTERNAL_H_
