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

#include "format_element.h"

#include <libaddressinput/address_field.h>

#include <cassert>
#include <ostream>

namespace i18n {
namespace addressinput {

FormatElement::FormatElement(AddressField field) : field_(field), literal_() {}

FormatElement::FormatElement(const std::string& literal)
    : field_(static_cast<AddressField>(-1)), literal_(literal) {
  assert(!literal.empty());
}

FormatElement::FormatElement()
    : field_(static_cast<AddressField>(-1)), literal_("\n") {}

bool FormatElement::operator==(const FormatElement& other) const {
  return field_ == other.field_ && literal_ == other.literal_;
}

}  // namespace addressinput
}  // namespace i18n

std::ostream& operator<<(std::ostream& o,
                         const i18n::addressinput::FormatElement& element) {
  if (element.IsField()) {
    o << "Field: " << element.GetField();
  } else if (element.IsNewline()) {
    o << "Newline";
  } else {
    o << "Literal: " << element.GetLiteral();
  }
  return o;
}
