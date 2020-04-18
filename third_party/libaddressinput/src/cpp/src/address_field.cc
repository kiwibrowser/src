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

#include <libaddressinput/address_field.h>

#include <cstddef>
#include <ostream>

#include "util/size.h"

using i18n::addressinput::AddressField;
using i18n::addressinput::COUNTRY;
using i18n::addressinput::RECIPIENT;
using i18n::addressinput::size;

std::ostream& operator<<(std::ostream& o, AddressField field) {
  static const char* const kFieldNames[] = {
    "COUNTRY",
    "ADMIN_AREA",
    "LOCALITY",
    "DEPENDENT_LOCALITY",
    "SORTING_CODE",
    "POSTAL_CODE",
    "STREET_ADDRESS",
    "ORGANIZATION",
    "RECIPIENT"
  };
  static_assert(COUNTRY == 0, "bad_base");
  static_assert(RECIPIENT == size(kFieldNames) - 1, "bad_length");

  if (field < 0 || static_cast<size_t>(field) >= size(kFieldNames)) {
    o << "[INVALID ENUM VALUE " << static_cast<int>(field) << "]";
  } else {
    o << kFieldNames[field];
  }
  return o;
}
