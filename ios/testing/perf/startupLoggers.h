// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

namespace startup_loggers {

// Registers the current time variable. This function should be called when the
// app is launched.
void RegisterAppStartTime();
// Registers the current time variable. This function should be called when the
// app gets didFinishLaunchingWithOptions notification.
void RegisterAppDidFinishLaunchingTime();
// Registers the current time variable. Chrome does some lauch option steps
// after the app gets didFinishLaunchingWithOptions notification. This function
// should be called when the app gets applicationDidBecomeActive notification.
void RegisterAppDidBecomeActiveTime();
// Returns whether data is successfully stored in the output json file.  This
// function stores the time variables into a json file.
bool LogData(NSString* testName);

}  // namespace startup_loggers
