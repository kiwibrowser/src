// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CORE_LANGUAGE_DETECTION_LANGUAGE_DETECTION_UTIL_H_
#define COMPONENTS_TRANSLATE_CORE_LANGUAGE_DETECTION_LANGUAGE_DETECTION_UTIL_H_

#include <string>

#include "base/strings/string16.h"

namespace translate {

// Determines content page language from Content-Language code and contents.
std::string DeterminePageLanguage(const std::string& code,
                                  const std::string& html_lang,
                                  const base::string16& contents,
                                  std::string* cld_language,
                                  bool* is_cld_reliable);

// Corrects language code if it contains well-known mistakes.
// Called only by tests.
void CorrectLanguageCodeTypo(std::string* code);

// Checks if the language code's format is valid.
// Called only by tests.
bool IsValidLanguageCode(const std::string& code);

// Checks if languages are matched, or similar. This function returns true
// against a language pair containing a language which is difficult for CLD to
// distinguish.
// Called only by tests.
bool IsSameOrSimilarLanguages(const std::string& page_language,
                              const std::string& cld_language);

// Checks if languages pair is one of well-known pairs of wrong server
// configuration.
// Called only by tests.
bool MaybeServerWrongConfiguration(const std::string& page_language,
                                   const std::string& cld_language);

// Returns true if the specified language often has the wrong server
// configuration language, false otherwise.
bool IsServerWrongConfigurationLanguage(const std::string& language);

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CORE_LANGUAGE_DETECTION_LANGUAGE_DETECTION_UTIL_H_
