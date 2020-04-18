// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/password_form_generation_data.h"

namespace autofill {

PasswordFormGenerationData::PasswordFormGenerationData() = default;

PasswordFormGenerationData::PasswordFormGenerationData(
    FormSignature form_signature,
    FieldSignature field_signature)
    : form_signature(form_signature), field_signature(field_signature) {}

PasswordFormGenerationData::PasswordFormGenerationData(
    const PasswordFormGenerationData& other) = default;

PasswordFormGenerationData::~PasswordFormGenerationData() = default;

}  // namespace autofill
