// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_FAKE_BOOT_CLOCK_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_FAKE_BOOT_CLOCK_H_

#include "chrome/browser/chromeos/power/ml/boot_clock.h"

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/test/test_mock_time_task_runner.h"

namespace chromeos {
namespace power {
namespace ml {

class FakeBootClock : public BootClock {
 public:
  FakeBootClock(scoped_refptr<const base::TestMockTimeTaskRunner> task_runner,
                base::TimeDelta initial_time_since_boot);
  ~FakeBootClock() override;

  // BootClock:
  base::TimeDelta GetTimeSinceBoot() override;

 private:
  scoped_refptr<const base::TestMockTimeTaskRunner> task_runner_;
  base::TimeDelta initial_time_since_boot_;
  base::TimeTicks initial_time_ticks_;

  DISALLOW_COPY_AND_ASSIGN(FakeBootClock);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_FAKE_BOOT_CLOCK_H_
