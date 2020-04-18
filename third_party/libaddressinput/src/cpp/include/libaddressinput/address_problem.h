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

#ifndef I18N_ADDRESSINPUT_ADDRESS_PROBLEM_H_
#define I18N_ADDRESSINPUT_ADDRESS_PROBLEM_H_

#include <iosfwd>

namespace i18n {
namespace addressinput {

// Address problem types, in no particular order.
enum AddressProblem {
  // The field is not null and not whitespace, and the field should not be used
  // by addresses in this country. For example, in the U.S. the SORTING_CODE
  // field is unused, so its presence is an error.
  UNEXPECTED_FIELD,

  // The field is null or whitespace, and the field is required. For example,
  // in the U.S. ADMIN_AREA is a required field.
  MISSING_REQUIRED_FIELD,

  // A list of values for the field is defined and the value does not occur in
  // the list. Applies to hierarchical elements like REGION, ADMIN_AREA,
  // LOCALITY, and DEPENDENT_LOCALITY. For example, in the US, the values for
  // ADMIN_AREA include "CA" but not "XX".
  UNKNOWN_VALUE,

  // A format for the field is defined and the value does not match. This is
  // used to match POSTAL_CODE against the the format pattern generally. Formats
  // indicate how many digits/letters should be present, and what punctuation is
  // allowed. For example, in the U.S. postal codes are five digits with an
  // optional hyphen followed by four digits.
  INVALID_FORMAT,

  // A specific pattern for the field is defined based on a specific sub-region
  // (an ADMIN_AREA for example) and the value does not match. This is used to
  // match POSTAL_CODE against a regular expression. For example, in the U.S.
  // postal codes in the state of California start with '9'.
  MISMATCHING_VALUE,

  // The value contains a P.O. box and the widget options have acceptPostal set
  // to false. For example, a street address line that contained "P.O. Box 3456"
  // would fire this error.
  USES_P_O_BOX
};

}  // namespace addressinput
}  // namespace i18n

// Produces human-readable output in logging, for example in unit tests. Prints
// what you would expect for valid values, e.g. "UNEXPECTED_FIELD" for
// UNEXPECTED_FIELD. For invalid values, prints "[INVALID ENUM VALUE x]".
std::ostream& operator<<(std::ostream& o,
                         i18n::addressinput::AddressProblem problem);

#endif  // I18N_ADDRESSINPUT_ADDRESS_PROBLEM_H_
