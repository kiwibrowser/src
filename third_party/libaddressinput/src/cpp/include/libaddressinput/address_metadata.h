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

#ifndef I18N_ADDRESSINPUT_ADDRESS_METADATA_H_
#define I18N_ADDRESSINPUT_ADDRESS_METADATA_H_

#include <libaddressinput/address_field.h>

#include <string>

namespace i18n {
namespace addressinput {

// Checks whether |field| is a required field for |region_code|. Returns false
// also if no data could be found for region_code. Note: COUNTRY is always
// required.
bool IsFieldRequired(AddressField field, const std::string& region_code);

// Checks whether |field| is a field that is used for |region_code|. Returns
// false also if no data could be found for region_code. Note: COUNTRY is always
// used.
bool IsFieldUsed(AddressField field, const std::string& region_code);

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ADDRESS_METADATA_H_
