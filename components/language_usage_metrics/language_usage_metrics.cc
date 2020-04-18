// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language_usage_metrics/language_usage_metrics.h"

#include <algorithm>

#include "base/metrics/histogram_functions.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"

namespace {
void RecordAcceptLanguage(int language_code) {
  base::UmaHistogramSparse("LanguageUsage.AcceptLanguage", language_code);
}
}  // namespace

namespace language_usage_metrics {

// static
void LanguageUsageMetrics::RecordAcceptLanguages(
    const std::string& accept_languages) {
  std::set<int> languages;
  ParseAcceptLanguages(accept_languages, &languages);
  std::for_each(languages.begin(), languages.end(), RecordAcceptLanguage);
}

// static
void LanguageUsageMetrics::RecordApplicationLanguage(
    const std::string& application_locale) {
  const int language_code = ToLanguageCode(application_locale);
  if (language_code != 0)
    base::UmaHistogramSparse("LanguageUsage.ApplicationLanguage",
                             language_code);
}

// static
int LanguageUsageMetrics::ToLanguageCode(const std::string& locale) {
  base::StringTokenizer parts(locale, "-_");
  if (!parts.GetNext())
    return 0;

  std::string language_part = base::ToLowerASCII(parts.token());

  int language_code = 0;
  for (std::string::iterator it = language_part.begin();
       it != language_part.end(); ++it) {
    char ch = *it;
    if (ch < 'a' || 'z' < ch)
      return 0;

    language_code <<= 8;
    language_code += ch;
  }

  return language_code;
}

// static
void LanguageUsageMetrics::ParseAcceptLanguages(
    const std::string& accept_languages,
    std::set<int>* languages) {
  languages->clear();
  base::StringTokenizer locales(accept_languages, ",");
  while (locales.GetNext()) {
    const int language_code = ToLanguageCode(locales.token());
    if (language_code != 0)
      languages->insert(language_code);
  }
}

}  // namespace language_usage_metrics
