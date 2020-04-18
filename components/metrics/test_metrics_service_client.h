// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_TEST_METRICS_SERVICE_CLIENT_H_
#define COMPONENTS_METRICS_TEST_METRICS_SERVICE_CLIENT_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "components/metrics/metrics_log_uploader.h"
#include "components/metrics/metrics_service_client.h"
#include "components/metrics/test_metrics_log_uploader.h"

namespace metrics {

// A simple concrete implementation of the MetricsServiceClient interface, for
// use in tests.
class TestMetricsServiceClient : public MetricsServiceClient {
 public:
  static const char kBrandForTesting[];

  TestMetricsServiceClient();
  ~TestMetricsServiceClient() override;

  // MetricsServiceClient:
  metrics::MetricsService* GetMetricsService() override;
  void SetMetricsClientId(const std::string& client_id) override;
  int32_t GetProduct() override;
  std::string GetApplicationLocale() override;
  bool GetBrand(std::string* brand_code) override;
  SystemProfileProto::Channel GetChannel() override;
  std::string GetVersionString() override;
  void CollectFinalMetricsForLog(const base::Closure& done_callback) override;
  std::unique_ptr<MetricsLogUploader> CreateUploader(
      base::StringPiece server_url,
      base::StringPiece insecure_server_url,
      base::StringPiece mime_type,
      MetricsLogUploader::MetricServiceType service_type,
      const MetricsLogUploader::UploadCallback& on_upload_complete) override;
  base::TimeDelta GetStandardUploadInterval() override;
  bool IsReportingPolicyManaged() override;
  EnableMetricsDefault GetMetricsReportingDefaultState() override;
  std::string GetAppPackageName() override;

  const std::string& get_client_id() const { return client_id_; }
  // Returns a weak ref to the last created uploader.
  TestMetricsLogUploader* uploader() { return uploader_; }
  void set_version_string(const std::string& str) { version_string_ = str; }
  void set_product(int32_t product) { product_ = product; }
  void set_reporting_is_managed(bool managed) {
    reporting_is_managed_ = managed;
  }
  void set_enable_default(EnableMetricsDefault enable_default) {
    enable_default_ = enable_default;
  }

 private:
  std::string client_id_;
  std::string version_string_;
  int32_t product_;
  bool reporting_is_managed_;
  EnableMetricsDefault enable_default_;

  // A weak ref to the last created TestMetricsLogUploader.
  TestMetricsLogUploader* uploader_;

  DISALLOW_COPY_AND_ASSIGN(TestMetricsServiceClient);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_TEST_METRICS_SERVICE_CLIENT_H_
