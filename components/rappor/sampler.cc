// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/sampler.h"

#include <map>
#include <string>
#include <utility>

#include "base/rand_util.h"

namespace rappor {

namespace internal {

Sampler::Sampler() {}

Sampler::~Sampler() {}

void Sampler::AddSample(const std::string& metric_name,
                        std::unique_ptr<Sample> sample) {
  ++sample_counts_[metric_name];
  // Replace the previous sample with a 1 in sample_count_ chance so that each
  // sample has equal probability of being reported.
  if (base::RandGenerator(sample_counts_[metric_name]) == 0)
    samples_[metric_name] = std::move(sample);
}

void Sampler::ExportMetrics(const std::string& secret, RapporReports* reports) {
  for (const auto& kv : samples_) {
    kv.second->ExportMetrics(secret, kv.first, reports);
  }
  samples_.clear();
  sample_counts_.clear();
}

}  // namespace internal

}  // namespace rappor
