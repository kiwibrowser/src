// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/rappor_metric.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "components/rappor/public/rappor_parameters.h"
#include "components/rappor/reports.h"

namespace rappor {

RapporMetric::RapporMetric(const std::string& metric_name,
                           const RapporParameters& parameters,
                           int32_t cohort_seed)
    : metric_name_(metric_name),
      parameters_(parameters),
      sample_count_(0),
      bloom_filter_(parameters.bloom_filter_size_bytes,
                    parameters.bloom_filter_hash_function_count,
                    (cohort_seed % parameters.num_cohorts) *
                        parameters.bloom_filter_hash_function_count) {
  DCHECK_GE(cohort_seed, 0);
  DCHECK_LT(cohort_seed, RapporParameters::kMaxCohorts);
  // Since cohort_seed is in the range [0, kMaxCohorts), num_cohorts should
  // divide kMaxCohorts for each cohort to have equal weight.
  DCHECK_EQ(0, RapporParameters::kMaxCohorts % parameters.num_cohorts);
}

RapporMetric::~RapporMetric() {}

void RapporMetric::AddSample(const std::string& str) {
  ++sample_count_;
  // Replace the previous sample with a 1 in sample_count_ chance so that each
  // sample has equal probability of being reported.
  if (base::RandGenerator(sample_count_) == 0) {
    bloom_filter_.SetString(str);
  }
}

ByteVector RapporMetric::GetReport(const std::string& secret) const {
  return internal::GenerateReport(
      secret,
      internal::kNoiseParametersForLevel[parameters().noise_level],
      bytes());
}

}  // namespace rappor
