// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/command_line.h"
#include "base/macros.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"
#include "chrome/browser/sync/test/integration/wifi_credentials_helper.h"
#include "chrome/common/chrome_switches.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/sync_wifi/wifi_credential.h"
#include "components/sync_wifi/wifi_security_class.h"

using sync_wifi::WifiCredential;

using WifiCredentialSet = sync_wifi::WifiCredential::CredentialSet;

class SingleClientWifiCredentialsSyncTest : public SyncTest {
 public:
  SingleClientWifiCredentialsSyncTest() : SyncTest(SINGLE_CLIENT) {}
  ~SingleClientWifiCredentialsSyncTest() override {}

  // SyncTest implementation.
  void SetUp() override {
    wifi_credentials_helper::SetUp();
    SyncTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    SyncTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableWifiCredentialSync);
  }

  bool SetupClients() override {
    if (!SyncTest::SetupClients())
      return false;
    wifi_credentials_helper::SetupClients();
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleClientWifiCredentialsSyncTest);
};

IN_PROC_BROWSER_TEST_F(SingleClientWifiCredentialsSyncTest, NoCredentials) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(wifi_credentials_helper::VerifierIsEmpty());
  ASSERT_TRUE(wifi_credentials_helper::ProfileMatchesVerifier(0));
}

IN_PROC_BROWSER_TEST_F(SingleClientWifiCredentialsSyncTest, SingleCredential) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  const char ssid[] = "fake-ssid";
  std::unique_ptr<WifiCredential> credential =
      wifi_credentials_helper::MakeWifiCredential(
          ssid, sync_wifi::SECURITY_CLASS_PSK, "fake_passphrase");
  ASSERT_TRUE(credential);

  const size_t profile_index = 0;
  wifi_credentials_helper::AddWifiCredential(
      profile_index, "fake_id", *credential);

  const WifiCredentialSet verifier_credentials =
      wifi_credentials_helper::GetWifiCredentialsForProfile(verifier());
  EXPECT_EQ(1U, verifier_credentials.size());
  EXPECT_EQ(WifiCredential::MakeSsidBytesForTest(ssid),
            verifier_credentials.begin()->ssid());

  ASSERT_TRUE(
      UpdatedProgressMarkerChecker(GetSyncService(profile_index)).Wait());
  EXPECT_TRUE(wifi_credentials_helper::ProfileMatchesVerifier(profile_index));
}
