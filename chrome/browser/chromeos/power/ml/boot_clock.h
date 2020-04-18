// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time/time.h"

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_BOOT_CLOCK_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_BOOT_CLOCK_H_

namespace chromeos {
namespace power {
namespace ml {

// A class that returns time since boot. The time since boot always increases
// even when system is suspended (unlike TimeTicks).
class BootClock {
 public:
  virtual ~BootClock() {}

  virtual base::TimeDelta GetTimeSinceBoot() = 0;
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_BOOT_CLOCK_H_
