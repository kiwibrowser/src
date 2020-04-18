// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_SAMPLER_H_
#define COMPONENTS_RAPPOR_SAMPLER_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "components/rappor/public/rappor_parameters.h"
#include "components/rappor/public/sample.h"

namespace rappor {

class RapporReports;

namespace internal {

// Sampler manages the collection and storage of Sample objects.
// For each metric name, it will randomly select one Sample to store and
// use when generating RapporReports.
class Sampler {
 public:
  Sampler();
  ~Sampler();

  // Store this sample for metric name, randomly selecting a sample if
  // others have already been recorded.
  void AddSample(const std::string& metric_name,
                 std::unique_ptr<Sample> sample);

  // Generate randomized reports for all stored samples and store them
  // in |reports|, then discard the samples.
  void ExportMetrics(const std::string& secret, RapporReports* reports);

 private:
  // The number of samples recorded for each metric since the last export.
  std::map<std::string, int> sample_counts_;

  // Stores a Sample for each metric, by metric name.
  std::unordered_map<std::string, std::unique_ptr<Sample>> samples_;

  DISALLOW_COPY_AND_ASSIGN(Sampler);
};

}  // namespace internal

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_SAMPLER_H_
