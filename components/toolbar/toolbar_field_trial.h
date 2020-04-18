// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TOOLBAR_TOOLBAR_FIELD_TRIAL_H_
#define COMPONENTS_TOOLBAR_TOOLBAR_FIELD_TRIAL_H_

#include "base/feature_list.h"

namespace toolbar {
namespace features {

// This feature simplifies the security indiciator UI for https:// pages. The
// exact UI treatment is dependent on the parameter 'treatment' which can have
// the following value:
// - 'ev-to-secure': Show the "Secure" chip for pages with an EV certificate.
// - 'secure-to-lock': Show only the lock icon for non-EV https:// pages.
// - 'both-to-lock': Show only the lock icon for all https:// pages.
extern const base::Feature kSimplifyHttpsIndicator;

// The parameter name which controls the UI treatment.
extern const char kSimplifyHttpsIndicatorParameterName[];

// The different parameter values, described above.
extern const char kSimplifyHttpsIndicatorParameterEvToSecure[];
extern const char kSimplifyHttpsIndicatorParameterSecureToLock[];
extern const char kSimplifyHttpsIndicatorParameterBothToLock[];

}  // namespace features
}  // namespace toolbar

#endif  // COMPONENTS_TOOLBAR_TOOLBAR_FIELD_TRIAL_H_
