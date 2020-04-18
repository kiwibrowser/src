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

#include <libaddressinput/region_data_builder.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/preload_supplier.h>
#include <libaddressinput/region_data.h>

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "language.h"
#include "lookup_key.h"
#include "region_data_constants.h"
#include "rule.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

namespace {

// The maximum depth of lookup keys.
static const size_t kLookupKeysMaxDepth = size(LookupKey::kHierarchy) - 1;

// Does not take ownership of |parent_region|, which is not allowed to be
// nullptr.
void BuildRegionTreeRecursively(
    const std::map<std::string, const Rule*>& rules,
    std::map<std::string, const Rule*>::const_iterator hint,
    const LookupKey& parent_key,
    RegionData* parent_region,
    const std::vector<std::string>& keys,
    bool prefer_latin_name,
    size_t region_max_depth) {
  assert(parent_region != nullptr);

  LookupKey lookup_key;
  for (std::vector<std::string>::const_iterator key_it = keys.begin();
       key_it != keys.end(); ++key_it) {
    lookup_key.FromLookupKey(parent_key, *key_it);
    const std::string& lookup_key_string =
        lookup_key.ToKeyString(kLookupKeysMaxDepth);

    ++hint;
    if (hint == rules.end() || hint->first != lookup_key_string) {
      hint = rules.find(lookup_key_string);
      if (hint == rules.end()) {
        return;
      }
    }

    const Rule* rule = hint->second;
    assert(rule != nullptr);

    const std::string& local_name = rule->GetName().empty()
        ? *key_it : rule->GetName();
    const std::string& name =
        prefer_latin_name && !rule->GetLatinName().empty()
            ? rule->GetLatinName() : local_name;
    RegionData* region = parent_region->AddSubRegion(*key_it, name);

    if (!rule->GetSubKeys().empty() &&
        region_max_depth > parent_key.GetDepth()) {
      BuildRegionTreeRecursively(rules,
                                 hint,
                                 lookup_key,
                                 region,
                                 rule->GetSubKeys(),
                                 prefer_latin_name,
                                 region_max_depth);
    }
  }
}

// The caller owns the result.
RegionData* BuildRegion(const std::map<std::string, const Rule*>& rules,
                        const std::string& region_code,
                        const Language& language) {
  AddressData address;
  address.region_code = region_code;

  LookupKey lookup_key;
  lookup_key.FromAddress(address);

  std::map<std::string, const Rule*>::const_iterator hint =
      rules.find(lookup_key.ToKeyString(kLookupKeysMaxDepth));
  assert(hint != rules.end());

  const Rule* rule = hint->second;
  assert(rule != nullptr);

  RegionData* region = new RegionData(region_code);

  // If there are sub-keys for field X, but field X is not used in this region
  // code, then these sub-keys are skipped over. For example, CH has sub-keys
  // for field ADMIN_AREA, but CH does not use ADMIN_AREA field.
  size_t region_max_depth =
      RegionDataConstants::GetMaxLookupKeyDepth(region_code);
  if (region_max_depth > 0) {
    BuildRegionTreeRecursively(rules,
                               hint,
                               lookup_key,
                               region,
                               rule->GetSubKeys(),
                               language.has_latin_script,
                               region_max_depth);
  }

  return region;
}

}  // namespace

RegionDataBuilder::RegionDataBuilder(PreloadSupplier* supplier)
    : supplier_(supplier),
      cache_() {
  assert(supplier_ != nullptr);
}

RegionDataBuilder::~RegionDataBuilder() {
  for (RegionCodeDataMap::const_iterator region_it = cache_.begin();
       region_it != cache_.end(); ++region_it) {
    for (LanguageRegionMap::const_iterator
         language_it = region_it->second->begin();
         language_it != region_it->second->end(); ++language_it) {
      delete language_it->second;
    }
    delete region_it->second;
  }
}

const RegionData& RegionDataBuilder::Build(
    const std::string& region_code,
    const std::string& ui_language_tag,
    std::string* best_region_tree_language_tag) {
  assert(supplier_->IsLoaded(region_code));
  assert(best_region_tree_language_tag != nullptr);

  // Look up the region tree in cache first before building it.
  RegionCodeDataMap::const_iterator region_it = cache_.find(region_code);
  if (region_it == cache_.end()) {
    region_it =
        cache_.insert(std::make_pair(region_code, new LanguageRegionMap)).first;
  }

  // No need to copy from default rule first, because only languages and Latin
  // format are going to be used, which do not exist in the default rule.
  Rule rule;
  rule.ParseSerializedRule(RegionDataConstants::GetRegionData(region_code));
  static const Language kUndefinedLanguage("und");
  const Language& best_language =
      rule.GetLanguages().empty()
          ? kUndefinedLanguage
          : ChooseBestAddressLanguage(rule, Language(ui_language_tag));
  *best_region_tree_language_tag = best_language.tag;

  LanguageRegionMap::const_iterator language_it =
      region_it->second->find(best_language.tag);
  if (language_it == region_it->second->end()) {
    const std::map<std::string, const Rule*>& rules =
        supplier_->GetRulesForRegion(region_code);
    language_it =
        region_it->second->insert(std::make_pair(best_language.tag,
                                                 BuildRegion(rules,
                                                             region_code,
                                                             best_language)))
            .first;
  }

  return *language_it->second;
}

}  // namespace addressinput
}  // namespace i18n
