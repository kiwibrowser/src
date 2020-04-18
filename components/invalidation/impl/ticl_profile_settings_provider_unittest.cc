// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/ticl_profile_settings_provider.h"

#include <memory>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/gcm_driver/fake_gcm_driver.h"
#include "components/gcm_driver/gcm_channel_status_syncer.h"
#include "components/invalidation/impl/fake_invalidation_state_tracker.h"
#include "components/invalidation/impl/invalidation_prefs.h"
#include "components/invalidation/impl/invalidation_state_tracker.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "components/invalidation/impl/ticl_invalidation_service.h"
#include "components/invalidation/impl/ticl_settings_provider.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "google_apis/gaia/fake_identity_provider.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "google_apis/gaia/identity_provider.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace invalidation {

class TiclProfileSettingsProviderTest : public testing::Test {
 protected:
  TiclProfileSettingsProviderTest();
  ~TiclProfileSettingsProviderTest() override;

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  TiclInvalidationService::InvalidationNetworkChannel GetNetworkChannel();

  base::MessageLoop message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  gcm::FakeGCMDriver gcm_driver_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  FakeOAuth2TokenService token_service_;

  std::unique_ptr<TiclInvalidationService> invalidation_service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TiclProfileSettingsProviderTest);
};

TiclProfileSettingsProviderTest::TiclProfileSettingsProviderTest() {}

TiclProfileSettingsProviderTest::~TiclProfileSettingsProviderTest() {}

void TiclProfileSettingsProviderTest::SetUp() {
  gcm::GCMChannelStatusSyncer::RegisterProfilePrefs(pref_service_.registry());
  ProfileInvalidationProvider::RegisterProfilePrefs(pref_service_.registry());

  request_context_getter_ =
      new net::TestURLRequestContextGetter(base::ThreadTaskRunnerHandle::Get());

  invalidation_service_.reset(new TiclInvalidationService(
      "TestUserAgent", std::unique_ptr<IdentityProvider>(
                           new FakeIdentityProvider(&token_service_)),
      std::unique_ptr<TiclSettingsProvider>(
          new TiclProfileSettingsProvider(&pref_service_)),
      &gcm_driver_, request_context_getter_));
  invalidation_service_->Init(std::unique_ptr<syncer::InvalidationStateTracker>(
      new syncer::FakeInvalidationStateTracker));
}

void TiclProfileSettingsProviderTest::TearDown() {
  invalidation_service_.reset();
}

TiclInvalidationService::InvalidationNetworkChannel
TiclProfileSettingsProviderTest::GetNetworkChannel() {
  return invalidation_service_->network_channel_type_;
}

TEST_F(TiclProfileSettingsProviderTest, ChannelSelectionTest) {
  // Default value should be GCM channel.
  EXPECT_EQ(TiclInvalidationService::GCM_NETWORK_CHANNEL, GetNetworkChannel());

  // If GCM is enabled and invalidation channel setting is not set or set to
  // true then use GCM channel.
  pref_service_.SetBoolean(gcm::prefs::kGCMChannelStatus, true);
  pref_service_.SetBoolean(prefs::kInvalidationServiceUseGCMChannel, true);
  EXPECT_EQ(TiclInvalidationService::GCM_NETWORK_CHANNEL, GetNetworkChannel());

  pref_service_.SetBoolean(gcm::prefs::kGCMChannelStatus, true);
  pref_service_.ClearPref(prefs::kInvalidationServiceUseGCMChannel);
  EXPECT_EQ(TiclInvalidationService::GCM_NETWORK_CHANNEL, GetNetworkChannel());

  pref_service_.ClearPref(gcm::prefs::kGCMChannelStatus);
  pref_service_.SetBoolean(prefs::kInvalidationServiceUseGCMChannel, true);
  EXPECT_EQ(TiclInvalidationService::GCM_NETWORK_CHANNEL, GetNetworkChannel());

  // If invalidation channel setting says use GCM but GCM is not enabled, do not
  // fall back to push channel.
  pref_service_.SetBoolean(gcm::prefs::kGCMChannelStatus, false);
  pref_service_.SetBoolean(prefs::kInvalidationServiceUseGCMChannel, true);
  EXPECT_EQ(TiclInvalidationService::GCM_NETWORK_CHANNEL, GetNetworkChannel());

  // If invalidation channel setting is set to false, fall back to push channel.
  pref_service_.SetBoolean(gcm::prefs::kGCMChannelStatus, true);
  pref_service_.SetBoolean(prefs::kInvalidationServiceUseGCMChannel, false);
  EXPECT_EQ(TiclInvalidationService::PUSH_CLIENT_CHANNEL, GetNetworkChannel());
}

}  // namespace invalidation
