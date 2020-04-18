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

#ifndef I18N_ADDRESSINPUT_LOOKUP_KEY_H_
#define I18N_ADDRESSINPUT_LOOKUP_KEY_H_

#include <libaddressinput/address_field.h>

#include <cstddef>
#include <map>
#include <string>

#include "util/size.h"

namespace i18n {
namespace addressinput {

struct AddressData;

// A LookupKey maps between an AddressData struct and the key string used to
// request address data from an address data server.
class LookupKey {
 public:
  // The array length is explicitly specified here, to make it possible to get
  // the length through size(LookupKey::kHierarchy).
  static const AddressField kHierarchy[4];

  LookupKey(const LookupKey&) = delete;
  LookupKey& operator=(const LookupKey&) = delete;

  LookupKey();
  ~LookupKey();

  // Initializes this object by parsing |address|.
  void FromAddress(const AddressData& address);

  // Initializes this object to be a copy of |parent| key that's one level
  // deeper with the next level node being |child_node|.
  //
  // For example, if |parent| is "data/US" and |child_node| is "CA", then this
  // key becomes "data/US/CA".
  //
  // The |parent| can be at most LOCALITY level. The |child_node| cannot be
  // empty.
  void FromLookupKey(const LookupKey& parent, const std::string& child_node);

  // Returns the lookup key string (of |max_depth|).
  std::string ToKeyString(size_t max_depth) const;

  // Returns the region code. Must not be called on an empty object.
  const std::string& GetRegionCode() const;

  // Returns the depth. Must not be called on an empty object.
  size_t GetDepth() const;

  void set_language(const std::string& language) { language_ = language; }

 private:
  std::map<AddressField, std::string> nodes_;
  // The language of the key, obtained from the address (empty for default
  // language).
  std::string language_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_LOOKUP_KEY_H_
