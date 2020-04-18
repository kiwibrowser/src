// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_HTTP_SERVER_PROPERTIES_PREF_DELEGATE_H_
#define SERVICES_NETWORK_HTTP_SERVER_PROPERTIES_PREF_DELEGATE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "components/prefs/pref_change_registrar.h"
#include "net/http/http_server_properties.h"
#include "net/http/http_server_properties_manager.h"

class PrefRegistrySimple;

namespace network {

// Manages disk storage for a net::HttpServerPropertiesManager.
class HttpServerPropertiesPrefDelegate
    : public net::HttpServerPropertiesManager::PrefDelegate {
 public:
  // The created object must be destroyed before |pref_service|.
  explicit HttpServerPropertiesPrefDelegate(PrefService* pref_service);
  ~HttpServerPropertiesPrefDelegate() override;

  static void RegisterPrefs(PrefRegistrySimple* pref_registry);

  // net::HttpServerPropertiesManager::PrefDelegate implementation.
  const base::DictionaryValue* GetServerProperties() const override;
  void SetServerProperties(const base::DictionaryValue& value,
                           base::OnceClosure callback) override;
  void StartListeningForUpdates(
      const base::RepeatingClosure& callback) override;

 private:
  PrefService* pref_service_;
  PrefChangeRegistrar pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(HttpServerPropertiesPrefDelegate);
};

}  // namespace network

#endif  // SERVICES_NETWORK_HTTP_SERVER_PROPERTIES_PREF_DELEGATE_H_
