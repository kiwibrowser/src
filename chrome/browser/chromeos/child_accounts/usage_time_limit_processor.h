// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Processor for the UsageTimeLimit policy. Used to determine the current state
// of the client, for example if it is locked and the reason why it may be
// locked.

#ifndef CHROME_BROWSER_CHROMEOS_CHILD_ACCOUNTS_USAGE_TIME_LIMIT_PROCESSOR_H_
#define CHROME_BROWSER_CHROMEOS_CHILD_ACCOUNTS_USAGE_TIME_LIMIT_PROCESSOR_H_

#include <vector>

#include "base/optional.h"
#include "base/time/time.h"
#include "base/values.h"

namespace chromeos {
namespace usage_time_limit {

enum class ActivePolicies {
  kNoActivePolicy,
  kOverride,
  kFixedLimit,
  kUsageLimit
};

struct State {
  // Whether the device is currently locked.
  bool is_locked;

  // Which policy is responsible for the current state.
  // If it is locked, one of [ override, fixed_limit, usage_limit ]
  // If it is not locked, one of [ no_active_policy, override ]
  ActivePolicies active_policy;

  // Whether time_usage_limit is currently active.
  bool is_time_usage_limit_enabled;

  // Remaining screen usage quota. Only available if
  // is_time_limit_enabled = true
  base::TimeDelta remaining_usage;

  // Next epoch time that time limit state could change. This could be the
  // start time of the next fixed window limit, the end time of the current
  // fixed limit, the earliest time a usage limit could be reached, or the
  // next time when screen time will start.
  base::Time next_state_change_time;

  // The policy that will be active in the next state.
  ActivePolicies next_state_active_policy;

  // Last time the state changed.
  base::Time last_state_changed;
};

// Returns the current state of the user session with the given usage time limit
// policy.
State GetState(const std::unique_ptr<base::DictionaryValue>& time_limit,
               const base::TimeDelta& used_time,
               const base::Time& usage_timestamp,
               const base::Time& current_time,
               const base::Optional<State>& previous_state);

// Ruturns the expected time that the used time stored should be reseted.
base::Time GetExpectedResetTime(
    const std::unique_ptr<base::DictionaryValue>& time_limit,
    base::Time current_time);

}  // namespace usage_time_limit
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CHILD_ACCOUNTS_USAGE_TIME_LIMIT_PROCESSOR_H_