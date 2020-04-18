// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_CONFIG_SERVICE_CLIENT_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_CONFIG_SERVICE_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "net/base/backoff_entry.h"
#include "net/base/network_change_notifier.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "base/android/application_status_listener.h"
#endif  // OS_ANDROID

namespace net {
class HttpRequestHeaders;
class HttpResponseHeaders;
class NetLog;
struct LoadTimingInfo;
class ProxyServer;
class URLFetcher;
class URLRequestContextGetter;
class URLRequestStatus;
}

namespace data_reduction_proxy {

class ClientConfig;
class DataReductionProxyConfig;
class DataReductionProxyEventCreator;
class DataReductionProxyIOData;
class DataReductionProxyMutableConfigValues;
class DataReductionProxyParams;
class DataReductionProxyRequestOptions;

typedef base::Callback<void(const std::string&)> ConfigStorer;

// Retrieves the default net::BackoffEntry::Policy for the Data Reduction Proxy
// configuration service client.
const net::BackoffEntry::Policy& GetBackoffPolicy();

// Retrieves the Data Reduction Proxy configuration from a remote service. This
// object lives on the IO thread.
// TODO(jeremyim): Rename the class to DataReductionProxyConfigGetter(?).
//
// The client config module is a state machine with 2 states:
// 1) Chrome has a config. Requests will attempt to use DRP with that config.
// 2) Chrome doesn’t have a config. Requests will go direct.
//
// When Chrome starts up, if there is a cached config on disk, it is loaded. Go
// to state (1). Otherwise, go to state (2).
//
// When a config fetch finishes, move to state (1). If already in state (1),
// replace the existing config.
//
// When in state (1), if a response comes back 407 whose request was made with
// the existing config, invalidate the existing config and move to state (2).
// Retry the request on the direct path.
//
// The following events will trigger a config fetch, without invalidating the
// existing config. The existing config will be replaced when the async config
// fetch returns.
// * Starting Chrome.
// * Using a config whose refresh_duration has expired (see
//   components/data_reduction_proxy/proto/client_config.proto).
// * Getting a IP address change event notification.
//
// Config fetches are async and subject to a backoff policy. On Android, the
// fetch policy is different if Chrome is in the background. Every time a config
// is fetched, it is written to the disk.
class DataReductionProxyConfigServiceClient
    : public net::NetworkChangeNotifier::IPAddressObserver,
      public net::URLFetcherDelegate {
 public:
  // The caller must ensure that all parameters remain alive for the lifetime of
  // the |DataReductionProxyConfigClient|, with the exception of |params|
  // which this instance will own.
  DataReductionProxyConfigServiceClient(
      std::unique_ptr<DataReductionProxyParams> params,
      const net::BackoffEntry::Policy& backoff_policy,
      DataReductionProxyRequestOptions* request_options,
      DataReductionProxyMutableConfigValues* config_values,
      DataReductionProxyConfig* config,
      DataReductionProxyEventCreator* event_creator,
      DataReductionProxyIOData* io_data,
      net::NetLog* net_log,
      ConfigStorer config_storer);

  ~DataReductionProxyConfigServiceClient() override;

  // Performs initialization on the IO thread.
  void InitializeOnIOThread(
      net::URLRequestContextGetter* url_request_context_getter);

  // Sets whether the configuration should be retrieved or not.
  void SetEnabled(bool enabled);

  // Request the retrieval of the Data Reduction Proxy configuration. This
  // operation takes place asynchronously.
  void RetrieveConfig();

  // Takes a serialized Data Reduction Proxy configuration and sets it as the
  // current Data Reduction Proxy configuration. If a remote configuration has
  // already been retrieved, the remote configuration takes precedence.
  void ApplySerializedConfig(const std::string& config_value);

  // Examines |response_headers| to determine if an authentication failure
  // occurred on a Data Reduction Proxy. Returns true if authentication failure
  // occurred, and the session key specified in |request_headers| matches the
  // current session in use by the client. If an authentication failure is
  // detected,  it fetches a new config.
  bool ShouldRetryDueToAuthFailure(
      const net::HttpRequestHeaders& request_headers,
      const net::HttpResponseHeaders* response_headers,
      const net::ProxyServer& proxy_server,
      const net::LoadTimingInfo& load_timing_info);

 protected:
  // Retrieves the backoff entry object being used to throttle request failures.
  // Virtual for testing.
  virtual net::BackoffEntry* GetBackoffEntry();

  // Returns the current time.
  // Virtual for testing.
  virtual base::Time Now();

  // Sets a timer to determine when to next refresh the Data Reduction Proxy
  // configuration.
  void SetConfigRefreshTimer(const base::TimeDelta& delay);

#if defined(OS_ANDROID)
  // Returns true if Chromium is in background.
  // Virtualized for mocking.
  virtual bool IsApplicationStateBackground() const;
#endif

  // Returns true if the remote config has been applied. Virtualized for
  // testing.
  virtual bool RemoteConfigApplied() const;

 private:
  friend class TestDataReductionProxyConfigServiceClient;

  // Returns the duration after which the Data Reduction Proxy configuration
  // should be retrieved. |backoff_delay| must be non-negative.
  base::TimeDelta CalculateNextConfigRefreshTime(
      bool fetch_succeeded,
      const base::TimeDelta& config_expiration,
      const base::TimeDelta& backoff_delay);

  // Override of net::NetworkChangeNotifier::IPAddressObserver.
  void OnIPAddressChanged() override;

  // Override of net::URLFetcherDelegate.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Retrieves the Data Reduction Proxy configuration from a remote service.
  void RetrieveRemoteConfig();

  // Invalidates the current Data Reduction Proxy configuration.
  void InvalidateConfig();

  // Returns a fetcher to retrieve the Data Reduction Proxy configuration.
  // |secure_proxy_check_url| is the url from which to retrieve the config.
  // |request_body| is the request body sent to the configuration service.
  std::unique_ptr<net::URLFetcher> GetURLFetcherForConfig(
      const GURL& secure_proxy_check_url,
      const std::string& request_body);

  // Handles the response from the remote Data Reduction Proxy configuration
  // service. |response| is the response body, |status| is the
  // |net::URLRequestStatus| of the response, and response_code is the HTTP
  // response code (if available).
  void HandleResponse(const std::string& response,
                      const net::URLRequestStatus& status,
                      int response_code);

  // Parses out the proxy configuration portion of |config| and applies it to
  // |config_| and |request_options_|. Takes into account the field trials that
  // this session belongs to. Returns true if the |config| was successfully
  // parsed and applied.
  bool ParseAndApplyProxyConfig(const ClientConfig& config);

#if defined(OS_ANDROID)
  // Listens to when Chromium comes to foreground and fetches new client config
  // if the config fetch is pending.
  void OnApplicationStateChange(base::android::ApplicationState new_state);
#endif

  std::unique_ptr<DataReductionProxyParams> params_;

  // The caller must ensure that the |request_options_| outlives this instance.
  DataReductionProxyRequestOptions* request_options_;

  // The caller must ensure that the |config_values_| outlives this instance.
  DataReductionProxyMutableConfigValues* config_values_;

  // The caller must ensure that the |config_| outlives this instance.
  DataReductionProxyConfig* config_;

  // The caller must ensure that the |event_creator_| outlives this instance.
  DataReductionProxyEventCreator* event_creator_;

  // The caller must ensure that the |io_data_| outlives this instance.
  DataReductionProxyIOData* io_data_;

  // The caller must ensure that the |net_log_| outlives this instance.
  net::NetLog* net_log_;

  // Used to persist a serialized Data Reduction Proxy configuration.
  ConfigStorer config_storer_;

  // Used to calculate the backoff time on request failures.
  net::BackoffEntry backoff_entry_;

  // The URL for retrieving the Data Reduction Proxy configuration.
  GURL config_service_url_;

  // True if the Data Reduction Proxy configuration should be retrieved.
  bool enabled_;

  // True if a remote Data Reduction Proxy configuration has been retrieved and
  // successfully applied.
  bool remote_config_applied_;

  // Used for setting up |fetcher_|.
  net::URLRequestContextGetter* url_request_context_getter_;

  // An event that fires when it is time to refresh the Data Reduction Proxy
  // configuration.
  base::OneShotTimer config_refresh_timer_;

  // A |net::URLFetcher| to retrieve the Data Reduction Proxy configuration.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // Used to correlate the start and end of requests.
  net::NetLogWithSource net_log_with_source_;

  // Used to determine the latency in retrieving the Data Reduction Proxy
  // configuration.
  base::TimeTicks config_fetch_start_time_;

#if defined(OS_ANDROID)
  // Listens to the application transitions from foreground to background or
  // vice versa.
  std::unique_ptr<base::android::ApplicationStatusListener>
      app_status_listener_;

  // True if config needs to be fetched when the application comes to
  // foreground.
  bool foreground_fetch_pending_;
#endif

  // Keeps track of whether the previous request to a Data Reduction Proxy
  // failed to authenticate. This is necessary in the situation where a new
  // configuration with a bad session key is obtained, but the previous request
  // failed to authenticate, since the new configuration marks |backoff_entry_|
  // with a success, resulting in no net increase in the backoff timer.
  bool previous_request_failed_authentication_;

  // Number of failed fetch attempts before the config is fetched successfully.
  // It is reset to 0 every time there is a change in IP address, or when the
  // config is fetched successfully.
  int32_t failed_attempts_before_success_;

  // Time when the IP address last changed.
  base::TimeTicks last_ip_address_change_;

  // True if a client config fetch is in progress.
  bool fetch_in_progress_;

  // Enforce usage on the IO thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyConfigServiceClient);
};

}  // namespace data_reduction_proxy
#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_CONFIG_SERVICE_CLIENT_H_
