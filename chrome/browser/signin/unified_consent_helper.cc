// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/unified_consent_helper.h"

#include "base/feature_list.h"
#include "build/buildflag.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "components/signin/core/browser/signin_buildflags.h"

signin::UnifiedConsentFeatureState GetUnifiedConsentFeatureState(
    Profile* profile) {
  DCHECK(profile);
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  // On Dice platforms, unified consent requires Dice to be enabled first.
  if (!AccountConsistencyModeManager::IsDiceEnabledForProfile(profile))
    return signin::UnifiedConsentFeatureState::kDisabled;
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

  return signin::GetUnifiedConsentFeatureState();
}

bool IsUnifiedConsentEnabled(Profile* profile) {
  DCHECK(profile);
  signin::UnifiedConsentFeatureState feature_state =
      GetUnifiedConsentFeatureState(profile);
  return feature_state != signin::UnifiedConsentFeatureState::kDisabled;
}

bool IsUnifiedConsentBumpEnabled(Profile* profile) {
  DCHECK(profile);
  signin::UnifiedConsentFeatureState feature_state =
      GetUnifiedConsentFeatureState(profile);
  return feature_state == signin::UnifiedConsentFeatureState::kEnabledWithBump;
}
