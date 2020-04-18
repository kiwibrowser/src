// Copyright (C) 2013 Google Inc.
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

#ifndef I18N_ADDRESSINPUT_ADDRESS_FIELD_UTIL_H_
#define I18N_ADDRESSINPUT_ADDRESS_FIELD_UTIL_H_

#include <libaddressinput/address_field.h>

#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

class FormatElement;

// Clears |fields|, parses |format|, and adds the format address fields to
// |fields|. The |fields| may also contain NEWLINE elements. For example, parses
// "%S%C%n%D%X" into {ADMIN_AREA, LOCALITY, NEWLINE, DEPENDENT_LOCALITY,
// SORTING_CODE}.
void ParseFormatRule(const std::string& format,
                     std::vector<FormatElement>* elements);

// Clears |fields|, parses |required|, and adds the required fields to |fields|.
// For example, parses "SCDX" into {ADMIN_AREA, LOCALITY, DEPENDENT_LOCALITY,
// SORTING_CODE}.
void ParseAddressFieldsRequired(const std::string& required,
                                std::vector<AddressField>* fields);

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ADDRESS_FIELD_UTIL_H_
