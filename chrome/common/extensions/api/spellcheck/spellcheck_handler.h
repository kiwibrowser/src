// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_API_SPELLCHECK_SPELLCHECK_HANDLER_H_
#define CHROME_COMMON_EXTENSIONS_API_SPELLCHECK_SPELLCHECK_HANDLER_H_

#include "base/macros.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handler.h"

namespace extensions {

// This structure holds the information parsed by the SpellcheckHandler to be
// used in the SpellcheckAPI functions. It is stored on the extension.
struct SpellcheckDictionaryInfo : public extensions::Extension::ManifestData {
  SpellcheckDictionaryInfo();
  ~SpellcheckDictionaryInfo() override;

  std::string language;
  std::string locale;
  std::string path;
  std::string format;
};

// Parses the "spellcheck" manifest key.
class SpellcheckHandler : public ManifestHandler {
 public:
  SpellcheckHandler();
  ~SpellcheckHandler() override;

  bool Parse(Extension* extension, base::string16* error) override;

 private:
  base::span<const char* const> Keys() const override;

  DISALLOW_COPY_AND_ASSIGN(SpellcheckHandler);
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_API_SPELLCHECK_SPELLCHECK_HANDLER_H_
