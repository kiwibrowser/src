// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_CACHE_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_CACHE_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/tabs/tab_model_observer.h"

@class Tab;

typedef void (^SnapshotCompletionBlock)(UIImage*);

struct PendingSnapshotRequest {
  CFTimeInterval requestId;
  NSUInteger sessionId;

  PendingSnapshotRequest() : requestId(0), sessionId(0) {}

  void clear() {
    requestId = 0;
    sessionId = 0;
  }
};

// A class that provides access to resized snapshots of tabs. The snapshots are
// cached and freed under memory pressure.
@interface TabSwitcherCache : NSObject<TabModelObserver>

@property(weak, nonatomic, readonly) TabModel* mainTabModel;

// Request a snapshot for the given |tab| of the given |size|, |completionBlock|
// will be called with the requested snapshot. Must be called from the main
// thread with a non nil |tab|, a non nil |completionBlock|, and a non
// CGSizeZero |size|.
// If the resized snapshot is cached, |completionBlock| is called synchronously.
// Otherwise it is called asynchronously on the main thread. The block's
// execution can be cancelled with |cancelPendingSnapshotRequest:|.
// Requesting a snapshot for a given tab will cancel any previous request for
// that same tab. The completion handler for a cancelled request may never be
// called.
- (PendingSnapshotRequest)requestSnapshotForTab:(Tab*)tab
                                       withSize:(CGSize)size
                                completionBlock:
                                    (SnapshotCompletionBlock)completionBlock;

// Updates the snapshot for the given |tab| with |image| resized to |size|.
- (void)updateSnapshotForTab:(Tab*)tab
                   withImage:(UIImage*)image
                        size:(CGSize)size;

// Cancels the request identified by |pendingRequest|. Does nothing if no
// request matches |pendingRequest|.
- (void)cancelPendingSnapshotRequest:(PendingSnapshotRequest)pendingRequest;

// Sets the main and incognito tab models.
- (void)setMainTabModel:(TabModel*)mainTabModel
            otrTabModel:(TabModel*)otrTabModel;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_CACHE_H_
