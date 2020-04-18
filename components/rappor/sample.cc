// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/public/sample.h"

#include <map>
#include <string>

#include "base/logging.h"
#include "base/metrics/metrics_hashes.h"
#include "components/rappor/bloom_filter.h"
#include "components/rappor/byte_vector_utils.h"
#include "components/rappor/proto/rappor_metric.pb.h"
#include "components/rappor/reports.h"

namespace rappor {

Sample::Sample(int32_t cohort_seed, const RapporParameters& parameters)
  : parameters_(parameters),
    bloom_offset_((cohort_seed % parameters_.num_cohorts) *
          parameters_.bloom_filter_hash_function_count) {
  // Must use bloom filter size that fits in uint64_t.
  DCHECK_LE(parameters_.bloom_filter_size_bytes, 8u);
}

Sample::~Sample() {
}

void Sample::SetStringField(const std::string& field_name,
                            const std::string& value) {
  DVLOG(2) << "Recording sample \"" << value
           << "\" for sample metric field \"" << field_name << "\"";
  DCHECK_EQ(0u, field_info_[field_name].size);
  uint64_t bloom_bits = internal::GetBloomBits(
      parameters_.bloom_filter_size_bytes,
      parameters_.bloom_filter_hash_function_count,
      bloom_offset_,
      value);
  field_info_[field_name] = Sample::FieldInfo{
    parameters_.bloom_filter_size_bytes /* size */,
    bloom_bits /* value */,
    parameters_.noise_level,
  };
}

void Sample::SetFlagsField(const std::string& field_name,
                           uint64_t flags,
                           size_t num_flags) {
  SetFlagsField(field_name, flags, num_flags, parameters_.noise_level);
}

void Sample::SetFlagsField(const std::string& field_name,
                           uint64_t flags,
                           size_t num_flags,
                           NoiseLevel noise_level) {
  if (noise_level == NO_NOISE) {
    // Non-noised fields can only be recorded for UMA rappor metrics.
    DCHECK_EQ(UMA_RAPPOR_GROUP, parameters_.recording_group);
    if (parameters_.recording_group != UMA_RAPPOR_GROUP)
      return;
  }
  DVLOG(2) << "Recording flags " << flags
           << " for sample metric field \"" << field_name << "\"";
  DCHECK_EQ(0u, field_info_[field_name].size);
  DCHECK_GT(num_flags, 0u);
  DCHECK_LE(num_flags, 64u);
  DCHECK(num_flags == 64u || flags >> num_flags == 0);
  field_info_[field_name] = Sample::FieldInfo{
    (num_flags + 7) / 8 /* size */,
    flags /* value */,
    noise_level,
  };
}

void Sample::SetUInt64Field(const std::string& field_name,
                            uint64_t value,
                            NoiseLevel noise_level) {
  // Noised integers not supported yet.
  DCHECK_EQ(NO_NOISE, noise_level);
  // Non-noised fields can only be recorded for UMA rappor metrics.
  DCHECK_EQ(UMA_RAPPOR_GROUP, parameters_.recording_group);
  if (parameters_.recording_group != UMA_RAPPOR_GROUP)
    return;
  DCHECK_EQ(0u, field_info_[field_name].size);
  field_info_[field_name] = Sample::FieldInfo{
    8,
    value,
    noise_level,
  };
}

void Sample::ExportMetrics(const std::string& secret,
                           const std::string& metric_name,
                           RapporReports* reports) const {
  for (const auto& kv : field_info_) {
    FieldInfo field_info = kv.second;
    ByteVector value_bytes(field_info.size);
    Uint64ToByteVector(field_info.value, field_info.size, &value_bytes);
    ByteVector report_bytes = internal::GenerateReport(
        secret,
        internal::kNoiseParametersForLevel[field_info.noise_level],
        value_bytes);

    RapporReports::Report* report = reports->add_report();
    report->set_name_hash(base::HashMetricName(
        metric_name + "." + kv.first));
    report->set_bits(std::string(report_bytes.begin(), report_bytes.end()));
    DVLOG(2) << "Exporting sample " << metric_name << "." << kv.first;
  }
}

}  // namespace rappor
