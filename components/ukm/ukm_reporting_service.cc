// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ReportingService specialized to report UKM metrics.

#include "components/ukm/ukm_reporting_service.h"

#include <memory>

#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/ukm/persisted_logs_metrics_impl.h"
#include "components/ukm/ukm_pref_names.h"
#include "components/ukm/ukm_service.h"

namespace ukm {

namespace {

// The UKM server's URL.
constexpr char kMimeType[] = "application/vnd.chrome.ukm";

// The number of UKM logs that will be stored in PersistedLogs before logs
// start being dropped.
constexpr int kMinPersistedLogs = 8;

// The number of bytes UKM logs that will be stored in PersistedLogs before
// logs start being dropped.
// This ensures that a reasonable amount of history will be stored even if there
// is a long series of very small logs.
constexpr int kMinPersistedBytes = 300000;

// If an upload fails, and the transmission was over this byte count, then we
// will discard the log, and not try to retransmit it.  We also don't persist
// the log to the prefs for transmission during the next chrome session if this
// limit is exceeded.
constexpr size_t kMaxLogRetransmitSize = 100 * 1024;

std::string GetServerUrl() {
  constexpr char kDefaultServerUrl[] = "https://clients4.google.com/ukm";
  std::string server_url =
      base::GetFieldTrialParamValueByFeature(kUkmFeature, "ServerUrl");
  if (!server_url.empty())
    return server_url;
  return kDefaultServerUrl;
}

}  // namespace

// static
void UkmReportingService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kUkmPersistedLogs);
  // Base class already registered by MetricsReportingService::RegisterPrefs
  // ReportingService::RegisterPrefs(registry);
}

UkmReportingService::UkmReportingService(metrics::MetricsServiceClient* client,
                                         PrefService* local_state)
    : ReportingService(client, local_state, kMaxLogRetransmitSize),
      persisted_logs_(std::make_unique<ukm::PersistedLogsMetricsImpl>(),
                      local_state,
                      prefs::kUkmPersistedLogs,
                      kMinPersistedLogs,
                      kMinPersistedBytes,
                      kMaxLogRetransmitSize) {}

UkmReportingService::~UkmReportingService() {}

metrics::LogStore* UkmReportingService::log_store() {
  return &persisted_logs_;
}

std::string UkmReportingService::GetUploadUrl() const {
  return GetServerUrl();
}

std::string UkmReportingService::GetInsecureUploadUrl() const {
  return "";
}

base::StringPiece UkmReportingService::upload_mime_type() const {
  return kMimeType;
}

metrics::MetricsLogUploader::MetricServiceType
UkmReportingService::service_type() const {
  return metrics::MetricsLogUploader::UKM;
}

void UkmReportingService::LogCellularConstraint(bool upload_canceled) {
  UMA_HISTOGRAM_BOOLEAN("UKM.LogUpload.Canceled.CellularConstraint",
                        upload_canceled);
}

void UkmReportingService::LogResponseOrErrorCode(int response_code,
                                                 int error_code,
                                                 bool was_https) {
  // |was_https| is ignored since all UKM logs are received over HTTPS.
  base::UmaHistogramSparse("UKM.LogUpload.ResponseOrErrorCode",
                           response_code >= 0 ? response_code : error_code);
}

void UkmReportingService::LogSuccess(size_t log_size) {
  UMA_HISTOGRAM_COUNTS_10000("UKM.LogSize.OnSuccess", log_size / 1024);
}

void UkmReportingService::LogLargeRejection(size_t log_size) {}

}  // namespace metrics
