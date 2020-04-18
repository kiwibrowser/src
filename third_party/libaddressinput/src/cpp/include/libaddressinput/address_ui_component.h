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

#ifndef I18N_ADDRESSINPUT_ADDRESS_UI_COMPONENT_H_
#define I18N_ADDRESSINPUT_ADDRESS_UI_COMPONENT_H_

#include <libaddressinput/address_field.h>

#include <string>

namespace i18n {
namespace addressinput {

// A description of an input field in an address form. The user of the library
// will use a list of these elements to layout the address form input fields.
struct AddressUiComponent {
  // The types of hints for how large the field should be in a multiline address
  // form.
  enum LengthHint {
    HINT_LONG,  // The field should take up the whole line.
    HINT_SHORT  // The field does not need to take up the whole line.
  };

  // The address field type for this UI component, for example LOCALITY.
  AddressField field;

  // The name of the field, for example "City".
  std::string name;

  // The hint for how large the input field should be in a multiline address
  // form.
  LengthHint length_hint;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_ADDRESS_UI_COMPONENT_H_
