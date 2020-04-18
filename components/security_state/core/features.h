// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_STATE_FEATURES_H_
#define COMPONENTS_SECURITY_STATE_FEATURES_H_

#include "base/feature_list.h"

namespace security_state {
namespace features {

// This feature enables more aggressive warnings for nonsecure http:// pages.
// The exact warning treatment is dependent on the parameter 'treatment' which
// can have the following values:
// - 'warning': Show a Not Secure warning on all http:// pages
// - 'dangerous': Treat all http:// pages as actively dangerous
// - 'warning-and-dangerous-on-form-edits': Show a Not Secure warning on all
//   http:// pages, and treat them as actively dangerous when the user edits
//   form fields
// - 'warning-and-dangerous-on-passwords-and-credit-cards': Show a Not Secure
//   warning on all http:// pages, and treat them as actively dangerous when
//   there is a sensitive field
extern const base::Feature kMarkHttpAsFeature;

// The parameter name which controls the warning treatment.
extern const char kMarkHttpAsFeatureParameterName[];

// The different parameter values, described above.
extern const char kMarkHttpAsParameterWarning[];
extern const char kMarkHttpAsParameterDangerous[];
extern const char kMarkHttpAsParameterWarningAndDangerousOnFormEdits[];
extern const char
    kMarkHttpAsParameterWarningAndDangerousOnPasswordsAndCreditCards[];

}  // namespace features
}  // namespace security_state

#endif  // COMPONENTS_SECURITY_STATE_FEATURES_H_
