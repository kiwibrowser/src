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

#ifndef I18N_ADDRESSINPUT_ADDRESS_NORMALIZER_H_
#define I18N_ADDRESSINPUT_ADDRESS_NORMALIZER_H_

#include <memory>

namespace i18n {
namespace addressinput {

class PreloadSupplier;
class StringCompare;
struct AddressData;

class AddressNormalizer {
 public:
  AddressNormalizer(const AddressNormalizer&) = delete;
  AddressNormalizer& operator=(const AddressNormalizer&) = delete;

  // Does not take ownership of |supplier|.
  explicit AddressNormalizer(const PreloadSupplier* supplier);
  ~AddressNormalizer();

  // Converts the names of different fields in the address into their canonical
  // form. Should be called only when supplier->IsLoaded() returns true for
  // the region code of the |address|.
  void Normalize(AddressData* address) const;

 private:
  const PreloadSupplier* const supplier_;  // Not owned.
  const std::unique_ptr<const StringCompare> compare_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ADDRESS_NORMALIZER_H_
