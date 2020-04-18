// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_server.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_switches.h"
#include "components/variations/variations_associated_data.h"
#include "net/base/host_port_pair.h"
#include "net/base/proxy_server.h"
#include "net/http/http_status_code.h"
#include "url/url_constants.h"

#if defined(OS_ANDROID)
#include "base/sys_info.h"
#endif

namespace {

const char kEnabled[] = "Enabled";
const char kControl[] = "Control";
const char kDisabled[] = "Disabled";
const char kDefaultSecureProxyCheckUrl[] = "http://check.googlezip.net/connect";
const char kDefaultWarmupUrl[] = "http://check.googlezip.net/e2e_probe";

const char kQuicFieldTrial[] = "DataReductionProxyUseQuic";

const char kLoFiFieldTrial[] = "DataCompressionProxyLoFi";
const char kLoFiFlagFieldTrial[] = "DataCompressionProxyLoFiFlag";

// Default URL for retrieving the Data Reduction Proxy configuration.
const char kClientConfigURL[] =
    "https://datasaver.googleapis.com/v1/clientConfigs";

// Default URL for sending pageload metrics.
const char kPingbackURL[] =
    "https://datasaver.googleapis.com/v1/metrics:recordPageloadMetrics";

// The name of the server side experiment field trial.
const char kServerExperimentsFieldTrial[] =
    "DataReductionProxyServerExperiments";

// LitePage black list version.
const char kLitePageBlackListVersion[] = "lite-page-blacklist-version";

const char kWarmupFetchCallbackEnabledParam[] = "warmup_fetch_callback_enabled";
const char kMissingViaBypassDisabledParam[] = "bypass_missing_via_disabled";
const char kDiscardCanaryCheckResultParam[] = "store_canary_check_result";

bool IsIncludedInFieldTrial(const std::string& name) {
  return base::StartsWith(base::FieldTrialList::FindFullName(name), kEnabled,
                          base::CompareCase::SENSITIVE);
}

// Returns the variation value for |parameter_name|. If the value is
// unavailable, |default_value| is returned.
std::string GetStringValueForVariationParamWithDefaultValue(
    const std::map<std::string, std::string>& variation_params,
    const std::string& parameter_name,
    const std::string& default_value) {
  const auto it = variation_params.find(parameter_name);
  if (it == variation_params.end())
    return default_value;
  return it->second;
}

bool CanShowAndroidLowMemoryDevicePromo() {
#if defined(OS_ANDROID)
  return base::SysInfo::IsLowEndDevice() &&
         base::FeatureList::IsEnabled(
             data_reduction_proxy::features::
                 kDataReductionProxyLowMemoryDevicePromo);
#endif
  return false;
}

}  // namespace

namespace data_reduction_proxy {
namespace params {

bool IsIncludedInPromoFieldTrial() {
  if (IsIncludedInFieldTrial("DataCompressionProxyPromoVisibility"))
    return true;

  return CanShowAndroidLowMemoryDevicePromo();
}

bool IsIncludedInFREPromoFieldTrial() {
  if (IsIncludedInFieldTrial("DataReductionProxyFREPromo"))
    return true;

  return CanShowAndroidLowMemoryDevicePromo();
}

bool IsIncludedInHoldbackFieldTrial() {
  return IsIncludedInFieldTrial("DataCompressionProxyHoldback");
}

std::string HoldbackFieldTrialGroup() {
  return base::FieldTrialList::FindFullName("DataCompressionProxyHoldback");
}

const char* GetLoFiFieldTrialName() {
  return kLoFiFieldTrial;
}

const char* GetLoFiFlagFieldTrialName() {
  return kLoFiFlagFieldTrial;
}

const char* GetWarmupCallbackParamName() {
  return kWarmupFetchCallbackEnabledParam;
}

const char* GetMissingViaBypassParamName() {
  return kMissingViaBypassDisabledParam;
}

const char* GetDiscardCanaryCheckResultParam() {
  return kDiscardCanaryCheckResultParam;
}

bool ShouldDiscardCanaryCheckResult() {
  return GetFieldTrialParamByFeatureAsBool(
      features::kDataReductionProxyRobustConnection,
      GetDiscardCanaryCheckResultParam(), false);
}

bool IsIncludedInServerExperimentsFieldTrial() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
             data_reduction_proxy::switches::
                 kDataReductionProxyServerExperimentsDisabled) &&
         base::FieldTrialList::FindFullName(kServerExperimentsFieldTrial)
                 .find(kDisabled) != 0;
}

bool FetchWarmupProbeURLEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableDataReductionProxyWarmupURLFetch);
}

GURL GetWarmupURL() {
  std::map<std::string, std::string> params;
  variations::GetVariationParams(GetQuicFieldTrialName(), &params);
  return GURL(GetStringValueForVariationParamWithDefaultValue(
      params, "warmup_url", kDefaultWarmupUrl));
}

bool IsWhitelistedHttpResponseCodeForProbes(int http_response_code) {
  // 200 and 404 are always whitelisted.
  if (http_response_code == net::HTTP_OK ||
      http_response_code == net::HTTP_NOT_FOUND) {
    return true;
  }

  // Check if there is an additional whitelisted HTTP response code provided via
  // the field trial params.
  std::map<std::string, std::string> params;
  variations::GetVariationParams(GetQuicFieldTrialName(), &params);
  const std::string value = GetStringValueForVariationParamWithDefaultValue(
      params, "whitelisted_probe_http_response_code", "");
  int response_code;
  if (!base::StringToInt(value, &response_code))
    return false;
  return response_code == http_response_code;
}

bool ShouldBypassMissingViaHeader(bool connection_is_cellular) {
  return GetFieldTrialParamByFeatureAsBool(
      data_reduction_proxy::features::kMissingViaHeaderShortDuration,
      connection_is_cellular ? "should_bypass_missing_via_cellular"
                             : "should_bypass_missing_via_wifi",
      true);
}

std::pair<base::TimeDelta, base::TimeDelta>
GetMissingViaHeaderBypassDurationRange(bool connection_is_cellular) {
  base::TimeDelta bypass_max =
      base::TimeDelta::FromSeconds(GetFieldTrialParamByFeatureAsInt(
          data_reduction_proxy::features::kMissingViaHeaderShortDuration,
          connection_is_cellular ? "missing_via_max_bypass_cellular_in_seconds"
                                 : "missing_via_max_bypass_wifi_in_seconds",
          300));
  base::TimeDelta bypass_min =
      base::TimeDelta::FromSeconds(GetFieldTrialParamByFeatureAsInt(
          data_reduction_proxy::features::kMissingViaHeaderShortDuration,
          connection_is_cellular ? "missing_via_min_bypass_cellular_in_seconds"
                                 : "missing_via_min_bypass_wifi_in_seconds",
          60));
  return {bypass_min, bypass_max};
}

bool IsForcePingbackEnabledViaFlags() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      data_reduction_proxy::switches::kEnableDataReductionProxyForcePingback);
}

bool WarnIfNoDataReductionProxy() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      data_reduction_proxy::switches::kEnableDataReductionProxyBypassWarning);
}

bool IsIncludedInQuicFieldTrial() {
  if (base::StartsWith(base::FieldTrialList::FindFullName(kQuicFieldTrial),
                       kControl, base::CompareCase::SENSITIVE)) {
    return false;
  }
  if (base::StartsWith(base::FieldTrialList::FindFullName(kQuicFieldTrial),
                       kDisabled, base::CompareCase::SENSITIVE)) {
    return false;
  }
  // QUIC is enabled by default.
  return true;
}

bool IsQuicEnabledForNonCoreProxies() {
  DCHECK(IsIncludedInQuicFieldTrial());
  std::map<std::string, std::string> params;
  variations::GetVariationParams(GetQuicFieldTrialName(), &params);
  return GetStringValueForVariationParamWithDefaultValue(
             params, "enable_quic_non_core_proxies", "true") != "false";
}

const char* GetQuicFieldTrialName() {
  return kQuicFieldTrial;
}

bool IsBrotliAcceptEncodingEnabled() {
  // Brotli encoding is enabled by default since the data reduction proxy server
  // controls when to serve Brotli encoded content. It can be disabled in
  // Chromium only if Chromium belongs to a field trial group whose name starts
  // with "Disabled".
  return !base::StartsWith(base::FieldTrialList::FindFullName(
                               "DataReductionProxyBrotliAcceptEncoding"),
                           kDisabled, base::CompareCase::SENSITIVE);
}

