// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DEVICE_SHARING_DEVICE_SHARING_MANAGER_H_
#define IOS_CHROME_BROWSER_DEVICE_SHARING_DEVICE_SHARING_MANAGER_H_

#import <Foundation/Foundation.h>

class GURL;
@class HandoffManager;

namespace ios {
class ChromeBrowserState;
}

// This manager maintains all state related to sharing the active URL to other
// devices. It has the role of a dispatcher that shares the active URL to
// various internal sharing services (e.g. handoff).
@interface DeviceSharingManager : NSObject

// Updates the internal browser state to |browserState|.
// If the browser state is already |browserState|, then this is a no-op.
// Otherwise, this method cleans up the active URL and updates the internal
// state to reflect the new browser state.
//
// Note that this method keep a weak reference to |browserState|. It
// expects its owner to clear the browser state via a call to
// |-updateBrowserState:nullptr| before |browserState| is destroyed.
//
// |browserState| must not be off the record.
- (void)updateBrowserState:(ios::ChromeBrowserState*)browserState;

// Updates the active URL to be shared with other devices. This method is
// a no-op if the active browser state was never set previously.
- (void)updateActiveURL:(const GURL&)activeURL;

@end

@interface DeviceSharingManager (TestingOnly)

// Exposing Handoff feature for testing.
@property(nonatomic, readonly) HandoffManager* handoffManager;

@end

#endif  // IOS_CHROME_BROWSER_DEVICE_SHARING_DEVICE_SHARING_MANAGER_H_
