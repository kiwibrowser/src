// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/counters/browsing_data_counter_utils.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browsing_data/counters/cache_counter.h"
#include "chrome/browser/browsing_data/counters/media_licenses_counter.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/browsing_data/counters/hosted_apps_counter.h"
#endif

// A helper function to display the size of cache in units of MB or higher.
// We need this, as 1 MB is the lowest nonzero cache size displayed by the
// counter.
base::string16 FormatBytesMBOrHigher(
    browsing_data::BrowsingDataCounter::ResultInt bytes) {
  if (ui::GetByteDisplayUnits(bytes) >= ui::DataUnits::DATA_UNITS_MEBIBYTE)
    return ui::FormatBytes(bytes);

  return ui::FormatBytesWithUnits(
      bytes, ui::DataUnits::DATA_UNITS_MEBIBYTE, true);
}

base::string16 GetChromeCounterTextFromResult(
    const browsing_data::BrowsingDataCounter::Result* result,
    Profile* profile) {
  std::string pref_name = result->source()->GetPrefName();

  if (!result->Finished()) {
    // The counter is still counting.
    return l10n_util::GetStringUTF16(IDS_CLEAR_BROWSING_DATA_CALCULATING);
  }

  if (pref_name == browsing_data::prefs::kDeleteCache ||
      pref_name == browsing_data::prefs::kDeleteCacheBasic) {
    // Cache counter.
    const auto* cache_result =
        static_cast<const CacheCounter::CacheResult*>(result);
    int64_t cache_size_bytes = cache_result->cache_size();
    bool is_upper_limit = cache_result->is_upper_limit();
    bool is_basic_tab = pref_name == browsing_data::prefs::kDeleteCacheBasic;

    // Three cases: Nonzero result for the entire cache, nonzero result for
    // a subset of cache (i.e. a finite time interval), and almost zero (< 1MB).
    static const int kBytesInAMegabyte = 1024 * 1024;
    if (cache_size_bytes >= kBytesInAMegabyte) {
      base::string16 formatted_size = FormatBytesMBOrHigher(cache_size_bytes);
      if (!is_upper_limit) {
        return is_basic_tab ? l10n_util::GetStringFUTF16(
                                  IDS_DEL_CACHE_COUNTER_BASIC, formatted_size)
                            : formatted_size;
      }
      return l10n_util::GetStringFUTF16(
          is_basic_tab ? IDS_DEL_CACHE_COUNTER_UPPER_ESTIMATE_BASIC
                       : IDS_DEL_CACHE_COUNTER_UPPER_ESTIMATE,
          formatted_size);
    }
    return l10n_util::GetStringUTF16(
        is_basic_tab ? IDS_DEL_CACHE_COUNTER_ALMOST_EMPTY_BASIC
                     : IDS_DEL_CACHE_COUNTER_ALMOST_EMPTY);
  }
  if (pref_name == browsing_data::prefs::kDeleteCookiesBasic) {
    // The basic tab doesn't show cookie counter results.
    NOTREACHED();
  }
  if (pref_name == browsing_data::prefs::kDeleteCookies) {
    // Site data counter.
    browsing_data::BrowsingDataCounter::ResultInt origins =
        static_cast<const browsing_data::BrowsingDataCounter::FinishedResult*>(
            result)
            ->Value();

    // Determines whether or not to show the count with exception message.
    int del_cookie_counter_msg_id = IDS_DEL_COOKIES_COUNTER_ADVANCED;

#if defined(OS_CHROMEOS)
    if (AccountConsistencyModeManager::IsMirrorEnabledForProfile(profile)) {
#else  // !defined(OS_CHROMEOS)
    if (AccountConsistencyModeManager::IsDiceEnabledForProfile(profile)) {
#endif
      del_cookie_counter_msg_id =
          IDS_DEL_COOKIES_COUNTER_ADVANCED_WITH_EXCEPTION;
    }

    return l10n_util::GetPluralStringFUTF16(del_cookie_counter_msg_id, origins);
  }

  if (pref_name == browsing_data::prefs::kDeleteMediaLicenses) {
    const MediaLicensesCounter::MediaLicenseResult* media_license_result =
        static_cast<const MediaLicensesCounter::MediaLicenseResult*>(result);
    if (media_license_result->Value() > 0) {
     return l10n_util::GetStringFUTF16(
          IDS_DEL_MEDIA_LICENSES_COUNTER_SITE_COMMENT,
          base::UTF8ToUTF16(media_license_result->GetOneOrigin()));
    }
    return l10n_util::GetStringUTF16(
        IDS_DEL_MEDIA_LICENSES_COUNTER_GENERAL_COMMENT);
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (pref_name == browsing_data::prefs::kDeleteHostedAppsData) {
    // Hosted apps counter.
    const HostedAppsCounter::HostedAppsResult* hosted_apps_result =
        static_cast<const HostedAppsCounter::HostedAppsResult*>(result);
    int hosted_apps_count = hosted_apps_result->Value();

    DCHECK_GE(hosted_apps_result->Value(),
              base::checked_cast<browsing_data::BrowsingDataCounter::ResultInt>(
                  hosted_apps_result->examples().size()));

    std::vector<base::string16> replacements;
    if (hosted_apps_count > 0) {
      replacements.push_back(                                     // App1,
          base::UTF8ToUTF16(hosted_apps_result->examples()[0]));
    }
    if (hosted_apps_count > 1) {
      replacements.push_back(
          base::UTF8ToUTF16(hosted_apps_result->examples()[1]));  // App2,
    }
    if (hosted_apps_count > 2) {
      replacements.push_back(l10n_util::GetPluralStringFUTF16(  // and X-2 more.
          IDS_DEL_HOSTED_APPS_COUNTER_AND_X_MORE,
          hosted_apps_count - 2));
    }

    // The output string has both the number placeholder (#) and substitution
    // placeholders ($1, $2, $3). First fetch the correct plural string first,
    // then substitute the $ placeholders.
    return base::ReplaceStringPlaceholders(
        l10n_util::GetPluralStringFUTF16(
            IDS_DEL_HOSTED_APPS_COUNTER, hosted_apps_count),
        replacements,
        nullptr);
  }
#endif

  return browsing_data::GetCounterTextFromResult(result);
}
