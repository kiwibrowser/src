// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/data_reduction_proxy/content/browser/data_reduction_proxy_pingback_client_impl.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_compression_stats.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_service.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_settings.h"
#include "components/data_reduction_proxy/core/browser/data_store.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "components/proxy_config/proxy_prefs.h"
#include "net/base/host_port_pair.h"
#include "net/base/proxy_server.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_list.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

// Assume that any proxy host ending with this suffix is a Data Reduction Proxy.
const char kDataReductionProxyDefaultHostSuffix[] = ".googlezip.net";

// Searches |proxy_list| for any Data Reduction Proxies, even if they don't
// match a currently configured Data Reduction Proxy.
bool ContainsDataReductionProxyDefaultHostSuffix(
    const net::ProxyList& proxy_list) {
  for (const net::ProxyServer& proxy : proxy_list.GetAll()) {
    if (proxy.is_valid() && !proxy.is_direct() &&
        base::EndsWith(proxy.host_port_pair().host(),
                       kDataReductionProxyDefaultHostSuffix,
                       base::CompareCase::SENSITIVE)) {
      return true;
    }
  }
  return false;
}

// Searches |proxy_rules| for any Data Reduction Proxies, even if they don't
// match a currently configured Data Reduction Proxy.
bool ContainsDataReductionProxyDefaultHostSuffix(
    const net::ProxyConfig::ProxyRules& proxy_rules) {
  return ContainsDataReductionProxyDefaultHostSuffix(
             proxy_rules.proxies_for_http) ||
         ContainsDataReductionProxyDefaultHostSuffix(
             proxy_rules.proxies_for_https);
}

// Extract the embedded PAC script from the given |pac_url|, and store the
// extracted script in |pac_script|. Returns true if extraction was successful,
// otherwise returns false. |pac_script| must not be NULL.
bool GetEmbeddedPacScript(base::StringPiece pac_url, std::string* pac_script) {
  DCHECK(pac_script);
  static const char kPacURLPrefix[] =
      "data:application/x-ns-proxy-autoconfig;base64,";
  return base::StartsWith(pac_url, kPacURLPrefix,
                          base::CompareCase::SENSITIVE) &&
         base::Base64Decode(pac_url.substr(arraysize(kPacURLPrefix) - 1),
                            pac_script);
}

}  // namespace

// The Data Reduction Proxy has been turned into a "best effort" proxy,
// meaning it is used only if the effective proxy configuration resolves to
// DIRECT for a URL. It no longer can be a ProxyConfig in the proxy preference
// hierarchy. This method removes the Data Reduction Proxy configuration from
// prefs, if present. |proxy_pref_name| is the name of the proxy pref.
void DataReductionProxyChromeSettings::MigrateDataReductionProxyOffProxyPrefs(
    PrefService* prefs) {
  ProxyPrefMigrationResult proxy_pref_status =
      MigrateDataReductionProxyOffProxyPrefsHelper(prefs);
  UMA_HISTOGRAM_ENUMERATION("DataReductionProxy.ProxyPrefMigrationResult",
                            proxy_pref_status,
                            DataReductionProxyChromeSettings::PROXY_PREF_MAX);
}

DataReductionProxyChromeSettings::ProxyPrefMigrationResult
DataReductionProxyChromeSettings::MigrateDataReductionProxyOffProxyPrefsHelper(
    PrefService* prefs) {
  base::DictionaryValue* dict = (base::DictionaryValue*)prefs->GetUserPrefValue(
      proxy_config::prefs::kProxy);
  if (!dict)
    return PROXY_PREF_NOT_CLEARED;

  // Clear empty "proxy" dictionary created by a bug. See http://crbug/448172.
  if (dict->empty()) {
    prefs->ClearPref(proxy_config::prefs::kProxy);
    return PROXY_PREF_CLEARED_EMPTY;
  }

  std::string mode;
  if (!dict->GetString("mode", &mode))
    return PROXY_PREF_NOT_CLEARED;
  // Clear "system" proxy entry since this is the default. This entry was
  // created by bug (http://crbug/448172).
  if (ProxyModeToString(ProxyPrefs::MODE_SYSTEM) == mode) {
    prefs->ClearPref(proxy_config::prefs::kProxy);
    return PROXY_PREF_CLEARED_MODE_SYSTEM;
  }

  // From M36 to M40, the DRP was configured using MODE_FIXED_SERVERS in the
  // proxy pref.
  if (ProxyModeToString(ProxyPrefs::MODE_FIXED_SERVERS) == mode) {
    std::string proxy_server;
    if (!dict->GetString("server", &proxy_server))
      return PROXY_PREF_NOT_CLEARED;
    net::ProxyConfig::ProxyRules proxy_rules;
    proxy_rules.ParseFromString(proxy_server);
    // Clear the proxy pref if it matches a currently configured Data Reduction
    // Proxy, or if the proxy host ends with ".googlezip.net", in order to
    // ensure that any DRP in the pref is cleared even if the DRP configuration
    // was changed. See http://crbug.com/476610.
    ProxyPrefMigrationResult rv;
    if (Config()->ContainsDataReductionProxy(proxy_rules))
      rv = PROXY_PREF_CLEARED_DRP;
    else if (ContainsDataReductionProxyDefaultHostSuffix(proxy_rules))
      rv = PROXY_PREF_CLEARED_GOOGLEZIP;
    else
      return PROXY_PREF_NOT_CLEARED;

    prefs->ClearPref(proxy_config::prefs::kProxy);
    return rv;
  }

  // Before M35, the DRP was configured using a PAC script base64 encoded into a
  // PAC url.
  if (ProxyModeToString(ProxyPrefs::MODE_PAC_SCRIPT) == mode) {
    std::string pac_url;
    std::string pac_script;
    if (!dict->GetString("pac_url", &pac_url) ||
        !GetEmbeddedPacScript(pac_url, &pac_script)) {
      return PROXY_PREF_NOT_CLEARED;
    }

    // In M35 and earlier, the way of specifying the DRP in a PAC script would
    // always include the port number after the host even if the port number
    // could be implied, so searching for ".googlezip.net:" in the PAC script
    // indicates whether there's a proxy in that PAC script with a host of the
    // form "*.googlezip.net".
    if (pac_script.find(".googlezip.net:") == std::string::npos)
      return PROXY_PREF_NOT_CLEARED;

    prefs->ClearPref(proxy_config::prefs::kProxy);
    return PROXY_PREF_CLEARED_PAC_GOOGLEZIP;
  }

  return PROXY_PREF_NOT_CLEARED;
}

