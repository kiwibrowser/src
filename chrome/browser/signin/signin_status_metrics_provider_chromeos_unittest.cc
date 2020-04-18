// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_status_metrics_provider_chromeos.h"

#include <string>

#include "base/files/file_path.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(SigninStatusMetricsProviderChromeOS, ComputeSigninStatusToUpload) {
  SigninStatusMetricsProviderChromeOS metrics_provider;

  SigninStatusMetricsProviderBase::SigninStatus status_to_upload =
      metrics_provider.ComputeSigninStatusToUpload(
          SigninStatusMetricsProviderBase::ALL_PROFILES_SIGNED_IN, true);
  EXPECT_EQ(SigninStatusMetricsProviderBase::ALL_PROFILES_SIGNED_IN,
            status_to_upload);

  status_to_upload = metrics_provider.ComputeSigninStatusToUpload(
      SigninStatusMetricsProviderBase::ALL_PROFILES_NOT_SIGNED_IN, false);
  EXPECT_EQ(SigninStatusMetricsProviderBase::ALL_PROFILES_NOT_SIGNED_IN,
            status_to_upload);

  status_to_upload = metrics_provider.ComputeSigninStatusToUpload(
      SigninStatusMetricsProviderBase::ALL_PROFILES_NOT_SIGNED_IN, true);
  EXPECT_EQ(SigninStatusMetricsProviderBase::MIXED_SIGNIN_STATUS,
            status_to_upload);

  status_to_upload = metrics_provider.ComputeSigninStatusToUpload(
      SigninStatusMetricsProviderBase::ALL_PROFILES_SIGNED_IN, false);
  EXPECT_EQ(SigninStatusMetricsProviderBase::ERROR_GETTING_SIGNIN_STATUS,
            status_to_upload);
}
