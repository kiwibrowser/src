// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_controller.h"

#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_util.h"
#include "chrome/common/chrome_features.h"
#include "components/prefs/pref_service.h"

DesktopIOSPromotionController::DesktopIOSPromotionController(
    Profile* profile,
    desktop_ios_promotion::PromotionEntryPoint entry_point)
    : profile_prefs_(profile->GetPrefs()),
      entry_point_(entry_point),
      dismissal_reason_(
          desktop_ios_promotion::PromotionDismissalReason::FOCUS_LOST) {}

DesktopIOSPromotionController::~DesktopIOSPromotionController() {
  desktop_ios_promotion::LogDismissalReason(dismissal_reason_, entry_point_);
}

void DesktopIOSPromotionController::OnPromotionShown() {
  UMA_HISTOGRAM_ENUMERATION(
      "DesktopIOSPromotion.ImpressionFromEntryPoint",
      static_cast<int>(entry_point_),
      static_cast<int>(
          desktop_ios_promotion::PromotionEntryPoint::ENTRY_POINT_MAX_VALUE));

  if (entry_point_ ==
      desktop_ios_promotion::PromotionEntryPoint::FOOTNOTE_FOLLOWUP_BUBBLE) {
    // We don't want to update sync with the impression of this entrypoint.
    return;
  }
  // update the impressions count.
  PrefService* local_state = g_browser_process->local_state();
  int impressions = local_state->GetInteger(
      desktop_ios_promotion::kEntryPointLocalPrefs
          [static_cast<int>(entry_point_)][static_cast<int>(
              desktop_ios_promotion::EntryPointLocalPrefType::IMPRESSIONS)]);
  impressions++;
  local_state->SetInteger(
      desktop_ios_promotion::kEntryPointLocalPrefs
          [static_cast<int>(entry_point_)][static_cast<int>(
              desktop_ios_promotion::EntryPointLocalPrefType::IMPRESSIONS)],
      impressions);

  // Update synced profile prefs.
  int shown_entrypoints =
      profile_prefs_->GetInteger(prefs::kIOSPromotionShownEntryPoints);
  shown_entrypoints |= 1 << static_cast<int>(entry_point_);
  profile_prefs_->SetInteger(prefs::kIOSPromotionShownEntryPoints,
                             shown_entrypoints);

  // If the promo is seen then it means the SMS was not sent on the last 7 days,
  // reset the pref.
  profile_prefs_->SetInteger(prefs::kIOSPromotionSMSEntryPoint, 0);

  double last_impression = base::Time::NowFromSystemTime().ToDoubleT();
  profile_prefs_->SetDouble(prefs::kIOSPromotionLastImpression,
                            last_impression);

  // If the variation id paramater is set on the finch experiement, set this
  // variation id to chrome sync pref to be accessed from iOS side.
  int variation_id = base::GetFieldTrialParamByFeatureAsInt(
      features::kDesktopIOSPromotion, "promo_variation_id", 0);
  if (variation_id)
    profile_prefs_->SetInteger(prefs::kIOSPromotionVariationId, variation_id);
}

void DesktopIOSPromotionController::OnLearnMoreLinkClicked() {
  dismissal_reason_ =
      desktop_ios_promotion::PromotionDismissalReason::LEARN_MORE;
}

void DesktopIOSPromotionController::SetDismissalReason(
    desktop_ios_promotion::PromotionDismissalReason reason) {
  dismissal_reason_ = reason;
}
