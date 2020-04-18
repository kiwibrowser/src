// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_METRICS_LOG_UPLOADER_H_
#define ANDROID_WEBVIEW_BROWSER_AW_METRICS_LOG_UPLOADER_H_

#include <jni.h>
#include <string>

#include "components/metrics/metrics_log_uploader.h"

namespace android_webview {

// Uploads UMA logs for WebView using the platform logging mechanism.
class AwMetricsLogUploader : public ::metrics::MetricsLogUploader {
 public:
  explicit AwMetricsLogUploader(
      const ::metrics::MetricsLogUploader::UploadCallback& on_upload_complete);

  ~AwMetricsLogUploader() override;

  // ::metrics::MetricsLogUploader:
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& log_hash,
                 const metrics::ReportingInfo& reporting_info) override;

 private:
  const metrics::MetricsLogUploader::UploadCallback on_upload_complete_;

  DISALLOW_COPY_AND_ASSIGN(AwMetricsLogUploader);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_METRICS_LOG_UPLOADER_H_