bool IsConfigClientEnabled() {
  // Config client is enabled by default. It can be disabled only if Chromium
  // belongs to a field trial group whose name starts with "Disabled".
  return !base::StartsWith(
      base::FieldTrialList::FindFullName("DataReductionProxyConfigService"),
      kDisabled, base::CompareCase::SENSITIVE);
}

GURL GetConfigServiceURL() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  std::string url;
  if (command_line->HasSwitch(switches::kDataReductionProxyConfigURL)) {
    url = command_line->GetSwitchValueASCII(
        switches::kDataReductionProxyConfigURL);
  }

  if (url.empty())
    return GURL(kClientConfigURL);

  GURL result(url);
  if (result.is_valid())
    return result;

  LOG(WARNING) << "The following client config URL specified at the "
               << "command-line or variation is invalid: " << url;
  return GURL(kClientConfigURL);
}

GURL GetPingbackURL() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  std::string url;
  if (command_line->HasSwitch(switches::kDataReductionPingbackURL)) {
    url =
        command_line->GetSwitchValueASCII(switches::kDataReductionPingbackURL);
  }

  if (url.empty())
    return GURL(kPingbackURL);

  GURL result(url);
  if (result.is_valid())
    return result;

  LOG(WARNING) << "The following page load metrics URL specified at the "
               << "command-line or variation is invalid: " << url;
  return GURL(kPingbackURL);
}

bool ShouldForceEnableDataReductionProxy() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      data_reduction_proxy::switches::kEnableDataReductionProxy);
}

int LitePageVersion() {
  return GetFieldTrialParameterAsInteger(
      data_reduction_proxy::params::GetLoFiFieldTrialName(),
      kLitePageBlackListVersion, 0, 0);
}

int GetFieldTrialParameterAsInteger(const std::string& group,
                                    const std::string& param_name,
                                    int default_value,
                                    int min_value) {
  DCHECK(default_value >= min_value);
  std::string param_value =
      variations::GetVariationParamValue(group, param_name);
  int value;
  if (param_value.empty() || !base::StringToInt(param_value, &value) ||
      value < min_value) {
    return default_value;
  }

  return value;
}

bool GetOverrideProxiesForHttpFromCommandLine(
    std::vector<DataReductionProxyServer>* override_proxies_for_http) {
  DCHECK(override_proxies_for_http);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDataReductionProxyHttpProxies)) {
    // It is illegal to use |kDataReductionProxy| or
    // |kDataReductionProxyFallback| with |kDataReductionProxyHttpProxies|.
    DCHECK(base::CommandLine::ForCurrentProcess()
               ->GetSwitchValueASCII(switches::kDataReductionProxy)
               .empty());
    DCHECK(base::CommandLine::ForCurrentProcess()
               ->GetSwitchValueASCII(switches::kDataReductionProxyFallback)
               .empty());
    override_proxies_for_http->clear();

    std::string proxy_overrides =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kDataReductionProxyHttpProxies);

    for (const auto& proxy_override :
         base::SplitStringPiece(proxy_overrides, ";", base::TRIM_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY)) {
      net::ProxyServer proxy_server = net::ProxyServer::FromURI(
          proxy_override, net::ProxyServer::SCHEME_HTTP);
      DCHECK(proxy_server.is_valid());
      DCHECK(!proxy_server.is_direct());

      // Overriding proxies have type UNSPECIFIED_TYPE.
      override_proxies_for_http->push_back(DataReductionProxyServer(
          std::move(proxy_server), ProxyServer::UNSPECIFIED_TYPE));
    }

    return true;
  }

  std::string origin =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kDataReductionProxy);

  std::string fallback_origin =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kDataReductionProxyFallback);

  if (origin.empty() && fallback_origin.empty())
    return false;

  override_proxies_for_http->clear();

  // Overriding proxies have type UNSPECIFIED_TYPE.
  if (!origin.empty()) {
    net::ProxyServer primary_proxy =
        net::ProxyServer::FromURI(origin, net::ProxyServer::SCHEME_HTTP);
    DCHECK(primary_proxy.is_valid());
    DCHECK(!primary_proxy.is_direct());
    override_proxies_for_http->push_back(DataReductionProxyServer(
        std::move(primary_proxy), ProxyServer::UNSPECIFIED_TYPE));
  }
  if (!fallback_origin.empty()) {
    net::ProxyServer fallback_proxy = net::ProxyServer::FromURI(
        fallback_origin, net::ProxyServer::SCHEME_HTTP);
    DCHECK(fallback_proxy.is_valid());
    DCHECK(!fallback_proxy.is_direct());
    override_proxies_for_http->push_back(DataReductionProxyServer(
        std::move(fallback_proxy), ProxyServer::UNSPECIFIED_TYPE));
  }

  return true;
}

