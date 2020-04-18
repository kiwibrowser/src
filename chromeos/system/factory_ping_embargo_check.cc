// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/system/factory_ping_embargo_check.h"

#include <string>

#include "base/metrics/histogram_macros.h"
#include "chromeos/system/statistics_provider.h"

namespace chromeos {
namespace system {

namespace {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
// These values must match the corresponding enum defined in enums.xml.
enum class EndDateValidityHistogramValue {
  kMalformed = 0,
  kInvalid = 1,
  kValid = 2,
  kMaxValue = kValid,
};

// This is in a helper function to help the compiler avoid generating duplicated
// code.
void RecordEndDateValidity(EndDateValidityHistogramValue value) {
  UMA_HISTOGRAM_ENUMERATION("FactoryPingEmbargo.EndDateValidity", value);
}

}  // namespace

FactoryPingEmbargoState GetFactoryPingEmbargoState(
    StatisticsProvider* statistics_provider) {
  std::string rlz_embargo_end_date;
  if (!statistics_provider->GetMachineStatistic(kRlzEmbargoEndDateKey,
                                                &rlz_embargo_end_date)) {
    // |rlz_embargo_end_date| only exists on new devices that have not yet
    // launched.
    return FactoryPingEmbargoState::kMissingOrMalformed;
  }
  base::Time parsed_time;
  if (!base::Time::FromUTCString(rlz_embargo_end_date.c_str(), &parsed_time)) {
    LOG(ERROR) << "|rlz_embargo_end_date| exists but cannot be parsed.";
    RecordEndDateValidity(EndDateValidityHistogramValue::kMalformed);
    return FactoryPingEmbargoState::kMissingOrMalformed;
  }

  if (parsed_time - base::Time::Now() >=
      kRlzEmbargoEndDateGarbageDateThreshold) {
    // If |rlz_embargo_end_date| is more than this many days in the future,
    // ignore it. Because it indicates that the device is not connected to an
    // ntp server in the factory, and its internal clock could be off when the
    // date is written.
    RecordEndDateValidity(EndDateValidityHistogramValue::kInvalid);
    return FactoryPingEmbargoState::kInvalid;
  }

  RecordEndDateValidity(EndDateValidityHistogramValue::kValid);
  return base::Time::Now() > parsed_time ? FactoryPingEmbargoState::kPassed
                                         : FactoryPingEmbargoState::kNotPassed;
}

}  // namespace system
}  // namespace chromeos
