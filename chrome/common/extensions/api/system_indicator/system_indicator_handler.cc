// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/system_indicator/system_indicator_handler.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

SystemIndicatorHandler::SystemIndicatorHandler() {
}

SystemIndicatorHandler::~SystemIndicatorHandler() {
}

bool SystemIndicatorHandler::Parse(Extension* extension,
                                   base::string16* error) {
  const base::DictionaryValue* system_indicator_value = NULL;
  if (!extension->manifest()->GetDictionary(
          manifest_keys::kSystemIndicator, &system_indicator_value)) {
    *error = base::ASCIIToUTF16(manifest_errors::kInvalidSystemIndicator);
    return false;
  }

  std::unique_ptr<ActionInfo> action_info =
      ActionInfo::Load(extension, system_indicator_value, error);

  if (!action_info.get())
    return false;

  ActionInfo::SetSystemIndicatorInfo(extension, action_info.release());
  return true;
}

base::span<const char* const> SystemIndicatorHandler::Keys() const {
  static constexpr const char* kKeys[] = {manifest_keys::kSystemIndicator};
  return kKeys;
}

}  // namespace extensions
