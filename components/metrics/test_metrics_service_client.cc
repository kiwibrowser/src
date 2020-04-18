// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/test_metrics_service_client.h"

#include <memory>

#include "base/callback.h"
#include "components/metrics/metrics_log_uploader.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"

namespace metrics {

// static
const char TestMetricsServiceClient::kBrandForTesting[] = "brand_for_testing";

TestMetricsServiceClient::TestMetricsServiceClient()
    : version_string_("5.0.322.0-64-devel"),
      product_(ChromeUserMetricsExtension::CHROME),
      reporting_is_managed_(false),
      enable_default_(EnableMetricsDefault::DEFAULT_UNKNOWN) {}

TestMetricsServiceClient::~TestMetricsServiceClient() {
}

metrics::MetricsService* TestMetricsServiceClient::GetMetricsService() {
  return nullptr;
}

void TestMetricsServiceClient::SetMetricsClientId(
    const std::string& client_id) {
  client_id_ = client_id;
}

int32_t TestMetricsServiceClient::GetProduct() {
  return product_;
}

std::string TestMetricsServiceClient::GetApplicationLocale() {
  return "en-US";
}

bool TestMetricsServiceClient::GetBrand(std::string* brand_code) {
  *brand_code = kBrandForTesting;
  return true;
}

SystemProfileProto::Channel TestMetricsServiceClient::GetChannel() {
  return SystemProfileProto::CHANNEL_BETA;
}

std::string TestMetricsServiceClient::GetVersionString() {
  return version_string_;
}

void TestMetricsServiceClient::CollectFinalMetricsForLog(
    const base::Closure& done_callback) {
  done_callback.Run();
}

std::unique_ptr<MetricsLogUploader> TestMetricsServiceClient::CreateUploader(
    base::StringPiece server_url,
    base::StringPiece insecure_server_url,
    base::StringPiece mime_type,
    MetricsLogUploader::MetricServiceType service_type,
    const MetricsLogUploader::UploadCallback& on_upload_complete) {
  uploader_ = new TestMetricsLogUploader(on_upload_complete);
  return std::unique_ptr<MetricsLogUploader>(uploader_);
}

base::TimeDelta TestMetricsServiceClient::GetStandardUploadInterval() {
  return base::TimeDelta::FromMinutes(5);
}

bool TestMetricsServiceClient::IsReportingPolicyManaged() {
  return reporting_is_managed_;
}

EnableMetricsDefault
TestMetricsServiceClient::GetMetricsReportingDefaultState() {
  return enable_default_;
}

std::string TestMetricsServiceClient::GetAppPackageName() {
  return "test app";
}

}  // namespace metrics
