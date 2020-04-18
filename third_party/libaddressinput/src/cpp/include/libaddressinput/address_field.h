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

#ifndef I18N_ADDRESSINPUT_ADDRESS_FIELD_H_
#define I18N_ADDRESSINPUT_ADDRESS_FIELD_H_

#include <iosfwd>

namespace i18n {
namespace addressinput {

// Address field types, ordered by size, from largest to smallest.
enum AddressField {
  COUNTRY,             // Country code.
  ADMIN_AREA,          // Administrative area such as a state, province,
                       // island, etc.
  LOCALITY,            // City or locality.
  DEPENDENT_LOCALITY,  // Dependent locality (may be an inner-city district or
                       // a suburb).
  SORTING_CODE,        // Sorting code.
  POSTAL_CODE,         // Zip or postal code.
  STREET_ADDRESS,      // Street address lines.
  ORGANIZATION,        // Organization, company, firm, institution, etc.
  RECIPIENT            // Name.
};

}  // namespace addressinput
}  // namespace i18n

// Produces human-readable output in logging, for example in unit tests. Prints
// what you would expect for valid fields, e.g. "COUNTRY" for COUNTRY. For
// invalid values, prints "[INVALID ENUM VALUE x]".
std::ostream& operator<<(std::ostream& o,
                         i18n::addressinput::AddressField field);

#endif  // I18N_ADDRESSINPUT_ADDRESS_FIELD_H_
