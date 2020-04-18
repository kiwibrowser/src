// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_TEST_INTEGRATION_WIFI_CREDENTIALS_HELPER_CHROMEOS_H_
#define CHROME_BROWSER_SYNC_TEST_INTEGRATION_WIFI_CREDENTIALS_HELPER_CHROMEOS_H_

#include "components/sync_wifi/wifi_credential.h"

namespace content {
class BrowserContext;
}

namespace wifi_credentials_helper {

namespace chromeos {

// Performs ChromeOS-specific setup.
void SetUpChromeOs();

// Performs ChromeOS-specific setup for a given sync client, given
// that client's BrowserContext. Should be called only after SetUpChromeOs.
void SetupClientForProfileChromeOs(
    const content::BrowserContext* browser_context);

// Adds a WiFi credential to the ChromeOS networking backend,
// associating the credential with the ChromeOS networking state that
// corresponds to |browser_context|.
void AddWifiCredentialToProfileChromeOs(
    const content::BrowserContext* browser_context,
    const sync_wifi::WifiCredential& credential);

// Returns the ChromeOS WiFi credentials associated with |browser_context|.
sync_wifi::WifiCredential::CredentialSet GetWifiCredentialsForProfileChromeOs(
    const content::BrowserContext* profile);

}  // namespace chromeos

}  // namespace wifi_credentials_helper

#endif  // CHROME_BROWSER_SYNC_TEST_INTEGRATION_WIFI_CREDENTIALS_HELPER_CHROMEOS_H_
