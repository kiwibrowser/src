// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_MANIFEST_HANDLERS_EXTENSION_ACTION_HANDLER_H_
#define CHROME_COMMON_EXTENSIONS_MANIFEST_HANDLERS_EXTENSION_ACTION_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "extensions/common/manifest_handler.h"

namespace extensions {

// Parses the "page_action" and "browser_action" manifest keys.
class ExtensionActionHandler : public ManifestHandler {
 public:
  ExtensionActionHandler();
  ~ExtensionActionHandler() override;

  bool Parse(Extension* extension, base::string16* error) override;
  bool Validate(const Extension* extension,
                std::string* error,
                std::vector<InstallWarning>* warnings) const override;

 private:
  bool AlwaysParseForType(Manifest::Type type) const override;
  base::span<const char* const> Keys() const override;

  DISALLOW_COPY_AND_ASSIGN(ExtensionActionHandler);
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_MANIFEST_HANDLERS_EXTENSION_ACTION_HANDLER_H_
