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

#include "address_field_util.h"

#include <libaddressinput/address_field.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "format_element.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

namespace {

// Check whether |c| is a field token character. On success, return true
// and sets |*field| to the corresponding AddressField value. Return false
// on failure.
bool ParseFieldToken(char c, AddressField* field) {
  assert(field != nullptr);

  // Simple mapping from field token characters to AddressField values.
  static const struct Entry { char ch; AddressField field; } kTokenMap[] = {
    { 'R', COUNTRY },
    { 'S', ADMIN_AREA },
    { 'C', LOCALITY },
    { 'D', DEPENDENT_LOCALITY },
    { 'X', SORTING_CODE },
    { 'Z', POSTAL_CODE },
    { 'A', STREET_ADDRESS },
    { 'O', ORGANIZATION },
    { 'N', RECIPIENT },
  };
  const size_t kTokenMapSize = size(kTokenMap);

  for (size_t n = 0; n < kTokenMapSize; ++n) {
      if (c == kTokenMap[n].ch) {
          *field = kTokenMap[n].field;
          return true;
      }
  }
  return false;
}

}  // namespace

void ParseFormatRule(const std::string& format,
                     std::vector<FormatElement>* elements) {
  assert(elements != nullptr);
  elements->clear();

  std::string::const_iterator prev = format.begin();
  for (std::string::const_iterator next = format.begin();
       next != format.end(); prev = ++next) {
    // Find the next field element or newline (indicated by %<TOKEN>).
    if ((next = std::find(next, format.end(), '%')) == format.end()) {
      // No more tokens in the format string.
      break;
    }
    if (prev < next) {
      // Push back preceding literal.
      elements->push_back(FormatElement(std::string(prev, next)));
    }
    if ((prev = ++next) == format.end()) {
      // Move forward and check we haven't reached the end of the string
      // (unlikely, it shouldn't end with %).
      break;
    }
    // Process the token after the %.
    AddressField field;
    if (*next == 'n') {
      elements->push_back(FormatElement());
    } else if (ParseFieldToken(*next, &field)) {
      elements->push_back(FormatElement(field));
    }  // Else it's an unknown token, we ignore it.
  }
  // Push back any trailing literal.
  if (prev != format.end()) {
    elements->push_back(FormatElement(std::string(prev, format.end())));
  }
}

void ParseAddressFieldsRequired(const std::string& required,
                                std::vector<AddressField>* fields) {
  assert(fields != nullptr);
  fields->clear();
  for (std::string::const_iterator it = required.begin();
       it != required.end(); ++it) {
    AddressField field;
    if (ParseFieldToken(*it, &field)) {
      fields->push_back(field);
    }
  }
}

}  // namespace addressinput
}  // namespace i18n
