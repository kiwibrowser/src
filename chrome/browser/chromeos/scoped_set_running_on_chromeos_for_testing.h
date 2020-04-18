// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SCOPED_SET_RUNNING_ON_CHROMEOS_FOR_TESTING_H_
#define CHROME_BROWSER_CHROMEOS_SCOPED_SET_RUNNING_ON_CHROMEOS_FOR_TESTING_H_

#include <string>

#include "base/time/time.h"

namespace chromeos {

// unit_tests must always reset status of modified global singletons,
// otherwise later tests will fail, because they are run in the same process.
//
// This object resets LSB-Release to empty string on destruction, forcing
// is_running_on_chromeos() to return false.
class ScopedSetRunningOnChromeOSForTesting {
 public:
  ScopedSetRunningOnChromeOSForTesting(const std::string& lsb_release,
                                       const base::Time& lsb_release_time);
  ~ScopedSetRunningOnChromeOSForTesting();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedSetRunningOnChromeOSForTesting);
};

}  // namespace chromeos

#endif  //  CHROME_BROWSER_CHROMEOS_SCOPED_SET_RUNNING_ON_CHROMEOS_FOR_TESTING_H_
