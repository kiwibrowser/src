// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_accept_language_settings.h"

#include <unordered_set>

#include "base/feature_list.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/chrome_features.h"
#include "net/http/http_util.h"

namespace chrome_accept_language_settings {
namespace {

// Helper class that builds the list of languages for the Accept-Language
// headers.
// The output is a comma-separated list of languages as string.
// Duplicates are removed.
class AcceptLanguageBuilder {
 public:
  // Adds a language to the string.
  // Duplicates are ignored.
  void AddLanguageCode(const std::string& language) {
    if (seen_.find(language) == seen_.end()) {
      if (str_.empty()) {
        base::StringAppendF(&str_, "%s", language.c_str());
      } else {
        base::StringAppendF(&str_, ",%s", language.c_str());
      }
      seen_.insert(language);
    }
  }

  // Returns the string constructed up to this point.
  std::string GetString() const { return str_; }

 private:
  // The string that contains the list of languages, comma-separated.
  std::string str_;
  // Set the remove duplicates.
  std::unordered_set<std::string> seen_;
};

// Extract the base language code from a language code.
// If there is no '-' in the code, the original code is returned.
std::string GetBaseLanguageCode(const std::string& language_code) {
  const std::vector<std::string> tokens = base::SplitString(
      language_code, "-", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  return tokens.empty() ? "" : tokens[0];
}

}  // namespace

std::string ComputeAcceptLanguageFromPref(const std::string& language_pref) {
  std::string accept_languages_str =
      base::FeatureList::IsEnabled(features::kUseNewAcceptLanguageHeader)
          ? ExpandLanguageList(language_pref)
          : language_pref;
  return net::HttpUtil::GenerateAcceptLanguageHeader(accept_languages_str);
}

std::string ExpandLanguageList(const std::string& language_prefs) {
  const std::vector<std::string> languages = base::SplitString(
      language_prefs, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (languages.empty())
    return "";

  AcceptLanguageBuilder builder;

  const int size = languages.size();
  for (int i = 0; i < size; ++i) {
    const std::string& language = languages[i];
    builder.AddLanguageCode(language);

    // Extract the base language
    const std::string& base_language = GetBaseLanguageCode(language);

    // Look ahead and add the base language if the next language is not part
    // of the same family.
    const int j = i + 1;
    if (j >= size || GetBaseLanguageCode(languages[j]) != base_language) {
      builder.AddLanguageCode(base_language);
    }
  }

  return builder.GetString();
}

}  // namespace chrome_accept_language_settings
