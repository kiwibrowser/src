// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_PUBLIC_SAMPLE_H_
#define COMPONENTS_RAPPOR_PUBLIC_SAMPLE_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <string>

#include "base/macros.h"
#include "components/rappor/public/rappor_parameters.h"

namespace rappor {

class RapporReports;
class RapporServiceImpl;
class TestSamplerFactory;

// Sample is a container for information about a single instance of some event
// we are sending Rappor data about.  It may contain multiple different fields,
// which describe different details of the event, and they will be sent in the
// same Rappor report, enabling analysis of correlations between those fields.
class Sample {
 public:
  virtual ~Sample();

  // Sets a string value field in this sample.
  virtual void SetStringField(const std::string& field_name,
                              const std::string& value);

  // TODO(holte): Move all callers to the version with NoiseLevel.
  virtual void SetFlagsField(const std::string& field_name,
                             uint64_t flags,
                             size_t num_flags);

  // Sets a group of boolean flags as a field in this sample, with the
  // specified noise level.
  // |flags| should be a set of boolean flags stored in the lowest |num_flags|
  // bits of |flags|.
  virtual void SetFlagsField(const std::string& field_name,
                             uint64_t flags,
                             size_t num_flags,
                             NoiseLevel noise_level);

  // Sets an integer value field in this sample, at the given noise level.
  virtual void SetUInt64Field(const std::string& field_name,
                              uint64_t value,
                              NoiseLevel noise_level);

  // Generate randomized reports and store them in |reports|.
  virtual void ExportMetrics(const std::string& secret,
                             const std::string& metric_name,
                             RapporReports* reports) const;

  const RapporParameters& parameters() { return parameters_; }

 private:
  friend class TestSamplerFactory;
  friend class RapporServiceImpl;
  friend class TestSample;

  // Constructs a sample.  Instead of calling this directly, call
  // RapporService::MakeSampleObj to create a sample.
  Sample(int32_t cohort_seed, const RapporParameters& parameters);

  const RapporParameters parameters_;

  // Offset used for bloom filter hash functions.
  uint32_t bloom_offset_;

  struct FieldInfo {
    // Size of the field, in bytes.
    size_t size;
    // The non-randomized report value for the field.
    uint64_t value;
    // The noise level to use when creating a report for the field.
    NoiseLevel noise_level;
  };

  // Information about all recorded fields.
  std::map<std::string, FieldInfo> field_info_;

  DISALLOW_COPY_AND_ASSIGN(Sample);
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_PUBLIC_SAMPLE_H_
