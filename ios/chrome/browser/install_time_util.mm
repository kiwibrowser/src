// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/install_time_util.h"

#import <Foundation/Foundation.h>
#include <stdint.h>

#include "base/mac/foundation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* const kInstallationTimeKey = @"omaha.InstallationTime";

// Returns the installation time of the application: this is the time the
// application was first installed, not the time the last version was installed.
// If the installation time is unknown, returns a base::Time corresponding to
// |kUnknownInstallDate|.
// To be noted: this value is reset if the application is uninstalled.
base::Time GetNSUserDefaultsInstallationTime() {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  CFAbsoluteTime time = [defaults doubleForKey:kInstallationTimeKey];
  if (time == 0.0)
    return base::Time();
  return base::Time::FromCFAbsoluteTime(time);
}

}  // namespace

namespace install_time_util {

// 2 is used because 0 is a magic value for Time, and 1 was the pre-M29 value
// which was migrated to a specific date (crbug.com/270124).
const int64_t kUnknownInstallDate = 2;

base::Time ComputeInstallationTime(bool is_first_run) {
  return ComputeInstallationTimeInternal(is_first_run,
                                         GetNSUserDefaultsInstallationTime());
}

base::Time ComputeInstallationTimeInternal(
    bool is_first_run,
    base::Time ns_user_defaults_install_time) {
  if (is_first_run)
    return base::Time::Now();

  if (ns_user_defaults_install_time.is_null())
    return base::Time::FromTimeT(kUnknownInstallDate);

  return ns_user_defaults_install_time;
}

}  // namespace install_time_util
