// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/command_line.h"
#include "base/macros.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/wifi_credentials_helper.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/sync_wifi/wifi_credential.h"
#include "components/sync_wifi/wifi_security_class.h"

using sync_wifi::WifiCredential;

using WifiCredentialSet = sync_wifi::WifiCredential::CredentialSet;

class TwoClientWifiCredentialsSyncTest : public SyncTest {
 public:
  TwoClientWifiCredentialsSyncTest() : SyncTest(TWO_CLIENT) {}
  ~TwoClientWifiCredentialsSyncTest() override {}

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
  DISALLOW_COPY_AND_ASSIGN(TwoClientWifiCredentialsSyncTest);
};

IN_PROC_BROWSER_TEST_F(TwoClientWifiCredentialsSyncTest, NoCredentials) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(wifi_credentials_helper::VerifierIsEmpty());
  ASSERT_TRUE(wifi_credentials_helper::AllProfilesMatch());
}

IN_PROC_BROWSER_TEST_F(TwoClientWifiCredentialsSyncTest, SingleCredential) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  const char ssid[] = "fake-ssid";
  std::unique_ptr<WifiCredential> credential =
      wifi_credentials_helper::MakeWifiCredential(
          ssid, sync_wifi::SECURITY_CLASS_PSK, "fake_passphrase");
  ASSERT_TRUE(credential);

  const size_t profile_a_index = 0;
  wifi_credentials_helper::AddWifiCredential(
      profile_a_index, "fake_id", *credential);

  const WifiCredentialSet verifier_credentials =
      wifi_credentials_helper::GetWifiCredentialsForProfile(verifier());
  EXPECT_EQ(1U, verifier_credentials.size());
  EXPECT_EQ(WifiCredential::MakeSsidBytesForTest(ssid),
            verifier_credentials.begin()->ssid());

  const size_t profile_b_index = 1;
  ASSERT_TRUE(GetClient(profile_a_index)->AwaitMutualSyncCycleCompletion(
      GetClient(profile_b_index)));
  EXPECT_FALSE(wifi_credentials_helper::VerifierIsEmpty());
  EXPECT_TRUE(wifi_credentials_helper::AllProfilesMatch());
}
