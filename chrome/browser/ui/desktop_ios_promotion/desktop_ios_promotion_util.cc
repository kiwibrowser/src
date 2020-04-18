// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/desktop_ios_promotion/desktop_ios_promotion_util.h"

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "third_party/libphonenumber/phonenumber_api.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"

using i18n::phonenumbers::PhoneNumber;
using i18n::phonenumbers::PhoneNumberUtil;

namespace desktop_ios_promotion {

// Default Impression cap. for each entry point.
const int kEntryPointImpressionCap[] = {0, 2, 2, 5, 10};

const char* kEntrypointHistogramPrefix[] = {
    "",  // Padding as PromotionEntryPoints values starts from 1.
    "SavePasswordsNewBubble",
    "BookmarksNewBubble",
    "BookmarksFootNote",
    "HistoryPage",
};

// Entry point string names, used to check which entry point is targeted from
// finch parameters.
const char* kPromotionEntryPointNames[] = {
    "", "SavePasswordsBubblePromotion", "BookmarksBubblePromotion",
    "BookmarksFootnotePromotion", "HistoryPagePromotion"};

// Text used on the promotion bubble body when the phone number is present,
// this array is indexed by the text version specified by body_text_id Finch
// parameter.
const int kBubbleBodyTextNoPhoneNumberId[2] = {
    IDS_PASSWORD_MANAGER_DESKTOP_TO_IOS_PROMO_TEXT,
    IDS_PASSWORD_MANAGER_DESKTOP_TO_IOS_PROMO_TEXT_V2};

// Text used on the promotion bubble body when the phone number is not present,
// this array is indexed by the text version specified by body_text_id Finch
// parameter.
const int kBubbleBodyTextWithPhoneNumberId[2] = {
    IDS_PASSWORD_MANAGER_DESKTOP_TO_IOS_PROMO_TEXT_WITH_PHONE_NUMBER,
    IDS_PASSWORD_MANAGER_DESKTOP_TO_IOS_PROMO_TEXT_WITH_PHONE_NUMBER_V2};

// Text used on the promotion bubble title, The first dimension is the entry
// point, and the second is the text version specified by title_text_id Finch
// parameter.
const int kBubbleTitleTextId[3][3] = {
    {0, 0, 0},  // Padding as PromotionEntryPoints values starts from 1.
    {IDS_PASSWORD_MANAGER_DESKTOP_TO_IOS_PROMO_TITLE,
     IDS_PASSWORD_MANAGER_DESKTOP_TO_IOS_PROMO_TITLE_V2,
     IDS_PASSWORD_MANAGER_DESKTOP_TO_IOS_PROMO_TITLE_V3},
    {IDS_BOOKMARK_BUBBLE_DESKTOP_TO_IOS_PROMO_TITLE,
     IDS_BOOKMARK_BUBBLE_DESKTOP_TO_IOS_PROMO_TITLE_V2,
     IDS_BOOKMARK_BUBBLE_DESKTOP_TO_IOS_PROMO_TITLE_V3}};

bool IsEligibleForIOSPromotion(
    Profile* profile,
    desktop_ios_promotion::PromotionEntryPoint entry_point) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kForceDesktopIOSPromotion)) {
    return true;
  }

  // Don't show promotion if there has been authentication error, because this
  // will prevent the recovery phone number from showing.
  const SigninErrorController* signin_error_controller =
      profiles::GetSigninErrorController(profile);
  if (signin_error_controller && signin_error_controller->HasError())
    return false;

  const browser_sync::ProfileSyncService* sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile);
  // Promotion should only show for english locale.
  PrefService* local_state = g_browser_process->local_state();
  std::string locale = base::i18n::GetConfiguredLocale();
  if (locale != "en-US" && locale != "en-CA")
    return false;
  if (!base::FeatureList::IsEnabled(features::kDesktopIOSPromotion) ||
      !sync_service || !sync_service->IsSyncAllowed())
    return false;

  // Check if the specific entrypoint is enabled by Finch.
  std::string targeted_entry_point = base::GetFieldTrialParamValueByFeature(
      features::kDesktopIOSPromotion, "entry_point");
  if (targeted_entry_point !=
      kPromotionEntryPointNames[static_cast<int>(entry_point)])
    return false;
  bool is_dismissed = local_state->GetBoolean(
      kEntryPointLocalPrefs[static_cast<int>(entry_point)][1]);
  int show_count = local_state->GetInteger(
      kEntryPointLocalPrefs[static_cast<int>(entry_point)][0]);

  // Get the impression cap. from Finch, if exists override the value from the
  // kEntryPointImpressionCap array.
  int impression_cap = base::GetFieldTrialParamByFeatureAsInt(
      features::kDesktopIOSPromotion, "max_views",
      kEntryPointImpressionCap[static_cast<int>(entry_point)]);
  if (is_dismissed || show_count >= impression_cap)
    return false;

  PrefService* prefs = profile->GetPrefs();
  // Don't show the promotion if the user have used any entry point to recieve
  // SMS on the last 7 days.
  double last_impression = prefs->GetDouble(prefs::kIOSPromotionLastImpression);
  int sms_entrypoint = prefs->GetInteger(prefs::kIOSPromotionSMSEntryPoint);
  base::TimeDelta delta =
      base::Time::Now() - base::Time::FromDoubleT(last_impression);
  if (delta.InDays() <= 7 && sms_entrypoint != 0)
    return false;

  bool is_user_eligible = prefs->GetBoolean(prefs::kIOSPromotionEligible);
  bool did_promo_done_before = prefs->GetBoolean(prefs::kIOSPromotionDone);
  return is_user_eligible && !did_promo_done_before;
}

