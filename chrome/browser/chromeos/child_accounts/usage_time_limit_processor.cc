// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usage_time_limit_processor.h"

namespace chromeos {
namespace usage_time_limit {

State GetState(const std::unique_ptr<base::DictionaryValue>& time_limit,
               const base::TimeDelta& used_time,
               const base::Time& usage_timestamp,
               const base::Time& current_time,
               const base::Optional<State>& previous_state) {
  // TODO: Implement method.
  return State();
}

base::Time GetExpectedResetTime(
    const std::unique_ptr<base::DictionaryValue>& time_limit,
    base::Time current_time) {
  // TODO: Implement this method.
  return base::Time();
}

}  // namespace usage_time_limit
}  // namespace chromeos