// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/desktop_promotion/desktop_promotion_sync_observer.h"

#include <algorithm>
#include <memory>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/pref_names.h"

namespace {

// These values are written to logs.  New values can be added, but existing
// values must never be reordered or deleted and reused.
const char* kDesktopIOSPromotionEntrypointHistogramPrefix[] = {
    "SavePasswordsNewBubble", "BookmarksNewBubble", "BookmarksFootNote",
    "HistoryPage",
};

}  // namespace

DesktopPromotionSyncObserver::DesktopPromotionSyncObserver(
    PrefService* pref_service,
    browser_sync::ProfileSyncService* sync_service)
    : pref_service_(pref_service), sync_service_(sync_service) {
  DCHECK(pref_service_);
  DCHECK(sync_service_);
  sync_service_->AddObserver(this);
}

DesktopPromotionSyncObserver::~DesktopPromotionSyncObserver() {
  sync_service_->RemoveObserver(this);
}

void DesktopPromotionSyncObserver::OnStateChanged(syncer::SyncService* sync) {
  if (desktop_metrics_logger_initiated_ ||
      !sync_service_->GetActiveDataTypes().Has(syncer::PRIORITY_PREFERENCES)) {
    return;
  }

  desktop_metrics_logger_initiated_ = true;
  bool done_logging =
      pref_service_->GetBoolean(prefs::kDesktopIOSPromotionDone);
  bool is_eligible =
      pref_service_->GetBoolean(prefs::kDesktopIOSPromotionEligible);
  double last_impression =
      pref_service_->GetDouble(prefs::kDesktopIOSPromotionLastImpression);
  base::TimeDelta delta =
      base::Time::Now() - base::Time::FromDoubleT(last_impression);
  if (done_logging || delta.InDays() >= 7) {
    sync_service_->RemoveObserver(this);
    // If the user was eligible but didn't see the promo on the last 7 days and
    // installed Chrome then their eligiblity pref is reset to false.
    if (delta.InDays() >= 7 && is_eligible)
      pref_service_->SetBoolean(prefs::kDesktopIOSPromotionEligible, false);
    return;
  }

  // This user have seen the promotion in the last 7 days so it may be a
  // reason of the installation.
  int sms_entrypoint =
      pref_service_->GetInteger(prefs::kDesktopIOSPromotionSMSEntryPoint);
  int shown_entrypoints =
      pref_service_->GetInteger(prefs::kDesktopIOSPromotionShownEntryPoints);

  // Entry points are represented on the preference by integers [1..4].
  // Entry points constants are defined on:
  // chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_util.h
  int entrypoint_prefixes_count =
      arraysize(kDesktopIOSPromotionEntrypointHistogramPrefix);
  for (int i = 1; i < entrypoint_prefixes_count + 1; i++) {
    // Note this fakes an enum UMA using an exact linear UMA, since the enum is
    // a modification of another enum, but isn't defined directly.
    if (sms_entrypoint == i) {
      UMA_HISTOGRAM_EXACT_LINEAR("DesktopIOSPromotion.SMSSent.IOSSigninReason",
                                 i, entrypoint_prefixes_count + 1);
      // If the time delta is negative due to client bad clock we log 0 instead.
      base::Histogram::FactoryGet(
          base::StringPrintf(
              "DesktopIOSPromotion.%s.SMSToSigninTime",
              kDesktopIOSPromotionEntrypointHistogramPrefix[i - 1]),
          1, 168, 24, base::Histogram::kUmaTargetedHistogramFlag)
          ->Add(std::max(0, delta.InHours()));
    } else {
      // If the user saw this promotion type, log that it could be a reason
      // for the signin.
      if ((1 << i) & shown_entrypoints)
        UMA_HISTOGRAM_EXACT_LINEAR("DesktopIOSPromotion.NoSMS.IOSSigninReason",
                                   i, entrypoint_prefixes_count + 1);
    }
  }

  // Check the variation id preference, if it's set then log to UMA that the
  // user has seen this promotion variation on desktop.
  int promo_variation_id =
      pref_service_->GetInteger(prefs::kDesktopIOSPromotionVariationId);
  if (promo_variation_id != 0) {
    if (sms_entrypoint != 0) {
      base::UmaHistogramSparse(
          "DesktopIOSPromotion.SMSSent.VariationSigninReason",
          promo_variation_id);
    } else {
      base::UmaHistogramSparse(
          "DesktopIOSPromotion.NoSMS.VariationSigninReason",
          promo_variation_id);
    }
  }

  pref_service_->SetBoolean(prefs::kDesktopIOSPromotionDone, true);
  sync_service_->RemoveObserver(this);
}
