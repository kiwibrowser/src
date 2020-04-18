// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_credential.h"

#include <memory>

#include "base/logging.h"
#include "base/values.h"
#include "components/onc/onc_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_wifi {

namespace {

const char kSsid[] = "fake-ssid";
const char kSsidNonUtf8[] = "\xc0";
const char kPassphraseWep[] = "abcde";
const char kPassphraseWepNonUtf8[] = "\xc0\xc0\xc0\xc0\xc0";
const char kPassphrasePsk[] = "fake-psk-passphrase";
const char kPassphrase8021X[] = "fake-8021X-passphrase";

WifiCredential MakeCredential(const std::string& ssid,
                              WifiSecurityClass security_class,
                              const std::string& passphrase) {
  std::unique_ptr<WifiCredential> credential = WifiCredential::Create(
      WifiCredential::MakeSsidBytesForTest(ssid), security_class, passphrase);
  CHECK(credential);
  return *credential;
}

bool TypeIsWifi(const base::DictionaryValue& onc_properties) {
  std::string network_type;
  EXPECT_TRUE(
      onc_properties.GetString(onc::toplevel_config::kType, &network_type));
  return network_type == onc::network_type::kWiFi;
}

std::string GetSsid(const base::DictionaryValue& onc_properties) {
  std::string ssid;
  EXPECT_TRUE(onc_properties.GetString(
      onc::network_config::WifiProperty(onc::wifi::kSSID), &ssid));
  return ssid;
}

std::string GetOncSecurity(const base::DictionaryValue& onc_properties) {
  std::string onc_security;
  EXPECT_TRUE(onc_properties.GetString(
      onc::network_config::WifiProperty(onc::wifi::kSecurity), &onc_security));
  return onc_security;
}

std::string GetPassphrase(const base::DictionaryValue& onc_properties) {
  std::string passphrase;
  EXPECT_TRUE(onc_properties.GetString(
      onc::network_config::WifiProperty(onc::wifi::kPassphrase), &passphrase));
  return passphrase;
}

}  // namespace

TEST(WifiCredentialTest, CreateWithSecurityClassInvalid) {
  std::unique_ptr<WifiCredential> credential = WifiCredential::Create(
      WifiCredential::MakeSsidBytesForTest(kSsid), SECURITY_CLASS_INVALID, "");
  EXPECT_FALSE(credential);
}

TEST(WifiCredentialTest, CreateWithPassphraseNonUtf8) {
  std::unique_ptr<WifiCredential> credential =
      WifiCredential::Create(WifiCredential::MakeSsidBytesForTest(kSsid),
                             SECURITY_CLASS_WEP, kPassphraseWepNonUtf8);
  EXPECT_FALSE(credential);
}

TEST(WifiCredentialTest, ToOncPropertiesSecurityNone) {
  const WifiCredential credential(
      MakeCredential(kSsid, SECURITY_CLASS_NONE, ""));
  std::unique_ptr<base::DictionaryValue> onc_properties =
      credential.ToOncProperties();
  ASSERT_TRUE(onc_properties);
  EXPECT_TRUE(TypeIsWifi(*onc_properties));
  EXPECT_EQ(kSsid, GetSsid(*onc_properties));
  EXPECT_EQ(onc::wifi::kSecurityNone, GetOncSecurity(*onc_properties));
}

TEST(WifiCredentialTest, ToOncPropertiesSecurityWep) {
  const WifiCredential credential(
      MakeCredential(kSsid, SECURITY_CLASS_WEP, kPassphraseWep));
  std::unique_ptr<base::DictionaryValue> onc_properties =
      credential.ToOncProperties();
  ASSERT_TRUE(onc_properties);
  EXPECT_TRUE(TypeIsWifi(*onc_properties));
  EXPECT_EQ(kSsid, GetSsid(*onc_properties));
  EXPECT_EQ(onc::wifi::kWEP_PSK, GetOncSecurity(*onc_properties));
  EXPECT_EQ(kPassphraseWep, GetPassphrase(*onc_properties));
}

TEST(WifiCredentialTest, ToOncPropertiesSecurityPsk) {
  const WifiCredential credential(
      MakeCredential(kSsid, SECURITY_CLASS_PSK, kPassphrasePsk));
  std::unique_ptr<base::DictionaryValue> onc_properties =
      credential.ToOncProperties();
  ASSERT_TRUE(onc_properties);
  EXPECT_TRUE(TypeIsWifi(*onc_properties));
  EXPECT_EQ(kSsid, GetSsid(*onc_properties));
  EXPECT_EQ(onc::wifi::kWPA_PSK, GetOncSecurity(*onc_properties));
  EXPECT_EQ(kPassphrasePsk, GetPassphrase(*onc_properties));
}

TEST(WifiCredentialTest, ToOncPropertiesSecurity8021X) {
  const WifiCredential credential(
      MakeCredential(kSsid, SECURITY_CLASS_802_1X, kPassphrase8021X));
  std::unique_ptr<base::DictionaryValue> onc_properties =
      credential.ToOncProperties();
  ASSERT_TRUE(onc_properties);
  EXPECT_TRUE(TypeIsWifi(*onc_properties));
  EXPECT_EQ(kSsid, GetSsid(*onc_properties));
  EXPECT_EQ(onc::wifi::kWPA_EAP, GetOncSecurity(*onc_properties));
  EXPECT_EQ(kPassphrase8021X, GetPassphrase(*onc_properties));
}

// TODO(quiche): Update this test, once ONC suports non-UTF-8 SSIDs.
// crbug.com/432546.
TEST(WifiCredentialTest, ToOncPropertiesSsidNonUtf8) {
  const WifiCredential credential(
      MakeCredential(kSsidNonUtf8, SECURITY_CLASS_NONE, ""));
  std::unique_ptr<base::DictionaryValue> onc_properties =
      credential.ToOncProperties();
  EXPECT_FALSE(onc_properties);
}

}  // namespace sync_wifi
