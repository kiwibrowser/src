// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/proxy_cros_settings_parser.h"

#include <stdint.h>

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "chromeos/network/proxy/ui_proxy_config.h"
#include "chromeos/network/proxy/ui_proxy_config_service.h"

namespace chromeos {

namespace {

std::unique_ptr<base::Value> CreateServerHostValue(
    const UIProxyConfig::ManualProxy& proxy) {
  return proxy.server.is_valid() ? std::make_unique<base::Value>(
                                       proxy.server.host_port_pair().host())
                                 : nullptr;
}

std::unique_ptr<base::Value> CreateServerPortValue(
    const UIProxyConfig::ManualProxy& proxy) {
  return proxy.server.is_valid() ? std::make_unique<base::Value>(
                                       proxy.server.host_port_pair().port())
                                 : nullptr;
}

net::ProxyServer CreateProxyServer(std::string host,
                                   uint16_t port,
                                   net::ProxyServer::Scheme scheme) {
  if (host.empty() && port == 0)
    return net::ProxyServer();
  uint16_t default_port = net::ProxyServer::GetDefaultPortForScheme(scheme);
  net::HostPortPair host_port_pair;
  // Check if host is a valid URL or a string of valid format <server>::<port>.
  GURL url(host);
  if (url.is_valid())  // See if host is URL.
    host_port_pair = net::HostPortPair::FromURL(url);
  if (host_port_pair.host().empty())  // See if host is <server>::<port>.
    host_port_pair = net::HostPortPair::FromString(host);
  if (host_port_pair.host().empty())  // Host is not URL or <server>::<port>.
    host_port_pair = net::HostPortPair(host, port);
  if (host_port_pair.port() == 0)  // No port in host, use default.
    host_port_pair.set_port(default_port);
  return net::ProxyServer(scheme, host_port_pair);
}

net::ProxyServer CreateProxyServerFromHost(
    const std::string& host,
    const UIProxyConfig::ManualProxy& proxy,
    net::ProxyServer::Scheme scheme) {
  uint16_t port = 0;
  if (proxy.server.is_valid())
    port = proxy.server.host_port_pair().port();
  return CreateProxyServer(host, port, scheme);
}

net::ProxyServer CreateProxyServerFromPort(
    uint16_t port,
    const UIProxyConfig::ManualProxy& proxy,
    net::ProxyServer::Scheme scheme) {
  std::string host;
  if (proxy.server.is_valid())
    host = proxy.server.host_port_pair().host();
  return CreateProxyServer(host, port, scheme);
}

}  // namespace

namespace proxy_cros_settings_parser {

// Common prefix of all proxy prefs.
const char kProxyPrefsPrefix[] = "cros.session.proxy";

// Names of proxy preferences.
const char kProxyPacUrl[] = "cros.session.proxy.pacurl";
const char kProxySingleHttp[] = "cros.session.proxy.singlehttp";
const char kProxySingleHttpPort[] = "cros.session.proxy.singlehttpport";
const char kProxyHttpUrl[] = "cros.session.proxy.httpurl";
const char kProxyHttpPort[] = "cros.session.proxy.httpport";
const char kProxyHttpsUrl[] = "cros.session.proxy.httpsurl";
const char kProxyHttpsPort[] = "cros.session.proxy.httpsport";
const char kProxyType[] = "cros.session.proxy.type";
const char kProxySingle[] = "cros.session.proxy.single";
const char kProxyFtpUrl[] = "cros.session.proxy.ftpurl";
const char kProxyFtpPort[] = "cros.session.proxy.ftpport";
const char kProxySocks[] = "cros.session.proxy.socks";
const char kProxySocksPort[] = "cros.session.proxy.socksport";
const char kProxyIgnoreList[] = "cros.session.proxy.ignorelist";
const char kProxyUsePacUrl[] = "cros.session.proxy.usepacurl";

const char* const kProxySettings[] = {
    kProxyPacUrl,    kProxySingleHttp, kProxySingleHttpPort, kProxyHttpUrl,
    kProxyHttpPort,  kProxyHttpsUrl,   kProxyHttpsPort,      kProxyType,
    kProxySingle,    kProxyFtpUrl,     kProxyFtpPort,        kProxySocks,
    kProxySocksPort, kProxyIgnoreList, kProxyUsePacUrl,
};

// We have to explicitly export this because the arraysize macro doesn't like
// extern arrays as their size is not known on compile time.
const size_t kProxySettingsCount = arraysize(kProxySettings);

bool IsProxyPref(const std::string& path) {
  return base::StartsWith(path, kProxyPrefsPrefix,
                          base::CompareCase::SENSITIVE);
}

void SetProxyPrefValue(const std::string& network_guid,
                       const std::string& path,
                       const base::Value* in_value,
                       UIProxyConfigService* config_service) {
  if (!in_value) {
    NOTREACHED();
    return;
  }

  // Retrieve proxy config.
  UIProxyConfig config;
  config_service->GetProxyConfig(network_guid, &config);

  if (path == kProxyPacUrl) {
    std::string val;
    if (in_value->GetAsString(&val)) {
      GURL url(val);
      if (url.is_valid())
        config.SetPacUrl(url);
      else
        config.mode = UIProxyConfig::MODE_AUTO_DETECT;
    }
  } else if (path == kProxySingleHttp) {
    std::string val;
    if (in_value->GetAsString(&val)) {
      config.SetSingleProxy(CreateProxyServerFromHost(
          val, config.single_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxySingleHttpPort) {
    int val;
    if (in_value->GetAsInteger(&val)) {
      config.SetSingleProxy(CreateProxyServerFromPort(
          val, config.single_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxyHttpUrl) {
    std::string val;
    if (in_value->GetAsString(&val)) {
      config.SetProxyForScheme(
          "http", CreateProxyServerFromHost(
              val, config.http_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxyHttpPort) {
    int val;
    if (in_value->GetAsInteger(&val)) {
      config.SetProxyForScheme(
          "http", CreateProxyServerFromPort(
              val, config.http_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxyHttpsUrl) {
    std::string val;
    if (in_value->GetAsString(&val)) {
      config.SetProxyForScheme(
          "https", CreateProxyServerFromHost(
              val, config.https_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxyHttpsPort) {
    int val;
    if (in_value->GetAsInteger(&val)) {
      config.SetProxyForScheme(
          "https", CreateProxyServerFromPort(
              val, config.https_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxyType) {
    int val;
    if (in_value->GetAsInteger(&val)) {
      if (val == 3) {
        if (config.automatic_proxy.pac_url.is_valid())
          config.SetPacUrl(config.automatic_proxy.pac_url);
        else
          config.mode = UIProxyConfig::MODE_AUTO_DETECT;
      } else if (val == 2) {
        if (config.single_proxy.server.is_valid()) {
          config.SetSingleProxy(config.single_proxy.server);
        } else {
          bool set_config = false;
          if (config.http_proxy.server.is_valid()) {
            config.SetProxyForScheme("http", config.http_proxy.server);
            set_config = true;
          }
          if (config.https_proxy.server.is_valid()) {
            config.SetProxyForScheme("https", config.https_proxy.server);
            set_config = true;
          }
          if (config.ftp_proxy.server.is_valid()) {
            config.SetProxyForScheme("ftp", config.ftp_proxy.server);
            set_config = true;
          }
          if (config.socks_proxy.server.is_valid()) {
            config.SetProxyForScheme("socks", config.socks_proxy.server);
            set_config = true;
          }
          if (!set_config)
            config.SetProxyForScheme("http", net::ProxyServer());
        }
      } else {
        config.mode = UIProxyConfig::MODE_DIRECT;
      }
    }
  } else if (path == kProxySingle) {
    bool val;
    if (in_value->GetAsBoolean(&val)) {
      if (val)
        config.SetSingleProxy(config.single_proxy.server);
      else
        config.SetProxyForScheme("http", config.http_proxy.server);
    }
  } else if (path == kProxyUsePacUrl) {
    bool use_pac_url;
    if (in_value->GetAsBoolean(&use_pac_url)) {
      if (use_pac_url && config.automatic_proxy.pac_url.is_valid())
        config.SetPacUrl(config.automatic_proxy.pac_url);
      else
        config.mode = UIProxyConfig::MODE_AUTO_DETECT;
    }
  } else if (path == kProxyFtpUrl) {
    std::string val;
    if (in_value->GetAsString(&val)) {
      config.SetProxyForScheme(
          "ftp", CreateProxyServerFromHost(
              val, config.ftp_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxyFtpPort) {
    int val;
    if (in_value->GetAsInteger(&val)) {
      config.SetProxyForScheme(
          "ftp", CreateProxyServerFromPort(
              val, config.ftp_proxy, net::ProxyServer::SCHEME_HTTP));
    }
  } else if (path == kProxySocks) {
    std::string val;
    if (in_value->GetAsString(&val)) {
      config.SetProxyForScheme(
          "socks", CreateProxyServerFromHost(
                       val, config.socks_proxy,
                       base::StartsWith(val, "socks5://",
                                        base::CompareCase::INSENSITIVE_ASCII)
                           ? net::ProxyServer::SCHEME_SOCKS5
                           : net::ProxyServer::SCHEME_SOCKS4));
    }
  } else if (path == kProxySocksPort) {
    int val;
    if (in_value->GetAsInteger(&val)) {
      std::string host = config.socks_proxy.server.host_port_pair().host();
      config.SetProxyForScheme(
          "socks", CreateProxyServerFromPort(
                       val, config.socks_proxy,
                       base::StartsWith(host, "socks5://",
                                        base::CompareCase::INSENSITIVE_ASCII)
                           ? net::ProxyServer::SCHEME_SOCKS5
                           : net::ProxyServer::SCHEME_SOCKS4));
    }
  } else if (path == kProxyIgnoreList) {
    net::ProxyBypassRules bypass_rules;
    if (in_value->is_list()) {
      const base::ListValue* list_value =
          static_cast<const base::ListValue*>(in_value);
      for (size_t x = 0; x < list_value->GetSize(); x++) {
        std::string val;
        if (list_value->GetString(x, &val))
          bypass_rules.AddRuleFromString(val);
      }
      config.SetBypassRules(bypass_rules);
    }
  } else {
    LOG(WARNING) << "Unknown proxy settings path " << path;
    return;
  }

  config_service->SetProxyConfig(network_guid, config);
}

bool GetProxyPrefValue(const std::string& network_guid,
                       const std::string& path,
                       UIProxyConfigService* config_service,
                       std::unique_ptr<base::Value>* out_value) {
  std::string controlled_by;
  std::unique_ptr<base::Value> data;
  UIProxyConfig config;
  config_service->GetProxyConfig(network_guid, &config);

  if (path == kProxyPacUrl) {
    // Only show pacurl for pac-script mode.
    if (config.mode == UIProxyConfig::MODE_PAC_SCRIPT &&
        config.automatic_proxy.pac_url.is_valid()) {
      data =
          std::make_unique<base::Value>(config.automatic_proxy.pac_url.spec());
    }
  } else if (path == kProxySingleHttp) {
    data = CreateServerHostValue(config.single_proxy);
  } else if (path == kProxySingleHttpPort) {
    data = CreateServerPortValue(config.single_proxy);
  } else if (path == kProxyHttpUrl) {
    data = CreateServerHostValue(config.http_proxy);
  } else if (path == kProxyHttpsUrl) {
    data = CreateServerHostValue(config.https_proxy);
  } else if (path == kProxyType) {
    if (config.mode == UIProxyConfig::MODE_AUTO_DETECT ||
        config.mode == UIProxyConfig::MODE_PAC_SCRIPT) {
      data = std::make_unique<base::Value>(3);
    } else if (config.mode == UIProxyConfig::MODE_SINGLE_PROXY ||
               config.mode == UIProxyConfig::MODE_PROXY_PER_SCHEME) {
      data = std::make_unique<base::Value>(2);
    } else {
      data = std::make_unique<base::Value>(1);
    }
    switch (config.state) {
      case ProxyPrefs::CONFIG_POLICY:
        controlled_by = "policy";
        break;
      case ProxyPrefs::CONFIG_EXTENSION:
        controlled_by = "extension";
        break;
      case ProxyPrefs::CONFIG_OTHER_PRECEDE:
        controlled_by = "other";
        break;
      default:
        if (!config.user_modifiable)
          controlled_by = "shared";
        break;
    }
  } else if (path == kProxySingle) {
    data = std::make_unique<base::Value>(config.mode ==
                                         UIProxyConfig::MODE_SINGLE_PROXY);
  } else if (path == kProxyUsePacUrl) {
    data = std::make_unique<base::Value>(config.mode ==
                                         UIProxyConfig::MODE_PAC_SCRIPT);
  } else if (path == kProxyFtpUrl) {
    data = CreateServerHostValue(config.ftp_proxy);
  } else if (path == kProxySocks) {
    data = CreateServerHostValue(config.socks_proxy);
  } else if (path == kProxyHttpPort) {
    data = CreateServerPortValue(config.http_proxy);
  } else if (path == kProxyHttpsPort) {
    data = CreateServerPortValue(config.https_proxy);
  } else if (path == kProxyFtpPort) {
    data = CreateServerPortValue(config.ftp_proxy);
  } else if (path == kProxySocksPort) {
    data = CreateServerPortValue(config.socks_proxy);
  } else if (path == kProxyIgnoreList) {
    auto list = std::make_unique<base::ListValue>();
    const auto& bypass_rules = config.bypass_rules.rules();
    for (const auto& rule : bypass_rules)
      list->AppendString(rule->ToString());
    data = std::move(list);
  } else {
    out_value->reset();
    return false;
  }

  // Decorate pref value as CoreOptionsHandler::CreateValueForPref() does.
  auto dict = std::make_unique<base::DictionaryValue>();
  if (!data)
    data = std::make_unique<base::Value>(base::Value::Type::STRING);
  dict->Set("value", std::move(data));
  if (path == kProxyType) {
    if (!controlled_by.empty())
      dict->SetString("controlledBy", controlled_by);
    dict->SetBoolean("disabled", !config.user_modifiable);
  } else {
    dict->SetBoolean("disabled", false);
  }
  *out_value = std::move(dict);
  return true;
}

}  // namespace proxy_cros_settings_parser

}  // namespace chromeos