gfx::ImageSkia GetPromoImage(SkColor color) {
  gfx::ImageSkia icon = gfx::CreateVectorIcon(
      kOpenInPhoneIcon, color_utils::DeriveDefaultIconColor(color));
  return icon;
}

base::string16 GetPromoText(
    desktop_ios_promotion::PromotionEntryPoint entry_point,
    const std::string& phone_number) {
  int text_id_from_finch = base::GetFieldTrialParamByFeatureAsInt(
      features::kDesktopIOSPromotion, "body_text_id", 0);
  int body_text_i10_id = kBubbleBodyTextWithPhoneNumberId[text_id_from_finch];
  if (phone_number.empty()) {
    body_text_i10_id = kBubbleBodyTextNoPhoneNumberId[text_id_from_finch];
    return l10n_util::GetStringUTF16(body_text_i10_id)
        .append(base::string16(13, ' '));
  }
  return l10n_util::GetStringFUTF16(body_text_i10_id,
                                    base::UTF8ToUTF16(phone_number));
}

base::string16 GetPromoTitle(
    desktop_ios_promotion::PromotionEntryPoint entry_point) {
  int text_id_from_finch = base::GetFieldTrialParamByFeatureAsInt(
      features::kDesktopIOSPromotion, "title_text_id", 0);
  return l10n_util::GetStringUTF16(
      kBubbleTitleTextId[static_cast<int>(entry_point)][text_id_from_finch]);
}

std::string GetSMSID() {
  const std::string default_sms_message_id = "19001507";
  // Get the SMS message id from the finch group.
  std::string finch_sms_id = base::GetFieldTrialParamValueByFeature(
      features::kDesktopIOSPromotion, "sms_id");
  return finch_sms_id.empty() ? default_sms_message_id : finch_sms_id;
}

std::string FormatPhoneNumber(const std::string& number) {
  PhoneNumberUtil* phone_util = PhoneNumberUtil::GetInstance();
  PhoneNumber number_object;
  if (phone_util->Parse(number, std::string(), &number_object) !=
      PhoneNumberUtil::NO_PARSING_ERROR) {
    return number;
  }
  std::string formatted_number;
  phone_util->Format(number_object,
                     PhoneNumberUtil::PhoneNumberFormat::NATIONAL,
                     &formatted_number);
  return formatted_number;
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kIOSPromotionEligible, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterBooleanPref(
      prefs::kIOSPromotionDone, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterIntegerPref(
      prefs::kIOSPromotionSMSEntryPoint, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterIntegerPref(
      prefs::kIOSPromotionShownEntryPoints, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterDoublePref(
      prefs::kIOSPromotionLastImpression, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterIntegerPref(
      prefs::kIOSPromotionVariationId, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
}

void RegisterLocalPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(prefs::kNumberSavePasswordsBubbleIOSPromoShown,
                                0);
  registry->RegisterBooleanPref(prefs::kSavePasswordsBubbleIOSPromoDismissed,
                                false);
  registry->RegisterIntegerPref(prefs::kNumberBookmarksBubbleIOSPromoShown, 0);
  registry->RegisterBooleanPref(prefs::kBookmarksBubbleIOSPromoDismissed,
                                false);
  registry->RegisterIntegerPref(prefs::kNumberBookmarksFootNoteIOSPromoShown,
                                0);
  registry->RegisterBooleanPref(prefs::kBookmarksFootNoteIOSPromoDismissed,
                                false);
  registry->RegisterIntegerPref(prefs::kNumberHistoryPageIOSPromoShown, 0);
  registry->RegisterBooleanPref(prefs::kHistoryPageIOSPromoDismissed, false);
}

void LogDismissalReason(PromotionDismissalReason reason,
                        PromotionEntryPoint entry_point) {
  constexpr int dismissal_reason_min =
      static_cast<int>(PromotionDismissalReason::FOCUS_LOST);
  base::Histogram::FactoryGet(
      base::StringPrintf(
          "DesktopIOSPromotion.%s.DismissalReason",
          desktop_ios_promotion::kEntrypointHistogramPrefix[static_cast<int>(
              entry_point)]),
      dismissal_reason_min,
      static_cast<int>(PromotionDismissalReason::DISMISSAL_REASON_MAX_VALUE),
      static_cast<int>(PromotionDismissalReason::DISMISSAL_REASON_MAX_VALUE) +
          1,
      base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(static_cast<int>(reason));
}

}  // namespace desktop_ios_promotion
