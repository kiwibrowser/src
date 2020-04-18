// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_HANDLER_H_
#define CHROME_COMMON_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_HANDLER_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handler.h"

namespace extensions {

// Parses the "system_indicator" manifest key.
class SystemIndicatorHandler : public ManifestHandler {
 public:
  SystemIndicatorHandler();
  ~SystemIndicatorHandler() override;

  bool Parse(Extension* extension, base::string16* error) override;

 private:
  base::span<const char* const> Keys() const override;

  DISALLOW_COPY_AND_ASSIGN(SystemIndicatorHandler);
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_API_SYSTEM_INDICATOR_SYSTEM_INDICATOR_HANDLER_H_
