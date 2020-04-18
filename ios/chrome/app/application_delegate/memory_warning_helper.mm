// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/app/application_delegate/memory_warning_helper.h"

#include "base/memory/memory_pressure_listener.h"
#include "base/metrics/histogram_macros.h"
#include "ios/chrome/browser/crash_report/breakpad_helper.h"
#import "ios/chrome/browser/metrics/previous_session_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The number of seconds to wait after a memory warning to clear the flag used
// to detect Out Of Memory crashes.
// NOTE: From local tests on various devices, this interval ranges between 1 and
// 3 seconds. It is set to 5 to ensure all out of memory crashes are identified,
// even if this may lead to overcounting them.
const CFTimeInterval kOutOfMemoryResetTimeInterval = 5;
}

@interface MemoryWarningHelper () {
  // The time at which to reset the OOM crash flag in the user defaults. This
  // is used to handle receiving multiple memory warnings in short
  // succession.
  CFAbsoluteTime _outOfMemoryResetTime;
}
// Resets the OOM crash flag from the user defaults.
- (void)resetOutOfMemoryFlagIfNecessary;
@end

@implementation MemoryWarningHelper

@synthesize foregroundMemoryWarningCount = _foregroundMemoryWarningCount;

- (void)handleMemoryPressure {
  // Notify the system that the memory is critical and something should be done.
  base::MemoryPressureListener::NotifyMemoryPressure(
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);

  ++_foregroundMemoryWarningCount;
  // Register that we might die because of memory. If we are still alive in
  // |kOutOfMemoryResetTimeInterval| seconds, reset the flag.
  [[PreviousSessionInfo sharedInstance] setMemoryWarningFlag];
  _outOfMemoryResetTime =
      CFAbsoluteTimeGetCurrent() + kOutOfMemoryResetTimeInterval;

  // Add information to breakpad.
  breakpad_helper::SetMemoryWarningCount(_foregroundMemoryWarningCount);
  breakpad_helper::SetMemoryWarningInProgress(true);

  dispatch_after(kOutOfMemoryResetTimeInterval, dispatch_get_main_queue(), ^{
    [self resetOutOfMemoryFlagIfNecessary];
  });
}

- (void)resetOutOfMemoryFlagIfNecessary {
  if (CFAbsoluteTimeGetCurrent() < _outOfMemoryResetTime)
    return;
  _outOfMemoryResetTime = 0;
  [[PreviousSessionInfo sharedInstance] resetMemoryWarningFlag];
  breakpad_helper::SetMemoryWarningInProgress(false);
}

- (void)resetForegroundMemoryWarningCount {
  _foregroundMemoryWarningCount = 0;
  breakpad_helper::SetMemoryWarningCount(0);
}

@end
