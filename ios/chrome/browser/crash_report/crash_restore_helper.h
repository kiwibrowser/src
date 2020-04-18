// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_CRASH_REPORT_CRASH_RESTORE_HELPER_H_
#define IOS_CHROME_BROWSER_CRASH_REPORT_CRASH_RESTORE_HELPER_H_

#import <Foundation/Foundation.h>

namespace ios {
class ChromeBrowserState;
}

@class TabModel;

// Helper class for handling session restoration after a crash.
@interface CrashRestoreHelper : NSObject

- (id)initWithBrowserState:(ios::ChromeBrowserState*)browserState;

// Saves the session information stored on disk in temporary files and will
// then delete those from their default location. This will ensure that the
// user will then start from scratch, while allowing restoring their old
// sessions.
- (void)moveAsideSessionInformation;

// Shows an infobar on the currently selected tab of the given |tabModel|. This
// infobar lets the user restore its session after a crash.
- (void)showRestoreIfNeeded:(TabModel*)tabModel;

@end

#endif  // IOS_CHROME_BROWSER_CRASH_REPORT_CRASH_RESTORE_HELPER_H_
