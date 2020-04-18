// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/public/cpp/lib/util.h"

#include <utility>

#include "base/values.h"

namespace prefs {
namespace {

enum class MatchType { kNo, kExact, kPrefix };

MatchType MatchPref(base::StringPiece string, base::StringPiece prefix) {
  if (string.substr(0, prefix.size()) != prefix)
    return MatchType::kNo;

  if (string.size() == prefix.size())
    return MatchType::kExact;

  return string[prefix.size()] == '.' ? MatchType::kPrefix : MatchType::kNo;
}

std::unique_ptr<base::DictionaryValue> FilterPrefsImpl(
    std::unique_ptr<base::DictionaryValue> prefs,
    const std::set<std::string>& observed_prefs,
    const std::string& prefix) {
  if (!prefs)
    return nullptr;

  auto filtered_value = std::make_unique<base::DictionaryValue>();
  std::string full_path = prefix;
  for (auto& pref : *prefs) {
    full_path.resize(prefix.size());
    if (full_path.empty()) {
      full_path = pref.first;
    } else {
      full_path.reserve(full_path.size() + 1 + pref.first.size());
      full_path += ".";
      full_path += pref.first;
    }
    auto it = observed_prefs.lower_bound(full_path);
    if (it == observed_prefs.end())
      continue;

    auto result = MatchPref(*it, full_path);
    switch (result) {
      case MatchType::kNo:
        break;
      case MatchType::kExact:
        filtered_value->Set(pref.first, std::move(pref.second));
        break;
      case MatchType::kPrefix:
        auto filtered_subpref =
            FilterPrefsImpl(base::DictionaryValue::From(std::move(pref.second)),
                            observed_prefs, full_path);
        if (filtered_subpref)
          filtered_value->Set(pref.first, std::move(filtered_subpref));
        break;
    }
  }
  return filtered_value;
}

}  // namespace

void SetValue(base::Value* dictionary_value,
              const std::vector<base::StringPiece>& path_components,
              std::unique_ptr<base::Value> value) {
  for (size_t i = 0; i < path_components.size() - 1; ++i) {
    base::Value* found = dictionary_value->FindKeyOfType(
        path_components[i], base::Value::Type::DICTIONARY);
    if (found) {
      dictionary_value = found;
    } else {
      dictionary_value = dictionary_value->SetKey(
          path_components[i], base::Value(base::Value::Type::DICTIONARY));
    }
  }
  const auto& key = path_components.back();
  if (value) {
    dictionary_value->SetKey(key,
                             base::Value::FromUniquePtrValue(std::move(value)));
  } else {
    dictionary_value->RemoveKey(key);
  }
}

std::unique_ptr<base::DictionaryValue> FilterPrefs(
    std::unique_ptr<base::DictionaryValue> prefs,
    const std::set<std::string>& observed_prefs) {
  return FilterPrefsImpl(std::move(prefs), observed_prefs, std::string());
}

}  // namespace prefs
