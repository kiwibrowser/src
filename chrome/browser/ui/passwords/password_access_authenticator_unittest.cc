// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/password_access_authenticator.h"

#include <utility>

#include "base/bind.h"
#include "base/test/simple_test_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::TestWithParam;
using ::testing::Values;

namespace {

enum class ReauthResult { PASS, FAIL };
bool FakeOsReauthCall(bool* reauth_called,
                      ReauthResult result,
                      password_manager::ReauthPurpose purpose) {
  *reauth_called = true;
  return result == ReauthResult::PASS;
}

}  // namespace

class PasswordAccessAuthenticatorTest
    : public TestWithParam<password_manager::ReauthPurpose> {
 public:
  PasswordAccessAuthenticatorTest() : purpose_(GetParam()) {}
  ~PasswordAccessAuthenticatorTest() = default;

 protected:
  password_manager::ReauthPurpose purpose_;
};

// Check that a passed authentication does not expire before
// |kAuthValidityPeriodSeconds| seconds and does expire after
// |kAuthValidityPeriodSeconds| seconds.
TEST_P(PasswordAccessAuthenticatorTest, Expiration) {
  bool reauth_called = false;

  base::SimpleTestClock clock;
  clock.Advance(base::TimeDelta::FromSeconds(123));

  PasswordAccessAuthenticator authenticator(base::BindRepeating(
      &FakeOsReauthCall, &reauth_called, ReauthResult::PASS));
  authenticator.SetClockForTesting(&clock);

  EXPECT_TRUE(authenticator.EnsureUserIsAuthenticated(purpose_));
  EXPECT_TRUE(reauth_called);

  clock.Advance(base::TimeDelta::FromSeconds(
      PasswordAccessAuthenticator::kAuthValidityPeriodSeconds - 1));
  reauth_called = false;

  EXPECT_TRUE(authenticator.EnsureUserIsAuthenticated(purpose_));
  EXPECT_FALSE(reauth_called);

  clock.Advance(base::TimeDelta::FromSeconds(2));
  reauth_called = false;

  EXPECT_TRUE(authenticator.EnsureUserIsAuthenticated(purpose_));
  EXPECT_TRUE(reauth_called);
}

// Check that a forced authentication ignores previous successful challenges.
TEST_P(PasswordAccessAuthenticatorTest, ForceReauth) {
  bool reauth_called = false;

  base::SimpleTestClock clock;
  clock.Advance(base::TimeDelta::FromSeconds(123));

  PasswordAccessAuthenticator authenticator(base::BindRepeating(
      &FakeOsReauthCall, &reauth_called, ReauthResult::PASS));
  authenticator.SetClockForTesting(&clock);

  EXPECT_TRUE(authenticator.EnsureUserIsAuthenticated(purpose_));
  EXPECT_TRUE(reauth_called);

  clock.Advance(base::TimeDelta::FromSeconds(
      PasswordAccessAuthenticator::kAuthValidityPeriodSeconds - 1));
  reauth_called = false;

  EXPECT_TRUE(authenticator.ForceUserReauthentication(purpose_));
  EXPECT_TRUE(reauth_called);
}

// Check that a failed authentication does not start the grace period for
// skipping authentication.
TEST_P(PasswordAccessAuthenticatorTest, Failed) {
  bool reauth_called = false;

  base::SimpleTestClock clock;
  clock.Advance(base::TimeDelta::FromSeconds(456));

  PasswordAccessAuthenticator authenticator(base::BindRepeating(
      &FakeOsReauthCall, &reauth_called, ReauthResult::FAIL));
  authenticator.SetClockForTesting(&clock);

  EXPECT_FALSE(authenticator.EnsureUserIsAuthenticated(purpose_));
  EXPECT_TRUE(reauth_called);

  // Advance just a little bit, so that if |authenticator| starts the grace
  // period, this is still within it.
  clock.Advance(base::TimeDelta::FromSeconds(1));
  reauth_called = false;

  EXPECT_FALSE(authenticator.EnsureUserIsAuthenticated(purpose_));
  EXPECT_TRUE(reauth_called);
}

// Test the edge case: for the first authentication, the reauth challenge
// should always happen, even if the time is less than
// |kAuthValidityPeriodSeconds| seconds from the beginning.
TEST_P(PasswordAccessAuthenticatorTest, TimeZero) {
  bool reauth_called = false;

  base::SimpleTestClock clock;
  // |clock| now shows exactly 0.

  PasswordAccessAuthenticator authenticator(base::BindRepeating(
      &FakeOsReauthCall, &reauth_called, ReauthResult::PASS));
  authenticator.SetClockForTesting(&clock);

  EXPECT_TRUE(authenticator.EnsureUserIsAuthenticated(purpose_));
  EXPECT_TRUE(reauth_called);
}

INSTANTIATE_TEST_CASE_P(,
                        PasswordAccessAuthenticatorTest,
                        Values(password_manager::ReauthPurpose::VIEW_PASSWORD,
                               password_manager::ReauthPurpose::EXPORT));
