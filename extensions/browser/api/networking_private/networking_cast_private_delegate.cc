// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_private/networking_cast_private_delegate.h"

#include <utility>

#include "base/strings/string_util.h"

namespace extensions {

NetworkingCastPrivateDelegate::Credentials::Credentials(
    const std::string& certificate,
    const std::vector<std::string>& intermediate_certificates,
    const std::string& signed_data,
    const std::string& device_ssid,
    const std::string& device_serial,
    const std::string& device_bssid,
    const std::string& public_key,
    const std::string& nonce)
    : certificate_(certificate),
      intermediate_certificates_(intermediate_certificates),
      signed_data_(signed_data),
      device_bssid_(device_bssid),
      public_key_(public_key) {
  unsigned_data_ = base::JoinString(
      {device_ssid, device_serial, device_bssid, public_key, nonce}, ",");
}

NetworkingCastPrivateDelegate::Credentials::~Credentials() {}

}  // namespace extensions
