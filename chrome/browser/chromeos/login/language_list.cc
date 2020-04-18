// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/language_list.h"

#include <stddef.h>

#include "base/i18n/rtl.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

LanguageList::LanguageList() {
  // Enumerate the languages we know about.
  const std::vector<std::string>& locale_codes =
      l10n_util::GetAvailableLocales();
  InitNativeNames(locale_codes);
}

LanguageList::~LanguageList() {}

base::string16 LanguageList::GetLanguageNameAt(int index) const {
  DCHECK_LT(index, languages_count());
  LocaleDataMap::const_iterator locale_data =
      native_names_.find(locale_names_[index]);
  DCHECK(locale_data != native_names_.end());

  // If the name is the same in the native language and local language,
  // don't show it twice.
  if (locale_data->second.native_name == locale_names_[index])
    return locale_data->second.native_name;

  // We must add directionality formatting to both the native name and the
  // locale name in order to avoid text rendering problems such as misplaced
  // parentheses or languages appearing in the wrong order.
  base::string16 locale_name = locale_names_[index];
  base::i18n::AdjustStringForLocaleDirection(&locale_name);

  base::string16 native_name = locale_data->second.native_name;
  base::i18n::AdjustStringForLocaleDirection(&native_name);

  // We used to have a localizable template here, but none of translators
  // changed the format. We also want to switch the order of locale_name
  // and native_name without going back to translators.
  std::string formatted_item;
  base::SStringPrintf(&formatted_item, "%s - %s",
                      base::UTF16ToUTF8(locale_name).c_str(),
                      base::UTF16ToUTF8(native_name).c_str());
  if (base::i18n::IsRTL())
    // Somehow combo box (even with LAYOUTRTL flag) doesn't get this
    // right so we add RTL BDO (U+202E) to set the direction
    // explicitly.
    formatted_item.insert(0, "\xE2\x80\xAE");  // U+202E = UTF-8 0xE280AE
  return base::UTF8ToUTF16(formatted_item);
}

std::string LanguageList::GetLocaleFromIndex(int index) const {
  DCHECK(static_cast<int>(locale_names_.size()) > index);
  LocaleDataMap::const_iterator locale_data =
      native_names_.find(locale_names_[index]);
  DCHECK(locale_data != native_names_.end());
  return locale_data->second.locale_code;
}

int LanguageList::GetIndexFromLocale(const std::string& locale) const {
  for (size_t i = 0; i < locale_names_.size(); ++i) {
    LocaleDataMap::const_iterator locale_data =
        native_names_.find(locale_names_[i]);
    DCHECK(locale_data != native_names_.end());
    if (locale_data->second.locale_code == locale)
      return static_cast<int>(i);
  }
  return -1;
}

void LanguageList::CopySpecifiedLanguagesUp(const std::string& locale_codes) {
  DCHECK(!locale_names_.empty());
  for (const std::string& code : base::SplitString(
           locale_codes, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    const int locale_index = GetIndexFromLocale(code);
    CHECK_NE(locale_index, -1);
    locale_names_.insert(locale_names_.begin(), locale_names_[locale_index]);
  }
}

void LanguageList::InitNativeNames(
    const std::vector<std::string>& locale_codes) {
  const std::string app_locale = g_browser_process->GetApplicationLocale();
  for (size_t i = 0; i < locale_codes.size(); ++i) {
    const char* locale_code = locale_codes[i].c_str();

    // TODO(jungshik): Even though these strings are used for the UI,
    // the old code does not add an RTL mark for RTL locales. Make sure
    // that it's ok without that.
    base::string16 name_in_current_ui =
        l10n_util::GetDisplayNameForLocale(locale_code, app_locale, false);
    base::string16 name_native =
        l10n_util::GetDisplayNameForLocale(locale_code, locale_code, false);

    locale_names_.push_back(name_in_current_ui);
    native_names_[name_in_current_ui] =
        LocaleData(name_native, locale_codes[i]);
  }

  // Sort using locale specific sorter.
  l10n_util::SortStrings16(g_browser_process->GetApplicationLocale(),
                           &locale_names_);
}

}  // namespace chromeos
