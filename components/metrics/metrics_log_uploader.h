// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_METRICS_LOG_UPLOADER_H_
#define COMPONENTS_METRICS_METRICS_LOG_UPLOADER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"

namespace metrics {

class ReportingInfo;

// MetricsLogUploader is an abstract base class for uploading UMA logs on behalf
// of MetricsService.
class MetricsLogUploader {
 public:
  // Type for OnUploadComplete callbacks.  These callbacks will receive three
  // parameters: A response code, a net error code, and a boolean specifying
  // if the connection was secure (over HTTPS).
  typedef base::Callback<void(int, int, bool)> UploadCallback;

  // Possible service types. This should correspond to a type from
  // DataUseUserData.
  enum MetricServiceType {
    UMA,
    UKM,
  };

  virtual ~MetricsLogUploader() {}

  // Uploads a log with the specified |compressed_log_data| and |log_hash|.
  // |log_hash| is expected to be the hex-encoded SHA1 hash of the log data
  // before compression.
  virtual void UploadLog(const std::string& compressed_log_data,
                         const std::string& log_hash,
                         const ReportingInfo& reporting_info) = 0;
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_METRICS_LOG_UPLOADER_H_
