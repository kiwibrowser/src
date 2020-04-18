// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_METRICS_SERVICE_CLIENT_H_
#define COMPONENTS_METRICS_METRICS_SERVICE_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/metrics/metrics_log_uploader.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "third_party/metrics_proto/system_profile.pb.h"

namespace base {
class FilePath;
}

namespace ukm {
class UkmService;
}

namespace metrics {

class MetricsLogUploader;
class MetricsService;

// An abstraction of operations that depend on the embedder's (e.g. Chrome)
// environment.
class MetricsServiceClient {
 public:
  MetricsServiceClient();
  virtual ~MetricsServiceClient();

  // Returns the MetricsService instance that this client is associated with.
  // With the exception of testing contexts, the returned instance must be valid
  // for the lifetime of this object (typically, the embedder's client
  // implementation will own the MetricsService instance being returned).
  virtual MetricsService* GetMetricsService() = 0;

  // Returns the UkmService instance that this client is associated with.
  virtual ukm::UkmService* GetUkmService();

  // Registers the client id with other services (e.g. crash reporting), called
  // when metrics recording gets enabled.
  virtual void SetMetricsClientId(const std::string& client_id) = 0;

  // Returns the product value to use in uploaded reports, which will be used to
  // set the ChromeUserMetricsExtension.product field. See comments on that
  // field on why it's an int32_t rather than an enum.
  virtual int32_t GetProduct() = 0;

  // Returns the current application locale (e.g. "en-US").
  virtual std::string GetApplicationLocale() = 0;

  // Retrieves the brand code string associated with the install, returning
  // false if no brand code is available.
  virtual bool GetBrand(std::string* brand_code) = 0;

  // Returns the release channel (e.g. stable, beta, etc) of the application.
  virtual SystemProfileProto::Channel GetChannel() = 0;

  // Returns the version of the application as a string.
  virtual std::string GetVersionString() = 0;

  // Called by the metrics service when a new environment has been recorded.
  // Takes the serialized environment as a parameter. The contents of
  // |serialized_environment| are consumed by the call, but the caller maintains
  // ownership.
  virtual void OnEnvironmentUpdate(std::string* serialized_environment) {}

  // Called by the metrics service to record a clean shutdown.
  virtual void OnLogCleanShutdown() {}

  // Called prior to a metrics log being closed, allowing the client to collect
  // extra histograms that will go in that log. Asynchronous API - the client
  // implementation should call |done_callback| when complete.
  virtual void CollectFinalMetricsForLog(
      const base::Closure& done_callback) = 0;

  // Get the URL of the metrics server.
  virtual std::string GetMetricsServerUrl();

  // Get the fallback HTTP URL of the metrics server.
  virtual std::string GetInsecureMetricsServerUrl();

  // Creates a MetricsLogUploader with the specified parameters (see comments on
  // MetricsLogUploader for details).
  virtual std::unique_ptr<MetricsLogUploader> CreateUploader(
      base::StringPiece server_url,
      base::StringPiece insecure_server_url,
      base::StringPiece mime_type,
      metrics::MetricsLogUploader::MetricServiceType service_type,
      const MetricsLogUploader::UploadCallback& on_upload_complete) = 0;

  // Returns the standard interval between upload attempts.
  virtual base::TimeDelta GetStandardUploadInterval() = 0;

  // Called on plugin loading errors.
  virtual void OnPluginLoadingError(const base::FilePath& plugin_path) {}

  // Called on renderer crashes in some embedders (e.g., those that do not use
  // //content and thus do not have //content's notification system available
  // as a mechanism for observing renderer crashes).
  virtual void OnRendererProcessCrash() {}

  // Returns whether metrics reporting is managed by policy.
  virtual bool IsReportingPolicyManaged();

  // Gets information about the default value for the metrics reporting checkbox
  // shown during first-run.
  virtual EnableMetricsDefault GetMetricsReportingDefaultState();

  // Returns whether cellular logic is enabled for metrics reporting.
  virtual bool IsUMACellularUploadLogicEnabled();

  // Returns true iff sync is in a state that allows UKM to be enabled.
  // See //components/ukm/observers/sync_disable_observer.h for details.
  virtual bool SyncStateAllowsUkm();

  // Returns true iff sync is in a state that allows UKM to capture extensions.
  // See //components/ukm/observers/sync_disable_observer.h for details.
  virtual bool SyncStateAllowsExtensionUkm();

  // Returns whether UKM notification listeners were attached to all profiles.
  virtual bool AreNotificationListenersEnabledOnAllProfiles();

  // Gets the Chrome package name for Android. Returns empty string for other
  // platforms.
  virtual std::string GetAppPackageName();

  // Sets the callback to run MetricsServiceManager::UpdateRunningServices.
  void SetUpdateRunningServicesCallback(const base::Closure& callback);

  // Notify MetricsServiceManager to UpdateRunningServices using callback.
  void UpdateRunningServices();

 private:
  base::Closure update_running_services_;

  DISALLOW_COPY_AND_ASSIGN(MetricsServiceClient);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_METRICS_SERVICE_CLIENT_H_