DataReductionProxyChromeSettings::DataReductionProxyChromeSettings()
    : data_reduction_proxy::DataReductionProxySettings(),
      data_reduction_proxy_enabled_pref_name_(prefs::kDataSaverEnabled) {
}

DataReductionProxyChromeSettings::~DataReductionProxyChromeSettings() {
}

void DataReductionProxyChromeSettings::Shutdown() {
  data_reduction_proxy::DataReductionProxyService* service =
      data_reduction_proxy_service();
  if (service)
    service->Shutdown();
}

void DataReductionProxyChromeSettings::InitDataReductionProxySettings(
    data_reduction_proxy::DataReductionProxyIOData* io_data,
    PrefService* profile_prefs,
    net::URLRequestContextGetter* request_context_getter,
    std::unique_ptr<data_reduction_proxy::DataStore> store,
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SequencedTaskRunner>& db_task_runner) {
#if defined(OS_ANDROID)
  // On mobile we write Data Reduction Proxy prefs directly to the pref service.
  // On desktop we store Data Reduction Proxy prefs in memory, writing to disk
  // every 60 minutes and on termination. Shutdown hooks must be added for
  // Android and iOS in order for non-zero delays to be supported.
  // (http://crbug.com/408264)
  base::TimeDelta commit_delay = base::TimeDelta();
#else
  base::TimeDelta commit_delay = base::TimeDelta::FromMinutes(60);
#endif

  std::unique_ptr<data_reduction_proxy::DataReductionProxyService> service =
      std::make_unique<data_reduction_proxy::DataReductionProxyService>(
          this, profile_prefs, request_context_getter, std::move(store),
          std::make_unique<
              data_reduction_proxy::DataReductionProxyPingbackClientImpl>(
              request_context_getter, ui_task_runner),
          ui_task_runner, io_data->io_task_runner(), db_task_runner,
          commit_delay);
  data_reduction_proxy::DataReductionProxySettings::
      InitDataReductionProxySettings(data_reduction_proxy_enabled_pref_name_,
                                     profile_prefs, io_data,
                                     std::move(service));
  io_data->SetDataReductionProxyService(
      data_reduction_proxy_service()->GetWeakPtr());

  data_reduction_proxy::DataReductionProxySettings::
      SetCallbackToRegisterSyntheticFieldTrial(
          base::Bind(
              &ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial));
  // TODO(bengr): Remove after M46. See http://crbug.com/445599.
  MigrateDataReductionProxyOffProxyPrefs(profile_prefs);
}

// static
data_reduction_proxy::Client DataReductionProxyChromeSettings::GetClient() {
#if defined(OS_ANDROID)
  return data_reduction_proxy::Client::CHROME_ANDROID;
#elif defined(OS_MACOSX)
  return data_reduction_proxy::Client::CHROME_MAC;
#elif defined(OS_CHROMEOS)
  return data_reduction_proxy::Client::CHROME_CHROMEOS;
#elif defined(OS_LINUX)
  return data_reduction_proxy::Client::CHROME_LINUX;
#elif defined(OS_WIN)
  return data_reduction_proxy::Client::CHROME_WINDOWS;
#elif defined(OS_FREEBSD)
  return data_reduction_proxy::Client::CHROME_FREEBSD;
#elif defined(OS_OPENBSD)
  return data_reduction_proxy::Client::CHROME_OPENBSD;
#elif defined(OS_SOLARIS)
  return data_reduction_proxy::Client::CHROME_SOLARIS;
#elif defined(OS_QNX)
  return data_reduction_proxy::Client::CHROME_QNX;
#else
  return data_reduction_proxy::Client::UNKNOWN;
#endif
}
