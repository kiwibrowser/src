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

#include <libaddressinput/address_input_helper.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>
#include <libaddressinput/address_metadata.h>
#include <libaddressinput/preload_supplier.h>

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include <re2/re2.h>

#include "language.h"
#include "lookup_key.h"
#include "region_data_constants.h"
#include "rule.h"
#include "util/re2ptr.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

// Used for building a hierarchy of rules, each one connected to its parent.
struct Node {
  const Node* parent;
  const Rule* rule;
};

namespace {

const char kLookupKeySeparator = '/';

const size_t kHierarchyDepth = size(LookupKey::kHierarchy);

// Gets the best name for the entity represented by the current rule, using the
// language provided. The language is currently used to distinguish whether a
// Latin-script name should be fetched; if it is not explicitly Latin-script, we
// prefer IDs over names (so return CA instead of California for an English
// user.) If there is no Latin-script name, we fall back to the ID.
std::string GetBestName(const Language& language, const Rule& rule) {
  if (language.has_latin_script) {
    const std::string& name = rule.GetLatinName();
    if (!name.empty()) {
      return name;
    }
  }
  // The ID is stored as data/US/CA for "CA", for example, and we only want the
  // last part.
  const std::string& id = rule.GetId();
  std::string::size_type pos = id.rfind(kLookupKeySeparator);
  assert(pos != std::string::npos);
  return id.substr(pos + 1);
}

void FillAddressFromMatchedRules(
    const std::vector<Node>* hierarchy,
    AddressData* address) {
  assert(hierarchy != nullptr);
  assert(address != nullptr);
  // We skip region code, because we never try and fill that in if it isn't
  // already set.
  Language language(address->language_code);
  for (size_t depth = kHierarchyDepth - 1; depth > 0; --depth) {
    // If there is only one match at this depth, then we should populate the
    // address, using this rule and its parents.
    if (hierarchy[depth].size() == 1) {
      for (const Node* node = &hierarchy[depth].front();
           node != nullptr; node = node->parent, --depth) {
        const Rule* rule = node->rule;
        assert(rule != nullptr);

        AddressField field = LookupKey::kHierarchy[depth];
        // Note only empty fields are permitted to be overwritten.
        if (address->IsFieldEmpty(field)) {
          address->SetFieldValue(field, GetBestName(language, *rule));
        }
      }
      break;
    }
  }
}

}  // namespace

AddressInputHelper::AddressInputHelper(PreloadSupplier* supplier)
    : supplier_(supplier) {
  assert(supplier_ != nullptr);
}

AddressInputHelper::~AddressInputHelper() {
}

void AddressInputHelper::FillAddress(AddressData* address) const {
  assert(address != nullptr);
  const std::string& region_code = address->region_code;
  if (!RegionDataConstants::IsSupported(region_code)) {
    // If we don't have a region code, we can't do anything reliably to fill
    // this address.
    return;
  }

  AddressData lookup_key_address;
  lookup_key_address.region_code = region_code;
  // First try and fill in the postal code if it is missing.
  LookupKey lookup_key;
  lookup_key.FromAddress(lookup_key_address);
  const Rule* region_rule = supplier_->GetRule(lookup_key);
  // We have already checked that the region is supported; and users of this
  // method must have called LoadRules() first, so we check this here.
  assert(region_rule != nullptr);

  const RE2ptr* postal_code_reg_exp = region_rule->GetPostalCodeMatcher();
  if (postal_code_reg_exp != nullptr) {
    if (address->postal_code.empty()) {
      address->postal_code = region_rule->GetSolePostalCode();
    }

    // If we have a valid postal code, try and work out the most specific
    // hierarchy that matches the postal code. Note that the postal code might
    // have been added in the previous check.
    if (!address->postal_code.empty() &&
        RE2::FullMatch(address->postal_code, *postal_code_reg_exp->ptr)) {

      // This hierarchy is used to store rules that represent possible matches
      // at each level of the hierarchy.
      std::vector<Node> hierarchy[kHierarchyDepth];
      CheckChildrenForPostCodeMatches(*address, lookup_key, nullptr, hierarchy);

      FillAddressFromMatchedRules(hierarchy, address);
    }
  }

  // TODO: When we have the data, we should fill in the state for countries with
  // state required and only one possible value, e.g. American Samoa.
}

void AddressInputHelper::CheckChildrenForPostCodeMatches(
    const AddressData& address,
    const LookupKey& lookup_key,
    const Node* parent,
    // An array of vectors.
    std::vector<Node>* hierarchy) const {
  const Rule* rule = supplier_->GetRule(lookup_key);
  assert(rule != nullptr);

  const RE2ptr* postal_code_prefix = rule->GetPostalCodeMatcher();
  if (postal_code_prefix == nullptr ||
      RE2::PartialMatch(address.postal_code, *postal_code_prefix->ptr)) {
    size_t depth = lookup_key.GetDepth();
    assert(depth < size(LookupKey::kHierarchy));

    // This was a match, so store it and its parent in the hierarchy.
    hierarchy[depth].push_back(Node());
    Node* node = &hierarchy[depth].back();
    node->parent = parent;
    node->rule = rule;

    if (depth < size(LookupKey::kHierarchy) - 1 &&
        IsFieldUsed(LookupKey::kHierarchy[depth + 1], address.region_code)) {
      // If children are used and present, check them too.
      for (std::vector<std::string>::const_iterator child_it =
               rule->GetSubKeys().begin();
           child_it != rule->GetSubKeys().end(); ++child_it) {
        LookupKey child_key;
        child_key.FromLookupKey(lookup_key, *child_it);
        CheckChildrenForPostCodeMatches(address, child_key, node, hierarchy);
      }
    }
  }
}

}  // namespace addressinput
}  // namespace i18n
