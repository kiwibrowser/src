// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PROXY_CONFIG_PROXY_CONFIG_DICTIONARY_H_
#define COMPONENTS_PROXY_CONFIG_PROXY_CONFIG_DICTIONARY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/proxy_config/proxy_config_export.h"
#include "components/proxy_config/proxy_prefs.h"

namespace base {
class DictionaryValue;
}

namespace net {
class ProxyServer;
}

// Factory and wrapper for proxy config dictionaries that are stored
// in the user preferences. The dictionary has the following structure:
// {
//   mode: string,
//   server: string,
//   pac_url: string,
//   bypass_list: string
// }
// See proxy_config_dictionary.cc for the structure of the respective strings.
class PROXY_CONFIG_EXPORT ProxyConfigDictionary {
 public:
  explicit ProxyConfigDictionary(std::unique_ptr<base::DictionaryValue> dict);
  ~ProxyConfigDictionary();

  bool GetMode(ProxyPrefs::ProxyMode* out) const;
  bool GetPacUrl(std::string* out) const;
  bool GetPacMandatory(bool* out) const;
  bool GetProxyServer(std::string* out) const;
  bool GetBypassList(std::string* out) const;
  bool HasBypassList() const;

  const base::DictionaryValue& GetDictionary() const;

  static std::unique_ptr<base::DictionaryValue> CreateDirect();
  static std::unique_ptr<base::DictionaryValue> CreateAutoDetect();
  static std::unique_ptr<base::DictionaryValue> CreatePacScript(
      const std::string& pac_url,
      bool pac_mandatory);
  static std::unique_ptr<base::DictionaryValue> CreateFixedServers(
      const std::string& proxy_server,
      const std::string& bypass_list);
  static std::unique_ptr<base::DictionaryValue> CreateSystem();

  // Encodes the proxy server as "<url-scheme>=<proxy-scheme>://<proxy>".
  // Used to generate the |proxy_server| arg for CreateFixedServers().
  static void EncodeAndAppendProxyServer(const std::string& url_scheme,
                                         const net::ProxyServer& server,
                                         std::string* spec);

 private:
  static std::unique_ptr<base::DictionaryValue> CreateDictionary(
      ProxyPrefs::ProxyMode mode,
      const std::string& pac_url,
      bool pac_mandatory,
      const std::string& proxy_server,
      const std::string& bypass_list);

  std::unique_ptr<base::DictionaryValue> dict_;

  DISALLOW_COPY_AND_ASSIGN(ProxyConfigDictionary);
};

#endif  // COMPONENTS_PROXY_CONFIG_PROXY_CONFIG_DICTIONARY_H_
