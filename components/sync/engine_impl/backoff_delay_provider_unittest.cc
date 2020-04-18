// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/backoff_delay_provider.h"

#include <memory>

#include "components/sync/base/syncer_error.h"
#include "components/sync/engine/cycle/model_neutral_state.h"
#include "components/sync/engine/polling_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;

namespace syncer {

class BackoffDelayProviderTest : public testing::Test {};

TEST_F(BackoffDelayProviderTest, GetRecommendedDelay) {
  std::unique_ptr<BackoffDelayProvider> delay(
      BackoffDelayProvider::FromDefaults());
  EXPECT_EQ(TimeDelta::FromSeconds(1),
            delay->GetDelay(TimeDelta::FromSeconds(0)));
  EXPECT_LE(TimeDelta::FromSeconds(1),
            delay->GetDelay(TimeDelta::FromSeconds(1)));
  EXPECT_LE(TimeDelta::FromSeconds(50),
            delay->GetDelay(TimeDelta::FromSeconds(50)));
  EXPECT_LE(TimeDelta::FromSeconds(10),
            delay->GetDelay(TimeDelta::FromSeconds(10)));
  EXPECT_EQ(TimeDelta::FromSeconds(kMaxBackoffSeconds),
            delay->GetDelay(TimeDelta::FromSeconds(kMaxBackoffSeconds)));
  EXPECT_EQ(TimeDelta::FromSeconds(kMaxBackoffSeconds),
            delay->GetDelay(TimeDelta::FromSeconds(kMaxBackoffSeconds + 1)));
}

TEST_F(BackoffDelayProviderTest, GetInitialDelay) {
  std::unique_ptr<BackoffDelayProvider> delay(
      BackoffDelayProvider::FromDefaults());
  ModelNeutralState state;
  state.last_get_key_result = SYNC_SERVER_ERROR;
  EXPECT_EQ(kInitialBackoffRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_get_key_result = UNSET;
  state.last_download_updates_result = SERVER_RETURN_MIGRATION_DONE;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = NETWORK_CONNECTION_UNAVAILABLE;
  EXPECT_EQ(kInitialBackoffRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = SERVER_RETURN_TRANSIENT_ERROR;
  EXPECT_EQ(kInitialBackoffRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = SERVER_RESPONSE_VALIDATION_FAILED;
  EXPECT_EQ(kInitialBackoffRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = DATATYPE_TRIGGERED_RETRY;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = SYNCER_OK;
  // Note that updating credentials triggers a canary job, trumping
  // the initial delay, but in theory we still expect this function to treat
  // it like any other error in the system (except migration).
  state.commit_result = SERVER_RETURN_INVALID_CREDENTIAL;
  EXPECT_EQ(kInitialBackoffRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.commit_result = SERVER_RETURN_MIGRATION_DONE;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.commit_result = NETWORK_CONNECTION_UNAVAILABLE;
  EXPECT_EQ(kInitialBackoffRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.commit_result = SERVER_RETURN_CONFLICT;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());
}

TEST_F(BackoffDelayProviderTest, GetInitialDelayWithOverride) {
  std::unique_ptr<BackoffDelayProvider> delay(
      BackoffDelayProvider::WithShortInitialRetryOverride());
  ModelNeutralState state;
  state.last_get_key_result = SYNC_SERVER_ERROR;
  EXPECT_EQ(kInitialBackoffShortRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_get_key_result = UNSET;
  state.last_download_updates_result = SERVER_RETURN_MIGRATION_DONE;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = SERVER_RETURN_TRANSIENT_ERROR;
  EXPECT_EQ(kInitialBackoffShortRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = SERVER_RESPONSE_VALIDATION_FAILED;
  EXPECT_EQ(kInitialBackoffShortRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = DATATYPE_TRIGGERED_RETRY;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.last_download_updates_result = SYNCER_OK;
  // Note that updating credentials triggers a canary job, trumping
  // the initial delay, but in theory we still expect this function to treat
  // it like any other error in the system (except migration).
  state.commit_result = SERVER_RETURN_INVALID_CREDENTIAL;
  EXPECT_EQ(kInitialBackoffShortRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.commit_result = SERVER_RETURN_MIGRATION_DONE;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());

  state.commit_result = SERVER_RETURN_CONFLICT;
  EXPECT_EQ(kInitialBackoffImmediateRetrySeconds,
            delay->GetInitialDelay(state).InSeconds());
}

}  // namespace syncer
