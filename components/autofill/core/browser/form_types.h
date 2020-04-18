// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_TYPES_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_TYPES_H_

#include "components/autofill/core/browser/field_types.h"

namespace autofill {

enum FormType : int {
  UNKNOWN_FORM_TYPE,
  ADDRESS_FORM,
  CREDIT_CARD_FORM,
  PASSWORD_FORM
};

class FormTypes {
 public:
  static FormType FieldTypeGroupToFormType(FieldTypeGroup field_type_group);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_TYPES_H_
