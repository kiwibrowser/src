// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lookup_key.h"

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "language.h"
#include "region_data_constants.h"
#include "rule.h"
#include "util/cctype_tolower_equal.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

namespace {

const char kSlashDelim[] = "/";
const char kDashDelim[] = "--";
const char kData[] = "data";
const char kUnknown[] = "ZZ";

// Assume the language_tag has had "Latn" script removed when this is called.
bool ShouldSetLanguageForKey(const std::string& language_tag,
                             const std::string& region_code) {
  // We only need a language in the key if there is subregion data at all.
  if (RegionDataConstants::GetMaxLookupKeyDepth(region_code) == 0) {
    return false;
  }
  Rule rule;
  rule.CopyFrom(Rule::GetDefault());
  // TODO: Pre-parse the rules and have a map from region code to rule.
  if (!rule.ParseSerializedRule(
          RegionDataConstants::GetRegionData(region_code))) {
    return false;
  }
  const std::vector<std::string>& languages = rule.GetLanguages();
  // Do not add the default language (we want "data/US", not "data/US--en").
  // (empty should not happen here because we have some sub-region data).
  if (languages.empty() || languages[0] == language_tag) {
    return false;
  }
  // Finally, only return true if the language is one of the remaining ones.
  return std::find_if(languages.begin() + 1, languages.end(),
                      std::bind2nd(EqualToTolowerString(), language_tag)) !=
         languages.end();
}

}  // namespace

const AddressField LookupKey::kHierarchy[] = {
  COUNTRY,
  ADMIN_AREA,
  LOCALITY,
  DEPENDENT_LOCALITY
};

LookupKey::LookupKey() {
}

LookupKey::~LookupKey() {
}

void LookupKey::FromAddress(const AddressData& address) {
  nodes_.clear();
  if (address.region_code.empty()) {
    nodes_.insert(std::make_pair(COUNTRY, kUnknown));
  } else {
    for (size_t i = 0; i < size(kHierarchy); ++i) {
      AddressField field = kHierarchy[i];
      if (address.IsFieldEmpty(field)) {
        // It would be impossible to find any data for an empty field value.
        break;
      }
      const std::string& value = address.GetFieldValue(field);
      if (value.find('/') != std::string::npos) {
        // The address metadata server does not have data for any fields with a
        // slash in their value. The slash is used as a syntax character in the
        // lookup key format.
        break;
      }
      nodes_.insert(std::make_pair(field, value));
    }
  }
  Language address_language(address.language_code);
  std::string language_tag_no_latn = address_language.has_latin_script
                                         ? address_language.base
                                         : address_language.tag;
  if (ShouldSetLanguageForKey(language_tag_no_latn, address.region_code)) {
    language_ = language_tag_no_latn;
  }
}

void LookupKey::FromLookupKey(const LookupKey& parent,
                              const std::string& child_node) {
  assert(parent.nodes_.size() < size(kHierarchy));
  assert(!child_node.empty());

  // Copy its nodes if this isn't the parent object.
  if (this != &parent) nodes_ = parent.nodes_;
  AddressField child_field = kHierarchy[nodes_.size()];
  nodes_.insert(std::make_pair(child_field, child_node));
}

std::string LookupKey::ToKeyString(size_t max_depth) const {
  assert(max_depth < size(kHierarchy));
  std::string key_string(kData);

  for (size_t i = 0; i <= max_depth; ++i) {
    AddressField field = kHierarchy[i];
    std::map<AddressField, std::string>::const_iterator it = nodes_.find(field);
    if (it == nodes_.end()) {
      break;
    }
    key_string.append(kSlashDelim);
    key_string.append(it->second);
  }
  if (!language_.empty()) {
    key_string.append(kDashDelim);
    key_string.append(language_);
  }
  return key_string;
}

const std::string& LookupKey::GetRegionCode() const {
  std::map<AddressField, std::string>::const_iterator it = nodes_.find(COUNTRY);
  assert(it != nodes_.end());
  return it->second;
}

size_t LookupKey::GetDepth() const {
  size_t depth = nodes_.size() - 1;
  assert(depth < size(kHierarchy));
  return depth;
}

}  // namespace addressinput
}  // namespace i18n
