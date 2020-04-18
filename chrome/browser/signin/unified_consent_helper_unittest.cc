// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/unified_consent_helper.h"

#include "build/buildflag.h"
#include "chrome/test/base/testing_profile.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/browser/scoped_unified_consent.h"
#include "components/signin/core/browser/signin_buildflags.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_DICE_SUPPORT)

// On Dice platforms, unified consent can only be enabled for Dice profiles.
TEST(UnifiedConsentHelperTest, DiceDisabled) {
  // Disable Dice.
  signin::ScopedAccountConsistencyDiceFixAuthErrors dice_fix_auth_errors;

  content::TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;

  for (signin::UnifiedConsentFeatureState state :
       {signin::UnifiedConsentFeatureState::kDisabled,
        signin::UnifiedConsentFeatureState::kEnabledNoBump,
        signin::UnifiedConsentFeatureState::kEnabledWithBump}) {
    signin::ScopedUnifiedConsent scoped_state(state);
    EXPECT_EQ(signin::UnifiedConsentFeatureState::kDisabled,
              GetUnifiedConsentFeatureState(&profile));
    EXPECT_FALSE(IsUnifiedConsentEnabled(&profile));
    EXPECT_FALSE(IsUnifiedConsentBumpEnabled(&profile));
  }
}

#endif

// Checks that the feature state for the profile is the same as the global
// feature state.
TEST(UnifiedConsentHelperTest, FeatureState) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  // Enable Dice.
  signin::ScopedAccountConsistencyDice dice;
#endif

  content::TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;

  // Unified consent is disabled by default.
  EXPECT_EQ(signin::UnifiedConsentFeatureState::kDisabled,
            GetUnifiedConsentFeatureState(&profile));

  // The feature state for the profile is the same as the global feature state.
  {
    signin::ScopedUnifiedConsent scoped_disabled(
        signin::UnifiedConsentFeatureState::kDisabled);
    EXPECT_EQ(signin::UnifiedConsentFeatureState::kDisabled,
              GetUnifiedConsentFeatureState(&profile));
    EXPECT_FALSE(IsUnifiedConsentEnabled(&profile));
    EXPECT_FALSE(IsUnifiedConsentBumpEnabled(&profile));
  }

  {
    signin::ScopedUnifiedConsent scoped_no_bump(
        signin::UnifiedConsentFeatureState::kEnabledNoBump);
    EXPECT_EQ(signin::UnifiedConsentFeatureState::kEnabledNoBump,
              GetUnifiedConsentFeatureState(&profile));
    EXPECT_TRUE(IsUnifiedConsentEnabled(&profile));
    EXPECT_FALSE(IsUnifiedConsentBumpEnabled(&profile));
  }

  {
    signin::ScopedUnifiedConsent scoped_bump(
        signin::UnifiedConsentFeatureState::kEnabledWithBump);
    EXPECT_EQ(signin::UnifiedConsentFeatureState::kEnabledWithBump,
              GetUnifiedConsentFeatureState(&profile));
    EXPECT_TRUE(IsUnifiedConsentEnabled(&profile));
    EXPECT_TRUE(IsUnifiedConsentBumpEnabled(&profile));
  }
}
