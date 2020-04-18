// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PUBLIC_CPP_LIB_UTIL_H_
#define SERVICES_PREFERENCES_PUBLIC_CPP_LIB_UTIL_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/strings/string_piece.h"

namespace base {
class DictionaryValue;
class Value;
}

namespace prefs {

// Sets a nested value inside the dictionary, specified by |path_components|.
// Creates nested dictionaries as needed.
void SetValue(base::Value* dictionary_value,
              const std::vector<base::StringPiece>& path_components,
              std::unique_ptr<base::Value> value);

// Filters |prefs| to paths contained within |observed_prefs|.
std::unique_ptr<base::DictionaryValue> FilterPrefs(
    std::unique_ptr<base::DictionaryValue> prefs,
    const std::set<std::string>& observed_prefs);

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PUBLIC_CPP_LIB_UTIL_H_
