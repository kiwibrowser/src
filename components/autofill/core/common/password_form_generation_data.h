// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_GENERATION_DATA_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_GENERATION_DATA_H_

#include <stdint.h>

#include "base/optional.h"
#include "base/strings/string16.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/signatures_util.h"
#include "url/gurl.h"

namespace autofill {

// Structure used for sending information from browser to renderer about on
// which fields password should be generated.
struct PasswordFormGenerationData {
  PasswordFormGenerationData();
  PasswordFormGenerationData(FormSignature form_signature,
                             FieldSignature field_signature);
  PasswordFormGenerationData(const PasswordFormGenerationData& other);
  ~PasswordFormGenerationData();

  // The unique signature of form where password should be generated
  // (see components/autofill/core/browser/form_structure.h).
  FormSignature form_signature;

  // The unique signature of field where password should be generated
  // (see components/autofill/core/browser/autofill_field.h).
  FieldSignature field_signature;

  // The unique signature of the confirmation field where the generated password
  // should be copied to.
  base::Optional<FieldSignature> confirmation_field_signature;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_GENERATION_DATA_H_
