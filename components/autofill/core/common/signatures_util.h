// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_SIGNATURES_UTIL_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_SIGNATURES_UTIL_H_

#include <stddef.h>

#include <stdint.h>
#include <string>

#include "base/strings/string16.h"

namespace autofill {

struct FormData;
struct FormFieldData;

using FormSignature = uint64_t;
using FieldSignature = uint32_t;

// Calculates form signature based on |form_data|.
FormSignature CalculateFormSignature(const FormData& form_data);

// Calculates field signature based on |field_name| and |field_type|.
FieldSignature CalculateFieldSignatureByNameAndType(
    const base::string16& field_name,
    const std::string& field_type);

// Calculates field signature based on |field_data|. This function is a proxy to
// |CalculateFieldSignatureByNameAndType|.
FieldSignature CalculateFieldSignatureForField(const FormFieldData& field_data);

// Returns 64-bit hash of the string.
uint64_t StrToHash64Bit(const std::string& str);

// Returns 32-bit hash of the string.
uint32_t StrToHash32Bit(const std::string& str);

}

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_SIGNATURES_UTIL_H_
