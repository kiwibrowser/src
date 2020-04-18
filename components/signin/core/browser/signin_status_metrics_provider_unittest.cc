// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_status_metrics_provider.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

TEST(SigninStatusMetricsProviderTest, UpdateInitialSigninStatus) {
  SigninStatusMetricsProvider metrics_provider(nullptr, true);

  metrics_provider.UpdateInitialSigninStatus(2, 2);
  EXPECT_EQ(SigninStatusMetricsProviderBase::ALL_PROFILES_SIGNED_IN,
            metrics_provider.GetSigninStatusForTesting());
  metrics_provider.UpdateInitialSigninStatus(2, 0);
  EXPECT_EQ(SigninStatusMetricsProviderBase::ALL_PROFILES_NOT_SIGNED_IN,
            metrics_provider.GetSigninStatusForTesting());
  metrics_provider.UpdateInitialSigninStatus(2, 1);
  EXPECT_EQ(SigninStatusMetricsProviderBase::MIXED_SIGNIN_STATUS,
            metrics_provider.GetSigninStatusForTesting());
}

TEST(SigninStatusMetricsProviderTest, GoogleSigninSucceeded) {
  SigninStatusMetricsProvider metrics_provider(nullptr, true);

  // Initial status is all signed out and then one of the profiles is signed in.
  metrics_provider.UpdateInitialSigninStatus(2, 0);
  metrics_provider.GoogleSigninSucceeded(std::string(), std::string());
  EXPECT_EQ(SigninStatusMetricsProviderBase::MIXED_SIGNIN_STATUS,
            metrics_provider.GetSigninStatusForTesting());

  // Initial status is mixed and then one of the profiles is signed in.
  metrics_provider.UpdateInitialSigninStatus(2, 1);
  metrics_provider.GoogleSigninSucceeded(std::string(), std::string());
  EXPECT_EQ(SigninStatusMetricsProviderBase::MIXED_SIGNIN_STATUS,
            metrics_provider.GetSigninStatusForTesting());
}

TEST(SigninStatusMetricsProviderTest, GoogleSignedOut) {
  SigninStatusMetricsProvider metrics_provider(nullptr, true);

  // Initial status is all signed in and then one of the profiles is signed out.
  metrics_provider.UpdateInitialSigninStatus(2, 2);
  metrics_provider.GoogleSignedOut(std::string(), std::string());
  EXPECT_EQ(SigninStatusMetricsProviderBase::MIXED_SIGNIN_STATUS,
            metrics_provider.GetSigninStatusForTesting());

  // Initial status is mixed and then one of the profiles is signed out.
  metrics_provider.UpdateInitialSigninStatus(2, 1);
  metrics_provider.GoogleSignedOut(std::string(), std::string());
  EXPECT_EQ(SigninStatusMetricsProviderBase::MIXED_SIGNIN_STATUS,
            metrics_provider.GetSigninStatusForTesting());
}