const char* GetServerExperimentsFieldTrialName() {
  return kServerExperimentsFieldTrial;
}

GURL GetSecureProxyCheckURL() {
  std::string secure_proxy_check_url =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kDataReductionProxySecureProxyCheckURL);
  if (secure_proxy_check_url.empty())
    secure_proxy_check_url = kDefaultSecureProxyCheckUrl;

  return GURL(secure_proxy_check_url);
}

}  // namespace params

DataReductionProxyParams::DataReductionProxyParams() {
  bool use_override_proxies_for_http =
      params::GetOverrideProxiesForHttpFromCommandLine(&proxies_for_http_);

  if (!use_override_proxies_for_http) {
    DCHECK(proxies_for_http_.empty());
    proxies_for_http_.push_back(DataReductionProxyServer(
        net::ProxyServer::FromURI("https://proxy.googlezip.net:443",
                                  net::ProxyServer::SCHEME_HTTP),
        ProxyServer::CORE));
    proxies_for_http_.push_back(DataReductionProxyServer(
        net::ProxyServer::FromURI("compress.googlezip.net:80",
                                  net::ProxyServer::SCHEME_HTTP),
        ProxyServer::CORE));
  }

  DCHECK(std::all_of(proxies_for_http_.begin(), proxies_for_http_.end(),
                     [](const DataReductionProxyServer& proxy) {
                       return proxy.proxy_server().is_valid() &&
                              !proxy.proxy_server().is_direct();
                     }));
}

DataReductionProxyParams::~DataReductionProxyParams() {}

void DataReductionProxyParams::SetProxiesForHttpForTesting(
    const std::vector<DataReductionProxyServer>& proxies_for_http) {
  DCHECK(std::all_of(proxies_for_http.begin(), proxies_for_http.end(),
                     [](const DataReductionProxyServer& proxy) {
                       return proxy.proxy_server().is_valid() &&
                              !proxy.proxy_server().is_direct();
                     }));

  proxies_for_http_ = proxies_for_http;
}

const std::vector<DataReductionProxyServer>&
DataReductionProxyParams::proxies_for_http() const {
  return proxies_for_http_;
}

base::Optional<DataReductionProxyTypeInfo>
DataReductionProxyParams::FindConfiguredDataReductionProxy(
    const net::ProxyServer& proxy_server) const {
  return FindConfiguredProxyInVector(proxies_for_http(), proxy_server);
}

// static
base::Optional<DataReductionProxyTypeInfo>
DataReductionProxyParams::FindConfiguredProxyInVector(
    const std::vector<DataReductionProxyServer>& proxies,
    const net::ProxyServer& proxy_server) {
  if (!proxy_server.is_valid() || proxy_server.is_direct())
    return base::nullopt;

  // Only compare the host port pair of the |proxy_server| since the proxy
  // scheme of the stored data reduction proxy may be different than the proxy
  // scheme of |proxy_server|. This may happen even when the |proxy_server| is a
  // valid data reduction proxy. As an example, the stored data reduction proxy
  // may have a proxy scheme of HTTPS while |proxy_server| may have QUIC as the
  // proxy scheme.
  const net::HostPortPair& host_port_pair = proxy_server.host_port_pair();
  auto it = std::find_if(
      proxies.begin(), proxies.end(),
      [&host_port_pair](const DataReductionProxyServer& proxy) {
        return proxy.proxy_server().host_port_pair().Equals(host_port_pair);
      });

  if (it == proxies.end())
    return base::nullopt;

  return DataReductionProxyTypeInfo(proxies,
                                    static_cast<size_t>(it - proxies.begin()));
}

}  // namespace data_reduction_proxy
