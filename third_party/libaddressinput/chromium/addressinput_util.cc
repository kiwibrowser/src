// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/libaddressinput/chromium/addressinput_util.h"

#include <stddef.h>

#include <algorithm>

#include "base/logging.h"
#include "base/macros.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_data.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_metadata.h"

namespace autofill {
namespace addressinput {

namespace {

using ::i18n::addressinput::AddressData;
using ::i18n::addressinput::AddressField;
using ::i18n::addressinput::AddressProblem;
using ::i18n::addressinput::IsFieldRequired;

using ::i18n::addressinput::MISSING_REQUIRED_FIELD;

// Returns true if the |problem| should not be reported for the |field| because
// the |filter| excludes it.
bool FilterExcludes(const std::multimap<AddressField, AddressProblem>* filter,
                    AddressField field,
                    AddressProblem problem) {
  return filter != NULL && !filter->empty() &&
         std::find(filter->begin(),
                   filter->end(),
                   std::multimap<AddressField, AddressProblem>::value_type(
                       field, problem)) == filter->end();
}

}  // namespace

bool HasAllRequiredFields(const AddressData& address_to_check) {
  std::multimap<AddressField, AddressProblem> problems;
  ValidateRequiredFields(address_to_check, NULL, &problems);
  return problems.empty();
}

void ValidateRequiredFields(
    const AddressData& address_to_check,
    const std::multimap<AddressField, AddressProblem>* filter,
    std::multimap<AddressField, AddressProblem>* problems) {
  DCHECK(problems);

  static const AddressField kFields[] = {
      ::i18n::addressinput::COUNTRY,
      ::i18n::addressinput::ADMIN_AREA,
      ::i18n::addressinput::LOCALITY,
      ::i18n::addressinput::DEPENDENT_LOCALITY,
      ::i18n::addressinput::SORTING_CODE,
      ::i18n::addressinput::POSTAL_CODE,
      ::i18n::addressinput::STREET_ADDRESS,
      // ORGANIZATION is never required.
      ::i18n::addressinput::RECIPIENT
  };

  for (size_t i = 0; i < arraysize(kFields); ++i) {
    AddressField field = kFields[i];
    if (address_to_check.IsFieldEmpty(field) &&
        IsFieldRequired(field, address_to_check.region_code) &&
        !FilterExcludes(filter, field, MISSING_REQUIRED_FIELD)) {
      problems->insert(std::make_pair(field, MISSING_REQUIRED_FIELD));
    }
  }
}

}  // namespace addressinput
}  // namespace autofill
