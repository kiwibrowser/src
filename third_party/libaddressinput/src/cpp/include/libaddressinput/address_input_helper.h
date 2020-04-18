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

#ifndef I18N_ADDRESSINPUT_ADDRESS_INPUT_HELPER_H_
#define I18N_ADDRESSINPUT_ADDRESS_INPUT_HELPER_H_

#include <vector>

namespace i18n {
namespace addressinput {

class LookupKey;
class PreloadSupplier;
struct AddressData;
struct Node;

class AddressInputHelper {
 public:
  AddressInputHelper(const AddressInputHelper&) = delete;
  AddressInputHelper& operator=(const AddressInputHelper&) = delete;

  // Creates an input helper that uses the supplier provided to get metadata to
  // help a user complete or fix an address. Doesn't take ownership of
  // |supplier|. Since latency is important for these kinds of tasks, we expect
  // the supplier to have the data already.
  AddressInputHelper(PreloadSupplier* supplier);
  ~AddressInputHelper();

  // Fill in missing components of an address as best as we can based on
  // existing data. For example, for some countries only one postal code is
  // valid; this would enter that one. For others, the postal code indicates
  // what state should be selected. Existing data will never be overwritten.
  //
  // Note that the preload supplier must have had the rules for the country
  // represented by this address loaded before this method is called - otherwise
  // an assertion failure will result.
  //
  // The address should have the best language tag as returned from
  // BuildComponents().
  void FillAddress(AddressData* address) const;

 private:
  void CheckChildrenForPostCodeMatches(
      const AddressData& address, const LookupKey& lookup_key,
      const Node* parent, std::vector<Node>* hierarchy) const;

  // We don't own the supplier_.
  PreloadSupplier* const supplier_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ADDRESS_INPUT_HELPER_H_
