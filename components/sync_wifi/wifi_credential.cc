// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_credential.h"

#include <memory>

#include "base/i18n/streaming_utf8_validator.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "components/onc/onc_constants.h"

namespace sync_wifi {

WifiCredential::WifiCredential(const WifiCredential& other) = default;

WifiCredential::~WifiCredential() {}

// static
std::unique_ptr<WifiCredential> WifiCredential::Create(
    const SsidBytes& ssid,
    WifiSecurityClass security_class,
    const std::string& passphrase) {
  if (security_class == SECURITY_CLASS_INVALID) {
    LOG(ERROR) << "SecurityClass is invalid.";
    return nullptr;
  }

  if (!base::StreamingUtf8Validator::Validate(passphrase)) {
    LOG(ERROR) << "Passphrase is not valid UTF-8";
    return nullptr;
  }

  return base::WrapUnique(new WifiCredential(ssid, security_class, passphrase));
}

std::unique_ptr<base::DictionaryValue> WifiCredential::ToOncProperties() const {
  const std::string ssid_utf8(ssid().begin(), ssid().end());
  // TODO(quiche): Remove this test, once ONC suports non-UTF-8 SSIDs.
  // crbug.com/432546.
  if (!base::StreamingUtf8Validator::Validate(ssid_utf8)) {
    LOG(ERROR) << "SSID is not valid UTF-8";
    return nullptr;
  }

  std::string onc_security;
  if (!WifiSecurityClassToOncSecurityString(security_class(), &onc_security)) {
    NOTREACHED() << "Failed to convert SecurityClass with value "
                 << security_class();
    return std::make_unique<base::DictionaryValue>();
  }

  auto onc_properties = std::make_unique<base::DictionaryValue>();
  onc_properties->SetString(onc::toplevel_config::kType,
                            onc::network_type::kWiFi);
  // TODO(quiche): Switch to the HexSSID property, once ONC fully supports it.
  // crbug.com/432546.
  onc_properties->SetString(onc::network_config::WifiProperty(onc::wifi::kSSID),
                            ssid_utf8);
  onc_properties->SetString(
      onc::network_config::WifiProperty(onc::wifi::kSecurity), onc_security);
  if (WifiSecurityClassSupportsPassphrases(security_class())) {
    onc_properties->SetString(
        onc::network_config::WifiProperty(onc::wifi::kPassphrase),
        passphrase());
  }
  return onc_properties;
}

std::string WifiCredential::ToString() const {
  return base::StringPrintf(
      "[SSID (hex): %s, SecurityClass: %d]",
      base::HexEncode(&ssid_.front(), ssid_.size()).c_str(),
      security_class_);  // Passphrase deliberately omitted.
}

// static
bool WifiCredential::IsLessThan(const WifiCredential& a,
                                const WifiCredential& b) {
  return a.ssid_ < b.ssid_ || a.security_class_ < b.security_class_ ||
         a.passphrase_ < b.passphrase_;
}

// static
WifiCredential::CredentialSet WifiCredential::MakeSet() {
  return CredentialSet(WifiCredential::IsLessThan);
}

// static
WifiCredential::SsidBytes WifiCredential::MakeSsidBytesForTest(
    const std::string& ssid) {
  return SsidBytes(ssid.begin(), ssid.end());
}

// Private methods.

WifiCredential::WifiCredential(const std::vector<unsigned char>& ssid,
                               WifiSecurityClass security_class,
                               const std::string& passphrase)
    : ssid_(ssid), security_class_(security_class), passphrase_(passphrase) {}

}  // namespace sync_wifi
