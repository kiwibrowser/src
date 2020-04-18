// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/i18n/i18n_api.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/i18n.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace GetAcceptLanguages = extensions::api::i18n::GetAcceptLanguages;

namespace extensions {

namespace {

// Errors.
static const char kEmptyAcceptLanguagesError[] = "accept-languages is empty.";

}

ExtensionFunction::ResponseAction I18nGetAcceptLanguagesFunction::Run() {
  std::string accept_languages = Profile::FromBrowserContext(browser_context())
                                     ->GetPrefs()
                                     ->GetString(prefs::kAcceptLanguages);
  // Currently, there are 2 ways to set browser's accept-languages: through UI
  // or directly modify the preference file. The accept-languages set through
  // UI is guranteed to be valid, and the accept-languages string returned from
  // profile()->GetPrefs()->GetString(prefs::kAcceptLanguages) is guranteed to
  // be valid and well-formed, which means each accept-langauge is a valid
  // code, and accept-languages are seperatd by "," without surrrounding
  // spaces. But we do not do any validation (either the format or the validity
  // of the language code) on accept-languages set through editing preference
  // file directly. So, here, we're adding extra checks to be resistant to
  // crashes caused by data corruption.
  if (accept_languages.empty())
    return RespondNow(Error(kEmptyAcceptLanguagesError));

  std::vector<std::string> languages = base::SplitString(
      accept_languages, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  languages.erase(std::remove(languages.begin(), languages.end(), ""),
                  languages.end());

  if (languages.empty())
    return RespondNow(Error(kEmptyAcceptLanguagesError));

  return RespondNow(
      ArgumentList(GetAcceptLanguages::Results::Create(languages)));
}

}  // namespace extensions
