// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_REAL_BOOT_CLOCK_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_REAL_BOOT_CLOCK_H_

#include "chrome/browser/chromeos/power/ml/boot_clock.h"

namespace chromeos {
namespace power {
namespace ml {

class RealBootClock : public BootClock {
 public:
  RealBootClock();
  ~RealBootClock() override;

  // BootClock:
  base::TimeDelta GetTimeSinceBoot() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RealBootClock);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_REAL_BOOT_CLOCK_H_
