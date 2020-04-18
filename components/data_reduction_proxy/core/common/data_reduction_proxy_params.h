// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_PARAMS_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_PARAMS_H_

#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_config_values.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_type_info.h"
#include "url/gurl.h"

namespace net {
class ProxyServer;
}

namespace data_reduction_proxy {

class DataReductionProxyServer;

// The data_reduction_proxy::params namespace is a collection of methods to
// determine the operating parameters of the Data Reduction Proxy as specified
// by field trials and command line switches.
namespace params {

// Returns true if this client is part of a field trial that should display
// a promotion for the data reduction proxy.
bool IsIncludedInPromoFieldTrial();

// Returns true if this client is part of a field trial that should display
// a FRE promotion for the data reduction proxy.
bool IsIncludedInFREPromoFieldTrial();

// Returns true if this client is part of a field trial that runs a holdback
// experiment. A holdback experiment is one in which a fraction of browser
// instances will not be configured to use the data reduction proxy even if
// users have enabled it to be used. The UI will not indicate that a holdback
// is in effect.
bool IsIncludedInHoldbackFieldTrial();

// The name of the Holdback experiment group, this can return an empty string if
// not included in a group.
std::string HoldbackFieldTrialGroup();

// Returns the name of the Lo-Fi field trial.
// TODO(ryansturm): crbug.com/759052 Cleanup once fully cutover to new blacklist
const char* GetLoFiFieldTrialName();

// Returns the name of the Lo-Fi field trial that configures LoFi flags when it
// is force enabled through flags.
// TODO(ryansturm): crbug.com/759052 Cleanup once fully cutover to new blacklist
const char* GetLoFiFlagFieldTrialName();

// Returns true if this client is part of the field trial that should enable
// server experiments for the data reduction proxy.
bool IsIncludedInServerExperimentsFieldTrial();

// Returns true if this client has the command line switch to enable forced
// pageload metrics pingbacks on every page load.
bool IsForcePingbackEnabledViaFlags();

// Returns true if this client has the command line switch to show
// interstitials for data reduction proxy bypasses.
bool WarnIfNoDataReductionProxy();

// Returns true if this client is part of a field trial that sets the origin
// proxy server as quic://proxy.googlezip.net.
bool IsIncludedInQuicFieldTrial();

// Returns true if QUIC is enabled for non core data reduction proxies.
bool IsQuicEnabledForNonCoreProxies();

const char* GetQuicFieldTrialName();

// Returns true if Brotli should be added to the accept-encoding header.
bool IsBrotliAcceptEncodingEnabled();

// Returns true if the Data Reduction Proxy config client should be used.
bool IsConfigClientEnabled();

// If the Data Reduction Proxy is used for a page load, the URL for the
// Data Reduction Proxy Pageload Metrics service.
GURL GetPingbackURL();

// If the Data Reduction Proxy config client is being used, the URL for the
// Data Reduction Proxy config service.
GURL GetConfigServiceURL();

// Returns true if the Data Reduction Proxy is forced to be enabled from the
// command line.
bool ShouldForceEnableDataReductionProxy();

// Returns whether the proxy should be bypassed for requests that are proxied
// but missing the via header based on if the connection is cellular.
bool ShouldBypassMissingViaHeader(bool connection_is_cellular);

// Returns the range of acceptable bypass lengths for requests that are proxied
// but missing the via header based on if the connection is cellular.
std::pair<base::TimeDelta, base::TimeDelta>
GetMissingViaHeaderBypassDurationRange(bool connection_is_cellular);

// The current LitePage experiment blacklist version.
int LitePageVersion();

// Retrieves the int stored in |param_name| from the field trial group
// |group|. If the value is not present, cannot be parsed, or is less than
// |min_value|, returns |default_value|.
int GetFieldTrialParameterAsInteger(const std::string& group,
                                    const std::string& param_name,
                                    int default_value,
                                    int min_value);

// Returns true if the list of Data Reduction Proxies to use for HTTP requests
// has been overridden on the command line, and if so, returns the override
// proxy list in |override_proxies_for_http|.
bool GetOverrideProxiesForHttpFromCommandLine(
    std::vector<DataReductionProxyServer>* override_proxies_for_http);

// Returns the name of the server side experiment field trial.
const char* GetServerExperimentsFieldTrialName();

// Returns the URL to check to decide if the secure proxy origin should be
// used.
GURL GetSecureProxyCheckURL();

// Returns true if fetching of the warmup URL is enabled.
bool FetchWarmupProbeURLEnabled();

// Returns the warmup URL.
GURL GetWarmupURL();

// Returns true if the |http_response_code| is in the whitelist of HTTP response
// codes that are considered as successful for fetching the warmup probe URL.
// If this method returns false, then the probe should be considered as
// unsuccessful.
bool IsWhitelistedHttpResponseCodeForProbes(int http_response_code);

// Returns the experiment parameter name to enable the warmup fetch callback.
const char* GetWarmupCallbackParamName();

// Returns the experiment parameter name to disable missing via header bypasses.
const char* GetMissingViaBypassParamName();

// Returns the experiment parameter name to discard the cached result for canary
// check probe.
const char* GetDiscardCanaryCheckResultParam();

// Returns true if canary check result should not be cached or reused across
// network changes.
bool ShouldDiscardCanaryCheckResult();

}  // namespace params

// Provides initialization parameters. Proxy origins, and the secure proxy
// check url are are taken from flags if available and from preprocessor
// constants otherwise. The DataReductionProxySettings class and others use this
// class to determine the necessary DNS names to configure use of the Data
// Reduction Proxy.
class DataReductionProxyParams : public DataReductionProxyConfigValues {
 public:
  // Constructs configuration parameters. A standard configuration has a primary
  // proxy, and a fallback proxy for HTTP traffic.
  DataReductionProxyParams();

  // Updates |proxies_for_http_|.
  void SetProxiesForHttpForTesting(
      const std::vector<DataReductionProxyServer>& proxies_for_http);

  ~DataReductionProxyParams() override;

  const std::vector<DataReductionProxyServer>& proxies_for_http()
      const override;

  // Finds the first proxy in |proxies_for_http()| that matches |proxy_server|
  // if any exist.
  base::Optional<DataReductionProxyTypeInfo> FindConfiguredDataReductionProxy(
      const net::ProxyServer& proxy_server) const override;

  // Helper function to locate |proxy_server| in |proxies| if it exists. This
  // function is exposed publicly so that DataReductionProxyParams can use it.
  static base::Optional<DataReductionProxyTypeInfo> FindConfiguredProxyInVector(
      const std::vector<DataReductionProxyServer>& proxies,
      const net::ProxyServer& proxy_server);

 private:
  std::vector<DataReductionProxyServer> proxies_for_http_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyParams);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_PARAMS_H_
