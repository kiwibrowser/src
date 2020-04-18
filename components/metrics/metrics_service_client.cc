// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/metrics_service_client.h"

#include "components/metrics/url_constants.h"

namespace metrics {

MetricsServiceClient::MetricsServiceClient() {}

MetricsServiceClient::~MetricsServiceClient() {}

ukm::UkmService* MetricsServiceClient::GetUkmService() {
  return nullptr;
}

bool MetricsServiceClient::IsReportingPolicyManaged() {
  return false;
}

EnableMetricsDefault MetricsServiceClient::GetMetricsReportingDefaultState() {
  return EnableMetricsDefault::DEFAULT_UNKNOWN;
}

bool MetricsServiceClient::IsUMACellularUploadLogicEnabled() {
  return false;
}

std::string MetricsServiceClient::GetMetricsServerUrl() {
  return kNewMetricsServerUrl;
}

std::string MetricsServiceClient::GetInsecureMetricsServerUrl() {
  return kNewMetricsServerUrlInsecure;
}

bool MetricsServiceClient::SyncStateAllowsUkm() {
  return false;
}

bool MetricsServiceClient::SyncStateAllowsExtensionUkm() {
  return false;
}

bool MetricsServiceClient::AreNotificationListenersEnabledOnAllProfiles() {
  return false;
}

void MetricsServiceClient::SetUpdateRunningServicesCallback(
    const base::Closure& callback) {
  update_running_services_ = callback;
}

void MetricsServiceClient::UpdateRunningServices() {
  if (update_running_services_)
    update_running_services_.Run();
}

std::string MetricsServiceClient::GetAppPackageName() {
  return std::string();
}

}  // namespace metrics
