// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/form_types.h"
#include "components/autofill/core/browser/field_types.h"

namespace autofill {

// static
FormType FormTypes::FieldTypeGroupToFormType(FieldTypeGroup field_type_group) {
  switch (field_type_group) {
    case CREDIT_CARD:
      return CREDIT_CARD_FORM;
    case USERNAME_FIELD:
    case PASSWORD_FIELD:
      return PASSWORD_FORM;
    case NO_GROUP:
    case TRANSACTION:
      return UNKNOWN_FORM_TYPE;
    default:
      // Assuming it's an address form by process of elimination.
      return ADDRESS_FORM;
  }
}
}  // namespace autofill
