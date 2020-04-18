// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/wifi_util_win/wifi_credentials_getter.h"

#include "components/wifi/wifi_service.h"

WiFiCredentialsGetter::WiFiCredentialsGetter(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

WiFiCredentialsGetter::~WiFiCredentialsGetter() = default;

void WiFiCredentialsGetter::GetWiFiCredentials(
    const std::string& ssid,
    GetWiFiCredentialsCallback callback) {
  if (ssid == kWiFiTestNetwork) {
    // test-mode: return the ssid in key_data.
    std::move(callback).Run(true, ssid);
    return;
  }

  std::unique_ptr<wifi::WiFiService> wifi_service(wifi::WiFiService::Create());
  wifi_service->Initialize(nullptr);

  std::string key_data;
  std::string error;
  wifi_service->GetKeyFromSystem(ssid, &key_data, &error);

  const bool success = error.empty();
  if (!success)
    key_data.clear();

  std::move(callback).Run(success, key_data);
}
