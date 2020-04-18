// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/http_server_properties_pref_delegate.h"

#include "base/bind.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

const char kPrefPath[] = "net.http_server_properties";

namespace network {

HttpServerPropertiesPrefDelegate::HttpServerPropertiesPrefDelegate(
    PrefService* pref_service)
    : pref_service_(pref_service) {
  pref_change_registrar_.Init(pref_service_);
}

HttpServerPropertiesPrefDelegate::~HttpServerPropertiesPrefDelegate() {}

void HttpServerPropertiesPrefDelegate::RegisterPrefs(
    PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterDictionaryPref(kPrefPath);
}

const base::DictionaryValue*
HttpServerPropertiesPrefDelegate::GetServerProperties() const {
  return pref_service_->GetDictionary(kPrefPath);
}

void HttpServerPropertiesPrefDelegate::SetServerProperties(
    const base::DictionaryValue& value,
    base::OnceClosure callback) {
  pref_service_->Set(kPrefPath, value);
  if (callback)
    pref_service_->CommitPendingWrite(std::move(callback));
}

void HttpServerPropertiesPrefDelegate::StartListeningForUpdates(
    const base::RepeatingClosure& callback) {
  pref_change_registrar_.Add(kPrefPath, callback);
  // PrefChangeRegistrar isn't notified of initial pref load, so watch for that,
  // too.
  if (pref_service_->GetInitializationStatus() ==
      PrefService::INITIALIZATION_STATUS_WAITING) {
    pref_service_->AddPrefInitObserver(base::BindOnce(
        [](const base::Closure& callback, bool) { callback.Run(); }, callback));
  }
}

}  // namespace network
