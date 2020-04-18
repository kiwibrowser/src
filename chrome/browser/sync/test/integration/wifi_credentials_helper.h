// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_TEST_INTEGRATION_WIFI_CREDENTIALS_HELPER_H_
#define CHROME_BROWSER_SYNC_TEST_INTEGRATION_WIFI_CREDENTIALS_HELPER_H_

#include <memory>
#include <string>

#include "components/sync_wifi/wifi_credential.h"
#include "components/sync_wifi/wifi_security_class.h"

class Profile;

// Functions needed by multiple wifi_credentials integration
// tests. This module is platfrom-agnostic, and calls out to
// platform-specific code as needed.
namespace wifi_credentials_helper {

// Performs common setup steps, such as configuring factories. Should
// be called before SyncTest::SetUp.
void SetUp();

// Initializes the clients. This includes associating their Chrome
// Profiles with platform-specific networking state. Should be called
// before adding/removing/modifying WiFi credentials.
void SetupClients();

// Checks if the verifier has any items in it. Returns true iff the
// verifier has no items.
bool VerifierIsEmpty();

// Compares the BrowserContext for |profile_index| with the
// verifier. Returns true iff their WiFi credentials match.
bool ProfileMatchesVerifier(int profile_index);

// Returns true iff all BrowserContexts match with the verifier.
bool AllProfilesMatch();

// Returns a new WifiCredential constructed from the given parameters.
std::unique_ptr<sync_wifi::WifiCredential> MakeWifiCredential(
    const std::string& ssid,
    sync_wifi::WifiSecurityClass security_class,
    const std::string& passphrase);

// Adds a WiFi credential to the service at index |profile_index|,
// and the verifier (if the SyncTest uses a verifier).
void AddWifiCredential(int profile_index,
                       const std::string& sync_id,
                       const sync_wifi::WifiCredential& credential);

// Returns the set of WifiCredentials configured in local network
// settings, for |profile|.
sync_wifi::WifiCredential::CredentialSet GetWifiCredentialsForProfile(
    const Profile* profile);

}  // namespace wifi_credentials_helper

#endif  // CHROME_BROWSER_SYNC_TEST_INTEGRATION_WIFI_CREDENTIALS_HELPER_H_
