// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_METRICS_CAST_METRICS_SERVICE_CLIENT_H_
#define CHROMECAST_BROWSER_METRICS_CAST_METRICS_SERVICE_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/metrics_log_uploader.h"
#include "components/metrics/metrics_service_client.h"

class PrefRegistrySimple;
class PrefService;

namespace base {
class SingleThreadTaskRunner;
}

namespace metrics {
struct ClientInfo;
class MetricsService;
class MetricsStateManager;
}  // namespace metrics

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace chromecast {
namespace metrics {

class ExternalMetrics;

class CastMetricsServiceClient : public ::metrics::MetricsServiceClient,
                                 public ::metrics::EnabledStateProvider {
 public:
  CastMetricsServiceClient(PrefService* pref_service,
                           net::URLRequestContextGetter* request_context);
  ~CastMetricsServiceClient() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Use |client_id| when starting MetricsService instead of generating a new
  // client ID. If used, SetForceClientId must be called before Initialize.
  void SetForceClientId(const std::string& client_id);
  void OnApplicationNotIdle();

  // Processes all events from shared file. This should be used to consume all
  // events in the file before shutdown. This function is safe to call from any
  // thread.
  void ProcessExternalEvents(const base::Closure& cb);

  void Initialize();
  void Finalize();

  // ::metrics::MetricsServiceClient:
  ::metrics::MetricsService* GetMetricsService() override;
  void SetMetricsClientId(const std::string& client_id) override;
  int32_t GetProduct() override;
  std::string GetApplicationLocale() override;
  bool GetBrand(std::string* brand_code) override;
  ::metrics::SystemProfileProto::Channel GetChannel() override;
  std::string GetVersionString() override;
  void CollectFinalMetricsForLog(const base::Closure& done_callback) override;
  std::string GetMetricsServerUrl() override;
  std::unique_ptr<::metrics::MetricsLogUploader> CreateUploader(
      base::StringPiece server_url,
      base::StringPiece insecure_server_url,
      base::StringPiece mime_type,
      ::metrics::MetricsLogUploader::MetricServiceType service_type,
      const ::metrics::MetricsLogUploader::UploadCallback& on_upload_complete)
      override;
  base::TimeDelta GetStandardUploadInterval() override;

  // ::metrics::EnabledStateProvider:
  bool IsConsentGiven() const override;

  // Starts/stops the metrics service.
  void EnableMetricsService(bool enabled);

  std::string client_id() const { return client_id_; }

 private:
  std::unique_ptr<::metrics::ClientInfo> LoadClientInfo();
  void StoreClientInfo(const ::metrics::ClientInfo& client_info);

  PrefService* const pref_service_;
  std::string client_id_;
  std::string force_client_id_;
  bool client_info_loaded_;

#if defined(OS_LINUX)
  ExternalMetrics* external_metrics_;
  ExternalMetrics* platform_metrics_;
#endif  // defined(OS_LINUX)
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<::metrics::MetricsStateManager> metrics_state_manager_;
  std::unique_ptr<::metrics::MetricsService> metrics_service_;
  std::unique_ptr<::metrics::EnabledStateProvider> enabled_state_provider_;
  net::URLRequestContextGetter* const request_context_;

  DISALLOW_COPY_AND_ASSIGN(CastMetricsServiceClient);
};

}  // namespace metrics
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_METRICS_CAST_METRICS_SERVICE_CLIENT_H_
