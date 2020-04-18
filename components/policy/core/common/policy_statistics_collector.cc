// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/policy_statistics_collector.h"

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/task_runner.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/policy/core/common/policy_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace policy {

const int PolicyStatisticsCollector::kStatisticsUpdateRate =
    24 * 60 * 60 * 1000;  // 24 hours.

PolicyStatisticsCollector::PolicyStatisticsCollector(
    const GetChromePolicyDetailsCallback& get_details,
    const Schema& chrome_schema,
    PolicyService* policy_service,
    PrefService* prefs,
    const scoped_refptr<base::TaskRunner>& task_runner)
    : get_details_(get_details),
      chrome_schema_(chrome_schema),
      policy_service_(policy_service),
      prefs_(prefs),
      task_runner_(task_runner) {
}

PolicyStatisticsCollector::~PolicyStatisticsCollector() {
}

void PolicyStatisticsCollector::Initialize() {
  using base::Time;
  using base::TimeDelta;

  TimeDelta update_rate = TimeDelta::FromMilliseconds(kStatisticsUpdateRate);
  Time last_update = Time::FromInternalValue(
      prefs_->GetInt64(policy_prefs::kLastPolicyStatisticsUpdate));
  TimeDelta delay = std::max(Time::Now() - last_update, TimeDelta::FromDays(0));
  if (delay >= update_rate)
    CollectStatistics();
  else
    ScheduleUpdate(update_rate - delay);
}

// static
void PolicyStatisticsCollector::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterInt64Pref(policy_prefs::kLastPolicyStatisticsUpdate, 0);
}

void PolicyStatisticsCollector::RecordPolicyUse(int id) {
  base::UmaHistogramSparse("Enterprise.Policies", id);
}

void PolicyStatisticsCollector::CollectStatistics() {
  const PolicyMap& policies = policy_service_->GetPolicies(
      PolicyNamespace(POLICY_DOMAIN_CHROME, std::string()));

  // Collect statistics.
  for (Schema::Iterator it(chrome_schema_.GetPropertiesIterator());
       !it.IsAtEnd(); it.Advance()) {
    if (policies.Get(it.key())) {
      const PolicyDetails* details = get_details_.Run(it.key());
      if (details)
        RecordPolicyUse(details->id);
      else
        NOTREACHED();
    }
  }

  // Take care of next update.
  prefs_->SetInt64(policy_prefs::kLastPolicyStatisticsUpdate,
                   base::Time::Now().ToInternalValue());
  ScheduleUpdate(base::TimeDelta::FromMilliseconds(kStatisticsUpdateRate));
}

void PolicyStatisticsCollector::ScheduleUpdate(base::TimeDelta delay) {
  update_callback_.Reset(base::Bind(
      &PolicyStatisticsCollector::CollectStatistics,
      base::Unretained(this)));
  task_runner_->PostDelayedTask(FROM_HERE, update_callback_.callback(), delay);
}

}  // namespace policy
