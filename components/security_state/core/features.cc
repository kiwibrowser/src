// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_state/core/features.h"

namespace security_state {
namespace features {

const base::Feature kMarkHttpAsFeature{"MarkHttpAs",
                                       base::FEATURE_ENABLED_BY_DEFAULT};
const char kMarkHttpAsFeatureParameterName[] = "treatment";
const char kMarkHttpAsParameterWarning[] = "warning";
const char kMarkHttpAsParameterDangerous[] = "dangerous";
const char kMarkHttpAsParameterWarningAndDangerousOnFormEdits[] =
    "warning-and-dangerous-on-form-edits";
const char kMarkHttpAsParameterWarningAndDangerousOnPasswordsAndCreditCards[] =
    "warning-and-dangerous-on-passwords-and-credit-cards";

}  // namespace features
}  // namespace security_state
