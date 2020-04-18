// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_MANIFEST_HANDLER_HELPERS_H_
#define EXTENSIONS_COMMON_MANIFEST_HANDLER_HELPERS_H_

#include <memory>
#include <string>

#include "base/strings/string16.h"

class ExtensionIconSet;

namespace base {
class DictionaryValue;
}

namespace extensions {
namespace manifest_handler_helpers {

// Strips leading slashes from the file path. Returns true iff the final path is
// non empty.
bool NormalizeAndValidatePath(std::string* path);

// Loads icon paths defined in dictionary |icons_value| into ExtensionIconSet
// |icons|. |icons_value| is a dictionary value {icon size -> icon path}.
// Returns success. If load fails, |error| will be set.
bool LoadIconsFromDictionary(const base::DictionaryValue* icons_value,
                             ExtensionIconSet* icons,
                             base::string16* error);

}  // namespace manifest_handler_helpers
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_MANIFEST_HANDLER_HELPERS_H_
