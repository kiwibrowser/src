// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/common/translate_util.h"

#include <stddef.h>
#include <algorithm>
#include <set>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "components/language/core/common/locale_util.h"
#include "components/translate/core/common/translate_switches.h"
#include "url/gurl.h"

namespace translate {

struct LanguageCodePair {
  // Code used in supporting list of Translate.
  const char* const translate_language;

  // Code used in Chrome internal.
  const char* const chrome_language;
};

// Some languages are treated as same languages in Translate even though they
// are different to be exact.
//
// If this table is updated, please sync this with the synonym table in
// chrome/browser/resources/settings/languages_page/languages.js.
const LanguageCodePair kLanguageCodeSimilitudes[] = {
  {"no", "nb"},
  {"tl", "fil"},
};

// Some languages have changed codes over the years and sometimes the older
// codes are used, so we must see them as synonyms.
//
// If this table is updated, please sync this with the synonym table in
// chrome/browser/resources/settings/languages_page/languages.js.
const LanguageCodePair kLanguageCodeSynonyms[] = {
  {"iw", "he"},
  {"jw", "jv"},
};

// Some Chinese language codes are compatible with zh-TW or zh-CN in terms of
// Translate.
//
// If this table is updated, please sync this with the synonym table in
// chrome/browser/resources/settings/languages_page/languages.js.
const LanguageCodePair kLanguageCodeChineseCompatiblePairs[] = {
    {"zh-TW", "zh-HK"},
    {"zh-TW", "zh-MO"},
    {"zh-CN", "zh-SG"},
};

const char kSecurityOrigin[] = "https://translate.googleapis.com/";

void ToTranslateLanguageSynonym(std::string* language) {
  for (size_t i = 0; i < arraysize(kLanguageCodeSimilitudes); ++i) {
    if (*language == kLanguageCodeSimilitudes[i].chrome_language) {
      *language = kLanguageCodeSimilitudes[i].translate_language;
      return;
    }
  }

  std::string main_part, tail_part;
  language::SplitIntoMainAndTail(*language, &main_part, &tail_part);
  if (main_part.empty())
    return;

  // Chinese is a special case: we do not return the main_part only.
  // There is not a single base language, but two: traditional and simplified.
  // The kLanguageCodeChineseCompatiblePairs list contains the relation between
  // various Chinese locales. We need to return the code from that mapping
  // instead of the main_part.
  // Note that "zh" does not have any mapping and as such we leave it as is. See
  // https://crbug/798512 for more info.
  for (size_t i = 0; i < arraysize(kLanguageCodeChineseCompatiblePairs); ++i) {
    if (*language == kLanguageCodeChineseCompatiblePairs[i].chrome_language) {
      *language = kLanguageCodeChineseCompatiblePairs[i].translate_language;
      return;
    }
  }
  if (main_part == "zh") {
    return;
  }

  // Apply linear search here because number of items in the list is just four.
  for (size_t i = 0; i < arraysize(kLanguageCodeSynonyms); ++i) {
    if (main_part == kLanguageCodeSynonyms[i].chrome_language) {
      main_part = std::string(kLanguageCodeSynonyms[i].translate_language);
      break;
    }
  }

  *language = main_part;
}

void ToChromeLanguageSynonym(std::string* language) {
  for (size_t i = 0; i < arraysize(kLanguageCodeSimilitudes); ++i) {
    if (*language == kLanguageCodeSimilitudes[i].translate_language) {
      *language = kLanguageCodeSimilitudes[i].chrome_language;
      return;
    }
  }

  std::string main_part, tail_part;
  language::SplitIntoMainAndTail(*language, &main_part, &tail_part);
  if (main_part.empty())
    return;

  // Apply liner search here because number of items in the list is just four.
  for (size_t i = 0; i < arraysize(kLanguageCodeSynonyms); ++i) {
    if (main_part == kLanguageCodeSynonyms[i].translate_language) {
      main_part = std::string(kLanguageCodeSynonyms[i].chrome_language);
      break;
    }
  }

  *language = main_part + tail_part;
}

GURL GetTranslateSecurityOrigin() {
  std::string security_origin(kSecurityOrigin);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kTranslateSecurityOrigin)) {
    security_origin =
        command_line->GetSwitchValueASCII(switches::kTranslateSecurityOrigin);
  }
  return GURL(security_origin);
}

}  // namespace translate
