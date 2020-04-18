// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_CALL_STACK_PROFILE_COLLECTOR_H_
#define COMPONENTS_METRICS_CALL_STACK_PROFILE_COLLECTOR_H_

#include "base/macros.h"
#include "components/metrics/public/interfaces/call_stack_profile_collector.mojom.h"

namespace metrics {

class CallStackProfileCollector : public mojom::CallStackProfileCollector {
 public:
  using CallStackProfile = base::StackSamplingProfiler::CallStackProfile;

  explicit CallStackProfileCollector(
      CallStackProfileParams::Process expected_process);
  ~CallStackProfileCollector() override;

  // Create a collector to receive profiles from |expected_process|.
  static void Create(CallStackProfileParams::Process expected_process,
                     mojom::CallStackProfileCollectorRequest request);

  // mojom::CallStackProfileCollector:
  void Collect(const CallStackProfileParams& params,
               base::TimeTicks start_timestamp,
               std::vector<CallStackProfile> profiles) override;

 private:
  // Profile params are validated to come from this process. Profiles with a
  // different process declared in the params are considered untrustworthy and
  // ignored.
  const CallStackProfileParams::Process expected_process_;

  DISALLOW_COPY_AND_ASSIGN(CallStackProfileCollector);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_CALL_STACK_PROFILE_COLLECTOR_H_
