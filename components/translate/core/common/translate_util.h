// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATE_UTIL_H_
#define COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATE_UTIL_H_

#include <string>
#include <vector>

class GURL;

namespace translate {

// Isolated world sets following security-origin by default.
extern const char kSecurityOrigin[];

// Converts language code synonym to use at Translate server.
//
// The same logic exists at language_options.js, and please keep consistency
// with the JavaScript file.
void ToTranslateLanguageSynonym(std::string* language);

// Converts language code synonym to use at Chrome internal.
void ToChromeLanguageSynonym(std::string* language);

// Gets Security origin with which Translate runs. This is used both for
// language checks and to obtain the list of available languages.
GURL GetTranslateSecurityOrigin();

}  // namespace translate

#endif  // COMPONENTS_TRANSLATE_CORE_COMMON_TRANSLATE_UTIL_H_
