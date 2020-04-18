// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/api/i18n/i18n_api.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "chromecast/common/extensions_api/i18n.h"
#include "components/prefs/pref_service.h"

namespace GetAcceptLanguages = extensions::cast::api::i18n::GetAcceptLanguages;

namespace extensions {

ExtensionFunction::ResponseAction I18nGetAcceptLanguagesFunction::Run() {
  std::vector<std::string> accept_languages;

  // TODO(rmrossi) Stubbed for now.
  return RespondNow(
      ArgumentList(GetAcceptLanguages::Results::Create(accept_languages)));
}

}  // namespace extensions
