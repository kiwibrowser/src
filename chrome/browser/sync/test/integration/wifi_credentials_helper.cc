// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/wifi_credentials_helper.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/test/integration/sync_datatype_helper.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/sync_wifi/wifi_credential_syncable_service.h"
#include "components/sync_wifi/wifi_credential_syncable_service_factory.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/sync/test/integration/wifi_credentials_helper_chromeos.h"
#endif

using sync_wifi::WifiCredential;
using sync_wifi::WifiCredentialSyncableService;
using sync_wifi::WifiCredentialSyncableServiceFactory;
using sync_wifi::WifiSecurityClass;
using sync_datatype_helper::test;

using WifiCredentialSet = sync_wifi::WifiCredential::CredentialSet;

namespace wifi_credentials_helper {

namespace {

void SetupClientForProfile(Profile* profile) {
#if defined(OS_CHROMEOS)
  wifi_credentials_helper::chromeos::SetupClientForProfileChromeOs(profile);
#else
  NOTREACHED();
#endif
}

WifiCredentialSyncableService* GetServiceForBrowserContext(
    content::BrowserContext* context) {
  return WifiCredentialSyncableServiceFactory::GetForBrowserContext(
      context);
}

WifiCredentialSyncableService* GetServiceForProfile(int profile_index) {
  return GetServiceForBrowserContext(test()->GetProfile(profile_index));
}

void AddWifiCredentialToProfile(
    Profile* profile, const WifiCredential& credential) {
#if defined(OS_CHROMEOS)
  wifi_credentials_helper::chromeos::AddWifiCredentialToProfileChromeOs(
      profile, credential);
#else
  NOTREACHED();
#endif
}

bool CredentialsMatch(const WifiCredentialSet& a_credentials,
                      const WifiCredentialSet& b_credentials) {
  if (a_credentials.size() != b_credentials.size()) {
    LOG(ERROR) << "CredentialSets a and b do not match in size: "
               << a_credentials.size()
               << " vs " << b_credentials.size() << " respectively.";
    return false;
  }

  for (const WifiCredential& credential : a_credentials) {
    if (b_credentials.find(credential) == b_credentials.end()) {
      LOG(ERROR)
          << "Network from a not found in b. "
          << "SSID (hex): "
          << base::HexEncode(credential.ssid().data(),
                             credential.ssid().size()).c_str()
          << " SecurityClass: " << credential.security_class()
          << " Passphrase: " << credential.passphrase();
      return false;
    }
  }

  return true;
}

}  // namespace

void SetUp() {
#if defined(OS_CHROMEOS)
  wifi_credentials_helper::chromeos::SetUpChromeOs();
#else
  NOTREACHED();
#endif
}

void SetupClients() {
  SetupClientForProfile(test()->verifier());
  for (int i = 0; i < test()->num_clients(); ++i)
    SetupClientForProfile(test()->GetProfile(i));
}

bool VerifierIsEmpty() {
  return GetWifiCredentialsForProfile(test()->verifier()).empty();
}

bool ProfileMatchesVerifier(int profile_index) {
  WifiCredentialSet verifier_credentials =
      GetWifiCredentialsForProfile(test()->verifier());
  WifiCredentialSet other_credentials =
      GetWifiCredentialsForProfile(test()->GetProfile(profile_index));
  return CredentialsMatch(verifier_credentials, other_credentials);
}

bool AllProfilesMatch() {
  if (test()->use_verifier() && !ProfileMatchesVerifier(0)) {
    LOG(ERROR) << "Profile 0 does not match verifier.";
    return false;
  }

  WifiCredentialSet profile0_credentials =
      GetWifiCredentialsForProfile(test()->GetProfile(0));
  for (int i = 1; i < test()->num_clients(); ++i) {
    WifiCredentialSet other_profile_credentials =
        GetWifiCredentialsForProfile(test()->GetProfile(i));
    if (!CredentialsMatch(profile0_credentials, other_profile_credentials)) {
      LOG(ERROR) << "Profile " << i << " " << "does not match with profile 0.";
      return false;
    }
  }
  return true;
}

std::unique_ptr<WifiCredential> MakeWifiCredential(
    const std::string& ssid,
    WifiSecurityClass security_class,
    const std::string& passphrase) {
  return WifiCredential::Create(WifiCredential::MakeSsidBytesForTest(ssid),
                                security_class,
                                passphrase);
}

void AddWifiCredential(int profile_index,
                       const std::string& sync_id,
                       const WifiCredential& credential) {
  AddWifiCredentialToProfile(test()->GetProfile(profile_index), credential);
  if (test()->use_verifier())
    AddWifiCredentialToProfile(test()->verifier(), credential);

  // TODO(quiche): Remove this, once we have plumbing to route
  // NetworkConfigurationObserver events to
  // WifiCredentialSyncableService instances.
  GetServiceForProfile(profile_index)
      ->AddToSyncedNetworks(sync_id, credential);
}

WifiCredentialSet GetWifiCredentialsForProfile(const Profile* profile) {
#if defined(OS_CHROMEOS)
  return wifi_credentials_helper::chromeos::
      GetWifiCredentialsForProfileChromeOs(profile);
#else
  NOTREACHED();
  return WifiCredential::MakeSet();
#endif
}

}  // namespace wifi_credentials_helper
