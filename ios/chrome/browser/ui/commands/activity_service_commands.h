// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_ACTIVITY_SERVICE_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_ACTIVITY_SERVICE_COMMANDS_H_

@protocol ActivityServiceCommands<NSObject>

// Shows the share sheet for the current page.
- (void)sharePage;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_ACTIVITY_SERVICE_COMMANDS_H_
