// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INSTALL_TIME_UTIL_H_
#define IOS_CHROME_BROWSER_INSTALL_TIME_UTIL_H_

#include <stdint.h>

#include "base/time/time.h"

namespace install_time_util {

extern const int64_t kUnknownInstallDate;

// Computes the true installation time of the application based on the current
// install time stored in NSUserDefaults and whether or not this is a first run
// launch.  This function will return a base::Time corresponding to
// |kUnknownInstallDate| if the true installation time is unknown.
base::Time ComputeInstallationTime(bool is_first_run);

// Internal implementation of |ComputeInstallationTime()|.  Exposed only for
// testing.
base::Time ComputeInstallationTimeInternal(
    bool is_first_run,
    base::Time ns_user_defaults_install_time);

}  // namespace install_time_util

#endif  // IOS_CHROME_BROWSER_INSTALL_TIME_UTIL_H_
