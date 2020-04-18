// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/entry.h"

#include "base/trace_event/memory_usage_estimator.h"

namespace download {

Entry::Entry()
    : bytes_downloaded(0u),
      attempt_count(0),
      resumption_count(0),
      cleanup_attempt_count(0),
      has_upload_data(false) {}
Entry::Entry(const Entry& other) = default;

Entry::Entry(const DownloadParams& params)
    : client(params.client),
      guid(params.guid),
      create_time(base::Time::Now()),
      scheduling_params(params.scheduling_params),
      request_params(params.request_params),
      bytes_downloaded(0u),
      attempt_count(0),
      resumption_count(0),
      cleanup_attempt_count(0),
      has_upload_data(false),
      traffic_annotation(params.traffic_annotation) {}

Entry::~Entry() = default;

bool Entry::operator==(const Entry& other) const {
  return client == other.client && guid == other.guid &&
         scheduling_params.cancel_time == other.scheduling_params.cancel_time &&
         scheduling_params.network_requirements ==
             other.scheduling_params.network_requirements &&
         scheduling_params.battery_requirements ==
             other.scheduling_params.battery_requirements &&
         scheduling_params.priority == other.scheduling_params.priority &&
         request_params.url == other.request_params.url &&
         request_params.method == other.request_params.method &&
         request_params.request_headers.ToString() ==
             other.request_params.request_headers.ToString() &&
         state == other.state && target_file_path == other.target_file_path &&
         create_time == other.create_time &&
         completion_time == other.completion_time &&
         last_cleanup_check_time == other.last_cleanup_check_time &&
         bytes_downloaded == other.bytes_downloaded &&
         attempt_count == other.attempt_count &&
         resumption_count == other.resumption_count &&
         cleanup_attempt_count == other.cleanup_attempt_count &&
         has_upload_data == other.has_upload_data &&
         traffic_annotation == other.traffic_annotation;
}

size_t Entry::EstimateMemoryUsage() const {
  // Ignore size of small primary types and objects.
  return base::trace_event::EstimateMemoryUsage(guid) +
         base::trace_event::EstimateMemoryUsage(request_params.url) +
         base::trace_event::EstimateMemoryUsage(request_params.method) +
         base::trace_event::EstimateMemoryUsage(
             request_params.request_headers.ToString()) +
         base::trace_event::EstimateMemoryUsage(target_file_path.value());
}

}  // namespace download
