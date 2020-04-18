// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proxy_config/proxy_config_dictionary.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "net/proxy_resolution/proxy_config.h"

namespace {

// Integer to specify the type of proxy settings.
// See ProxyPrefs for possible values and interactions with the other proxy
// preferences.
const char kProxyMode[] = "mode";
// String specifying the proxy server. For a specification of the expected
// syntax see net::ProxyConfig::ProxyRules::ParseFromString().
const char kProxyServer[] = "server";
// URL to the proxy .pac file.
const char kProxyPacUrl[] = "pac_url";
// Optional boolean flag indicating whether a valid PAC script is mandatory.
// If true, network traffic does not fall back to direct connections in case the
// PAC script is not available.
const char kProxyPacMandatory[] = "pac_mandatory";
// String containing proxy bypass rules. For a specification of the
// expected syntax see net::ProxyBypassRules::ParseFromString().
const char kProxyBypassList[] = "bypass_list";

}  // namespace

ProxyConfigDictionary::ProxyConfigDictionary(
    std::unique_ptr<base::DictionaryValue> dict)
    : dict_(std::move(dict)) {}

ProxyConfigDictionary::~ProxyConfigDictionary() {}

bool ProxyConfigDictionary::GetMode(ProxyPrefs::ProxyMode* out) const {
  std::string mode_str;
  return dict_->GetString(kProxyMode, &mode_str)
      && StringToProxyMode(mode_str, out);
}

bool ProxyConfigDictionary::GetPacUrl(std::string* out) const {
  return dict_->GetString(kProxyPacUrl, out);
}

bool ProxyConfigDictionary::GetPacMandatory(bool* out) const {
  if (!dict_->HasKey(kProxyPacMandatory)) {
    *out = false;
    return true;
  }
  return dict_->GetBoolean(kProxyPacMandatory, out);
}

bool ProxyConfigDictionary::GetProxyServer(std::string* out) const {
  return dict_->GetString(kProxyServer, out);
}

bool ProxyConfigDictionary::GetBypassList(std::string* out) const {
  return dict_->GetString(kProxyBypassList, out);
}

bool ProxyConfigDictionary::HasBypassList() const {
  return dict_->HasKey(kProxyBypassList);
}

const base::DictionaryValue& ProxyConfigDictionary::GetDictionary() const {
  return *dict_;
}

// static
std::unique_ptr<base::DictionaryValue> ProxyConfigDictionary::CreateDirect() {
  return CreateDictionary(ProxyPrefs::MODE_DIRECT,
                          std::string(),
                          false,
                          std::string(),
                          std::string());
}

// static
std::unique_ptr<base::DictionaryValue>
ProxyConfigDictionary::CreateAutoDetect() {
  return CreateDictionary(ProxyPrefs::MODE_AUTO_DETECT,
                          std::string(),
                          false,
                          std::string(),
                          std::string());
}

// static
std::unique_ptr<base::DictionaryValue> ProxyConfigDictionary::CreatePacScript(
    const std::string& pac_url,
    bool pac_mandatory) {
  return CreateDictionary(ProxyPrefs::MODE_PAC_SCRIPT,
                          pac_url,
                          pac_mandatory,
                          std::string(),
                          std::string());
}

// static
std::unique_ptr<base::DictionaryValue>
ProxyConfigDictionary::CreateFixedServers(const std::string& proxy_server,
                                          const std::string& bypass_list) {
  if (!proxy_server.empty()) {
    return CreateDictionary(ProxyPrefs::MODE_FIXED_SERVERS,
                            std::string(),
                            false,
                            proxy_server,
                            bypass_list);
  } else {
    return CreateDirect();
  }
}

// static
std::unique_ptr<base::DictionaryValue> ProxyConfigDictionary::CreateSystem() {
  return CreateDictionary(ProxyPrefs::MODE_SYSTEM,
                          std::string(),
                          false,
                          std::string(),
                          std::string());
}

// static
std::unique_ptr<base::DictionaryValue> ProxyConfigDictionary::CreateDictionary(
    ProxyPrefs::ProxyMode mode,
    const std::string& pac_url,
    bool pac_mandatory,
    const std::string& proxy_server,
    const std::string& bypass_list) {
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetString(kProxyMode, ProxyModeToString(mode));
  if (!pac_url.empty()) {
    dict->SetString(kProxyPacUrl, pac_url);
    dict->SetBoolean(kProxyPacMandatory, pac_mandatory);
  }
  if (!proxy_server.empty())
    dict->SetString(kProxyServer, proxy_server);
  if (!bypass_list.empty())
    dict->SetString(kProxyBypassList, bypass_list);
  return dict;
}

// static
void ProxyConfigDictionary::EncodeAndAppendProxyServer(
    const std::string& url_scheme,
    const net::ProxyServer& server,
    std::string* spec) {
  if (!server.is_valid())
    return;

  if (!spec->empty())
    *spec += ';';

  if (!url_scheme.empty()) {
    *spec += url_scheme;
    *spec += "=";
  }
  *spec += server.ToURI();
}
