// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_LANGUAGE_LIST_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_LANGUAGE_LIST_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"

namespace chromeos {

// LanguageList is used to enumerate native names corresponding to the
// language code (e.g. English (United States) for en-US).
class LanguageList {
 public:
  LanguageList();
  ~LanguageList();

  // Returns the number of locale names.
  int languages_count() const { return static_cast<int>(locale_names_.size()); }

  // Returns the language for the given |index|.
  base::string16 GetLanguageNameAt(int index) const;

  // Return the locale for the given |index|. E.g., may return pt-BR.
  std::string GetLocaleFromIndex(int index) const;

  // Returns the index for the given |locale|. Returns -1 if it's not found.
  int GetIndexFromLocale(const std::string& locale) const;

  // Duplicates specified languages at the beginning of the list for easier
  // access.
  void CopySpecifiedLanguagesUp(const std::string& locale_codes);

 private:
  struct LocaleData {
    LocaleData() {}
    LocaleData(const base::string16& name, const std::string& code)
        : native_name(name), locale_code(code) {}

    base::string16 native_name;
    std::string locale_code;  // E.g., en-us.
  };

  typedef std::map<base::string16, LocaleData> LocaleDataMap;

  void InitNativeNames(const std::vector<std::string>& locale_codes);

  // The names of all the locales in the current application locale.
  std::vector<base::string16> locale_names_;

  // A map of some extra data (LocaleData) keyed off the name of the locale.
  LocaleDataMap native_names_;

  DISALLOW_COPY_AND_ASSIGN(LanguageList);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_LANGUAGE_LIST_H_
