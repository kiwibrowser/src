// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_RATIONALIZATION_UTIL_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_RATIONALIZATION_UTIL_H_

#include <vector>

namespace autofill {

class AutofillField;

namespace rationalization_util {

// Helper function that rationalizes phone numbers fields in the given
// vector of fields. The vector of fields are expected to have all fields
// for a certain section.
void RationalizePhoneNumberFields(
    std::vector<AutofillField*>& fields_in_section);

}  // namespace rationalization_util
}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_RATIONALIZATION_UTIL_H_
