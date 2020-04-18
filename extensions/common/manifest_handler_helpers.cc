// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handler_helpers.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace errors = manifest_errors;

namespace manifest_handler_helpers {

bool NormalizeAndValidatePath(std::string* path) {
  size_t first_non_slash = path->find_first_not_of('/');
  if (first_non_slash == std::string::npos) {
    *path = "";
    return false;
  }

  *path = path->substr(first_non_slash);
  return true;
}

bool LoadIconsFromDictionary(const base::DictionaryValue* icons_value,
                             ExtensionIconSet* icons,
                             base::string16* error) {
  DCHECK(icons);
  DCHECK(error);
  for (base::DictionaryValue::Iterator iterator(*icons_value);
       !iterator.IsAtEnd(); iterator.Advance()) {
    int size = 0;
    std::string icon_path;
    if (!base::StringToInt(iterator.key(), &size) || size <= 0 ||
        size > extension_misc::EXTENSION_ICON_GIGANTOR * 4) {
      *error = ErrorUtils::FormatErrorMessageUTF16(errors::kInvalidIconKey,
                                                   iterator.key());
      return false;
    }
    if (!iterator.value().GetAsString(&icon_path) ||
        !NormalizeAndValidatePath(&icon_path)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(errors::kInvalidIconPath,
                                                   iterator.key());
      return false;
    }

    icons->Add(size, icon_path);
  }
  return true;
}

}  // namespace manifest_handler_helpers

}  // namespace extensions
