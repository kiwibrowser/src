// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/toolbar/toolbar_field_trial.h"

#include "base/feature_list.h"

namespace toolbar {
namespace features {

// Features used for EV UI removal experiment (https://crbug.com/803501).
const base::Feature kSimplifyHttpsIndicator{"SimplifyHttpsIndicator",
                                            base::FEATURE_DISABLED_BY_DEFAULT};
const char kSimplifyHttpsIndicatorParameterName[] = "treatment";
const char kSimplifyHttpsIndicatorParameterEvToSecure[] = "ev-to-secure";
const char kSimplifyHttpsIndicatorParameterSecureToLock[] = "secure-to-lock";
const char kSimplifyHttpsIndicatorParameterBothToLock[] = "both-to-lock";

}  // namespace features
}  // namespace toolbar
