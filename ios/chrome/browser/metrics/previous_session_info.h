// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_METRICS_PREVIOUS_SESSION_INFO_H_
#define IOS_CHROME_BROWSER_METRICS_PREVIOUS_SESSION_INFO_H_

#import <Foundation/Foundation.h>

namespace previous_session_info_constants {
// Key in the UserDefaults for a boolean value keeping track of memory warnings.
extern NSString* const kDidSeeMemoryWarningShortlyBeforeTerminating;
}  // namespace previous_session_info_constants

// PreviousSessionInfo has two jobs:
// - Holding information about the last session, persisted across restart.
//   These informations are accessible via the properties on the shared
//   instance.
// - Persist information about the current session, for use in a next session.
@interface PreviousSessionInfo : NSObject

// Whether the app received a memory warning seconds before being terminated.
@property(nonatomic, assign, readonly)
    BOOL didSeeMemoryWarningShortlyBeforeTerminating;

// Whether the app was updated between the previous and the current session.
@property(nonatomic, assign, readonly) BOOL isFirstSessionAfterUpgrade;

// Whether the language has been changed between the previous and the current
// session.
@property(nonatomic, assign, readonly) BOOL isFirstSessionAfterLanguageChange;

// Singleton PreviousSessionInfo. During the lifetime of the app, the returned
// object is the same, and describes the previous session, even after a new
// session has started (by calling beginRecordingCurrentSession).
+ (instancetype)sharedInstance;

// Clears the persisted information about the previous session and starts
// persisting information about the current session, for use in a next session.
- (void)beginRecordingCurrentSession;

// When a session has begun, records that a memory warning was received.
- (void)setMemoryWarningFlag;

// When a session has begun, records that any memory warning flagged can be
// ignored.
- (void)resetMemoryWarningFlag;

@end

#endif  // IOS_CHROME_BROWSER_METRICS_PREVIOUS_SESSION_INFO_H_
