// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_bubble_controller.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_util.h"
#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_view.h"
#include "chrome/browser/ui/desktop_ios_promotion/sms_service_factory.h"
#include "components/prefs/pref_service.h"

DesktopIOSPromotionBubbleController::DesktopIOSPromotionBubbleController(
    Profile* profile,
    DesktopIOSPromotionView* promotion_view,
    desktop_ios_promotion::PromotionEntryPoint entry_point)
    : DesktopIOSPromotionController(profile, entry_point),
      sms_service_(SMSServiceFactory::GetForProfile(profile)),
      promotion_view_(promotion_view),
      weak_ptr_factory_(this) {
  sms_service_->QueryPhoneNumber(
      base::Bind(&DesktopIOSPromotionBubbleController::OnGotPhoneNumber,
                 weak_ptr_factory_.GetWeakPtr()));
}

DesktopIOSPromotionBubbleController::~DesktopIOSPromotionBubbleController() =
    default;

void DesktopIOSPromotionBubbleController::OnSendSMSClicked() {
  sms_service_->SendSMS(
      desktop_ios_promotion::GetSMSID(),
      base::Bind(&DesktopIOSPromotionBubbleController::OnSendSMS,
                 weak_ptr_factory_.GetWeakPtr()));

  // Update Profile prefs.
  profile_prefs()->SetInteger(prefs::kIOSPromotionSMSEntryPoint,
                              static_cast<int>(entry_point()));

  // Update dismissal reason.
  SetDismissalReason(desktop_ios_promotion::PromotionDismissalReason::SEND_SMS);
}

void DesktopIOSPromotionBubbleController::OnNoThanksClicked() {
  if (entry_point() !=
      desktop_ios_promotion::PromotionEntryPoint::FOOTNOTE_FOLLOWUP_BUBBLE) {
    PrefService* local_state = g_browser_process->local_state();
    local_state->SetBoolean(
        desktop_ios_promotion::kEntryPointLocalPrefs
            [static_cast<int>(entry_point())][static_cast<int>(
                desktop_ios_promotion::EntryPointLocalPrefType::DISMISSED)],
        true);
  }
  SetDismissalReason(
      desktop_ios_promotion::PromotionDismissalReason::NO_THANKS);
}

std::string DesktopIOSPromotionBubbleController::GetUsersRecoveryPhoneNumber() {
  return recovery_number_;
}

void DesktopIOSPromotionBubbleController::OnGotPhoneNumber(
    SMSService::Request* request,
    bool success,
    const std::string& number) {
  DCHECK(promotion_view_);
  if (success) {
    recovery_number_ = desktop_ios_promotion::FormatPhoneNumber(number);
    promotion_view_->UpdateRecoveryPhoneLabel();
  }
  UMA_HISTOGRAM_BOOLEAN("DesktopIOSPromotion.QueryPhoneNumberSucceeded",
                        success);
}

void DesktopIOSPromotionBubbleController::OnSendSMS(
    SMSService::Request* request,
    bool success,
    const std::string& number) {
  UMA_HISTOGRAM_BOOLEAN("DesktopIOSPromotion.SendSMSSucceeded", success);
}
