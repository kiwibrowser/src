// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/testing/perf/startupLoggers.h"
#include "base/time/time.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace startup_loggers {
// Stores the time of app startup states.
base::Time* g_start_time;
base::Time* g_finish_launching_time;
base::Time* g_become_active_time;

void RegisterAppStartTime() {
  DCHECK(!g_start_time);
  g_start_time = new base::Time(base::Time::Now());
}

void RegisterAppDidFinishLaunchingTime() {
  g_finish_launching_time = new base::Time(base::Time::Now());
}

void RegisterAppDidBecomeActiveTime() {
  g_become_active_time = new base::Time(base::Time::Now());
}

bool LogData(NSString* testName) {
  // Store the data into a format compatible with infra scripts.
  double finishLaunchingDuration =
      g_finish_launching_time->ToDoubleT() - g_start_time->ToDoubleT();
  double becomeActiveDuration =
      g_become_active_time->ToDoubleT() - g_start_time->ToDoubleT();
  NSDictionary* values = @{
    @"AppDidFinishLaunchingTime" : @(finishLaunchingDuration),
    @"AppDidBecomeActiveTime" : @(becomeActiveDuration)
  };
  NSDictionary* timingData =
      @{ testName : @{@"unit" : @"seconds", @"value" : values} };
  NSDictionary* summary = @{@"Perf Data" : timingData};

  // Converts data into json format.
  NSError* error;
  NSData* jsonData =
      [NSJSONSerialization dataWithJSONObject:summary
                                      options:NSJSONWritingPrettyPrinted
                                        error:&error];
  if (error) {
    return false;
  }

  // Stores data into a json file under the app's document directory.
  NSString* fileName = @"perf_result.json";
  NSArray<NSString*>* outputDirectories = NSSearchPathForDirectoriesInDomains(
      NSDocumentDirectory, NSUserDomainMask, YES);
  if ([outputDirectories count] == 0) {
    return false;
  }
  NSString* outputPath =
      [outputDirectories[0] stringByAppendingPathComponent:fileName];
  return [jsonData writeToFile:outputPath atomically:YES];
}
}
