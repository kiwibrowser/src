// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_PASSWORD_FORM_FIELD_PREDICTION_MAP_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_PASSWORD_FORM_FIELD_PREDICTION_MAP_H_

#include <map>

#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"

namespace autofill {

// This enum lists form field types as understood by the password manager,
// esentially a digest of |autofill::ServerFieldType|. Note that we cannot
// simply reuse |autofill::ServerFieldType| as it is defined in the browser,
// while this enum will be used by both the browser and renderer.
enum PasswordFormFieldPredictionType {
  PREDICTION_USERNAME,
  PREDICTION_CURRENT_PASSWORD,
  PREDICTION_NEW_PASSWORD,
  PREDICTION_NOT_PASSWORD,
  PREDICTION_MAX = PREDICTION_NOT_PASSWORD
};

using PasswordFormFieldPredictionMap =
    std::map<FormFieldData, PasswordFormFieldPredictionType>;
using FormsPredictionsMap =
    std::map<FormData, PasswordFormFieldPredictionMap>;

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_PASSWORD_FORM_FIELD_PREDICTION_MAP_H_
