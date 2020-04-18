// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_UKM_LOGGER_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_UKM_LOGGER_IMPL_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/power/ml/user_activity_ukm_logger.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace chromeos {
namespace power {
namespace ml {

class UserActivityEvent;

class UserActivityUkmLoggerImpl : public UserActivityUkmLogger {
 public:
  // Both |boundary_end| and |rounding| must be positive.
  struct Bucket {
    int boundary_end;
    int rounding;
  };

  // Bucketize |original_value| using given |buckets|, which is an array of
  // Bucket and must be sorted in ascending order of |boundary_end|.
  // |original_value| must be non-negative. An example of |buckets| is
  // {{60, 1}, {300, 10}, {600, 20}}. This function looks for the first
  // |boundary_end| > |original_value| and bucket it to the nearest |rounding|.
  // If |original_value| is greater than all |boundary_end|, the function
  // returns the largest |boundary_end|. Using the above |buckets| example, the
  // function will return 30 if |original_value| = 30, and 290 if
  // |original_value| = 299.
  static int Bucketize(int original_value,
                       const Bucket* buckets,
                       size_t num_buckets);

  UserActivityUkmLoggerImpl();
  ~UserActivityUkmLoggerImpl() override;

  // chromeos::power::ml::UserActivityUkmLogger overrides:
  void LogActivity(
      const UserActivityEvent& event,
      const std::map<ukm::SourceId, TabProperty>& open_tabs) override;

 private:
  friend class UserActivityUkmLoggerTest;

  ukm::UkmRecorder* ukm_recorder_;  // not owned

  // This ID is incremented each time a UserActivity is logged to UKM.
  // Event index starts from 1, and resets when a new session starts.
  int next_sequence_id_ = 1;

  DISALLOW_COPY_AND_ASSIGN(UserActivityUkmLoggerImpl);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_UKM_LOGGER_IMPL_H_
