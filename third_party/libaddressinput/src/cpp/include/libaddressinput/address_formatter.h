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
//
// Utility functions for formatting the addresses represented as AddressData.
//
// Note these work best if the address has a language code specified - this can
// be obtained when building the UI components (calling BuildComponents on
// address_ui.h).

#ifndef I18N_ADDRESSINPUT_ADDRESS_FORMATTER_H_
#define I18N_ADDRESSINPUT_ADDRESS_FORMATTER_H_

#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

struct AddressData;

// Formats the address onto multiple lines. This formats the address in national
// format; without the country.
void GetFormattedNationalAddress(
    const AddressData& address_data, std::vector<std::string>* lines);

// Formats the address as a single line. This formats the address in national
// format; without the country.
void GetFormattedNationalAddressLine(
    const AddressData& address_data, std::string* line);

// Formats the street-level part of an address as a single line. For example,
// two lines of "Apt 1", "10 Red St." will be concatenated in a
// language-appropriate way, to give something like "Apt 1, 10 Red St".
void GetStreetAddressLinesAsSingleLine(
    const AddressData& address_data, std::string* line);

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ADDRESS_FORMATTER_H_
