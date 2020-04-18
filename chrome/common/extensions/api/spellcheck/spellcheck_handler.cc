// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/spellcheck/spellcheck_handler.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;

SpellcheckDictionaryInfo::SpellcheckDictionaryInfo() {
}

SpellcheckDictionaryInfo::~SpellcheckDictionaryInfo() {
}

SpellcheckHandler::SpellcheckHandler() {
}

SpellcheckHandler::~SpellcheckHandler() {
}

bool SpellcheckHandler::Parse(Extension* extension, base::string16* error) {
  const base::DictionaryValue* spellcheck_value = NULL;
  if (!extension->manifest()->GetDictionary(keys::kSpellcheck,
                                            &spellcheck_value)) {
    *error = base::ASCIIToUTF16(errors::kInvalidSpellcheck);
    return false;
  }
  std::unique_ptr<SpellcheckDictionaryInfo> spellcheck_info(
      new SpellcheckDictionaryInfo);
  if (!spellcheck_value->HasKey(keys::kSpellcheckDictionaryLanguage) ||
      !spellcheck_value->GetString(keys::kSpellcheckDictionaryLanguage,
                                  &spellcheck_info->language)) {
    *error = base::ASCIIToUTF16(errors::kInvalidSpellcheckDictionaryLanguage);
    return false;
  }
  if (!spellcheck_value->HasKey(keys::kSpellcheckDictionaryLocale) ||
      !spellcheck_value->GetString(keys::kSpellcheckDictionaryLocale,
                                  &spellcheck_info->locale)) {
    *error = base::ASCIIToUTF16(errors::kInvalidSpellcheckDictionaryLocale);
    return false;
  }
  if (!spellcheck_value->HasKey(keys::kSpellcheckDictionaryFormat) ||
      !spellcheck_value->GetString(keys::kSpellcheckDictionaryFormat,
                                  &spellcheck_info->format)) {
    *error = base::ASCIIToUTF16(errors::kInvalidSpellcheckDictionaryFormat);
    return false;
  }
  if (!spellcheck_value->HasKey(keys::kSpellcheckDictionaryPath) ||
      !spellcheck_value->GetString(keys::kSpellcheckDictionaryPath,
                                  &spellcheck_info->path)) {
    *error = base::ASCIIToUTF16(errors::kInvalidSpellcheckDictionaryPath);
    return false;
  }
  extension->SetManifestData(keys::kSpellcheck, std::move(spellcheck_info));
  return true;
}

base::span<const char* const> SpellcheckHandler::Keys() const {
  static constexpr const char* kKeys[] = {keys::kSpellcheck};
  return kKeys;
}

}  // namespace extensions
