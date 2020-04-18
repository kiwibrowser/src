// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_UTIL_H_
#define CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_UTIL_H_

#include "base/macros.h"
#include "chrome/common/pref_names.h"
#include "ui/gfx/image/image_skia.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefRegistrySimple;
class Profile;

namespace desktop_ios_promotion {

// Represents the reason that the Desktop to iOS promotion was dismissed for.
// These values are written to logs. New values can be added, but existing
// values must never be reordered or deleted and reused.
enum class PromotionDismissalReason {
  // Focus lost is the default reason and represents no interaction.
  FOCUS_LOST = 0,
  NO_THANKS = 1,
  CLOSE_BUTTON = 2,
  SEND_SMS = 3,
  LEARN_MORE = 4,
  // Used for histograms logging.
  DISMISSAL_REASON_MAX_VALUE = 5
};

enum class EntryPointLocalPrefType { IMPRESSIONS = 0, DISMISSED = 1 };

// The place where the promotion appeared.
// Intentionally skipped the = 0 value, as it represents the defaults on the
// prefs (eg. User didn't recieve SMS).
// These values are written to logs. New values can be added, but existing
// values must never be reordered or deleted and reused.
enum class PromotionEntryPoint {
  SAVE_PASSWORD_BUBBLE = 1,
  BOOKMARKS_BUBBLE = 2,
  BOOKMARKS_FOOTNOTE = 3,
  HISTORY_PAGE = 4,
  FOOTNOTE_FOLLOWUP_BUBBLE = 5,
  // Used for histograms logging.
  ENTRY_POINT_MAX_VALUE = 6
};

// Entry points local prefs, each entry point has a preference for impressions
// count and a preference for whether user dismissed it or not.
// Do not change the order of this array, as it's indexed using
// desktop_ios_promotion::PromotionEntryPoint.
const char* const kEntryPointLocalPrefs[5][2] = {
    // The first emtpy value is for padding as PromotionEntryPoints enum starts
    // from 1.
    {"", ""},
    {prefs::kNumberSavePasswordsBubbleIOSPromoShown,
     prefs::kSavePasswordsBubbleIOSPromoDismissed},
    {prefs::kNumberBookmarksBubbleIOSPromoShown,
     prefs::kBookmarksBubbleIOSPromoDismissed},
    {prefs::kNumberBookmarksFootNoteIOSPromoShown,
     prefs::kBookmarksFootNoteIOSPromoDismissed},
    {prefs::kNumberHistoryPageIOSPromoShown,
     prefs::kHistoryPageIOSPromoDismissed}};

bool IsEligibleForIOSPromotion(Profile* profile,
                               PromotionEntryPoint entry_point);

// Returns the SMS ID to be used with send SMS API call.
std::string GetSMSID();

// Returns the Promotion text based on the promotion entry point, Finch
// parameters and phone number presence.
base::string16 GetPromoText(PromotionEntryPoint entry_point,
                            const std::string& phone_number = std::string());

// Returns the Promotion title based on the promotion entry point and finch
// parameters.
base::string16 GetPromoTitle(PromotionEntryPoint entry_point);

gfx::ImageSkia GetPromoImage(SkColor color);

std::string FormatPhoneNumber(const std::string& number);

// Register all Priority Sync preferences.
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

// Register all local only Preferences.
void RegisterLocalPrefs(PrefRegistrySimple* registry);

// Log promotion dismissal reason for entry points.
void LogDismissalReason(PromotionDismissalReason reason,
                        PromotionEntryPoint entry_point);

}  // namespace desktop_ios_promotion

#endif  // CHROME_BROWSER_UI_DESKTOP_IOS_PROMOTION_DESKTOP_IOS_PROMOTION_UTIL_H_
