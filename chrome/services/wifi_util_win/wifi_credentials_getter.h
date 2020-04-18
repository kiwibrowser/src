// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_WIFI_UTIL_WIN_WIFI_CREDENTIALS_GETTER_H_
#define CHROME_SERVICES_WIFI_UTIL_WIN_WIFI_CREDENTIALS_GETTER_H_

#include <memory>

#include "chrome/services/wifi_util_win/public/mojom/wifi_credentials_getter.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

class WiFiCredentialsGetter : public chrome::mojom::WiFiCredentialsGetter {
 public:
  explicit WiFiCredentialsGetter(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~WiFiCredentialsGetter() override;

 private:
  // chrome::mojom::WiFiCredentialsGetter:
  void GetWiFiCredentials(const std::string& ssid,
                          GetWiFiCredentialsCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(WiFiCredentialsGetter);
};

#endif  // CHROME_SERVICES_WIFI_UTIL_WIN_WIFI_CREDENTIALS_GETTER_H_
