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

#include <libaddressinput/address_normalizer.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>
#include <libaddressinput/preload_supplier.h>

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "lookup_key.h"
#include "rule.h"
#include "util/size.h"
#include "util/string_compare.h"

namespace i18n {
namespace addressinput {

AddressNormalizer::AddressNormalizer(const PreloadSupplier* supplier)
    : supplier_(supplier),
      compare_(new StringCompare) {
  assert(supplier_ != nullptr);
}

AddressNormalizer::~AddressNormalizer() {}

void AddressNormalizer::Normalize(AddressData* address) const {
  assert(address != nullptr);
  assert(supplier_->IsLoaded(address->region_code));

  AddressData region_address;
  region_address.region_code = address->region_code;
  LookupKey parent_key;
  parent_key.FromAddress(region_address);
  const Rule* parent_rule = supplier_->GetRule(parent_key);
  // Since we only set the |region_code| in the |region_address|, and the rule
  // for the |region_code| is already loaded, |parent_rule| should not be null.
  assert(parent_rule != nullptr);

  std::vector<std::string> languages(parent_rule->GetLanguages());

  if (languages.empty()) {
    languages.push_back("");
  } else {
    languages[0] = "";  // The default language doesn't need a tag on the id.
  }


  LookupKey lookup_key;
  for (size_t depth = 1; depth < size(LookupKey::kHierarchy); ++depth) {
    AddressField field = LookupKey::kHierarchy[depth];
    if (address->IsFieldEmpty(field)) {
      return;
    }
    const std::string& field_value = address->GetFieldValue(field);
    bool no_match_found_yet = true;

    const std::vector<std::string>& sub_keys = parent_rule->GetSubKeys();

    for (size_t i = 0; i < sub_keys.size(); i++) {
      const std::string& sub_key = sub_keys[i];
      if (!no_match_found_yet)
        break;
      for (const std::string& language : languages) {
        lookup_key.set_language(language);
        lookup_key.FromLookupKey(parent_key, sub_key);
        const Rule* rule = supplier_->GetRule(lookup_key);

        // A rule with key = |subkey| and specified |language| was expected to
        // be found in a certain format (e.g. data/CA/QC--fr), but it was not.
        // This is due to a possible inconsistency in the data format.
        if (rule == nullptr) continue;

        bool matches_latin_name =
            compare_->NaturalEquals(field_value, rule->GetLatinName());
        bool matches_local_name_id =
            compare_->NaturalEquals(field_value, sub_key) ||
            compare_->NaturalEquals(field_value, rule->GetName());
        if (matches_latin_name || matches_local_name_id) {
          address->SetFieldValue(
              field, matches_latin_name ? rule->GetLatinName() : sub_key);
          no_match_found_yet = false;
          parent_key.FromLookupKey(parent_key, sub_key);
          parent_rule = supplier_->GetRule(parent_key);
          assert(parent_rule != nullptr);
          break;
        }
      }
    }
    if (no_match_found_yet) {
      return;  // Abort search.
    }
  }
}

}  // namespace addressinput
}  // namespace i18n
