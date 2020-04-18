// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_RAPPOR_METRIC_H_
#define COMPONENTS_RAPPOR_RAPPOR_METRIC_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "components/rappor/bloom_filter.h"
#include "components/rappor/byte_vector_utils.h"
#include "components/rappor/public/rappor_parameters.h"

namespace rappor {

// A RapporMetric is an object that collects string samples into a Bloom filter,
// and generates randomized reports about the collected data.
//
// This class should not be used directly by metrics clients.  Record metrics
// using RapporServiceImpl::RecordSample or RapporService::RecordSample instead.
//
// For a full description of the rappor metrics, see
// http://www.chromium.org/developers/design-documents/rappor
class RapporMetric {
 public:
  // Takes the |metric_name| that this will be reported to the server with,
  // a |parameters| describing size and probability weights used in recording
  // this metric, and a |cohort| value, which determines the hash functions
  // used in the Bloom filter.
  RapporMetric(const std::string& metric_name,
               const RapporParameters& parameters,
               int32_t cohort);
  ~RapporMetric();

  // Records an additional sample in the Bloom filter.
  // A random sample will be used when reporting this metric when more than one
  // sample is collected in the same reporting interval.
  void AddSample(const std::string& str);

  // Retrieves the current Bloom filter bits.
  const ByteVector& bytes() const { return bloom_filter_.bytes(); }

  // Gets the parameter values this metric was constructed with.
  const RapporParameters& parameters() const { return parameters_; }

  // Generates the bits to report for this metric.  Using the secret as a seed,
  // randomly selects bits for redaction.  Then flips coins to generate the
  // final report bits.
  ByteVector GetReport(const std::string& secret) const;

 private:
  const std::string metric_name_;
  const RapporParameters parameters_;
  uint32_t sample_count_;
  BloomFilter bloom_filter_;

  DISALLOW_COPY_AND_ASSIGN(RapporMetric);
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_RAPPOR_METRIC_H_
