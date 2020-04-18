// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/main/main_feature_flags.h"

const base::Feature kTabSwitcherPresentsBVC{"TabSwitcherPresentsBVC",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

bool TabSwitcherPresentsBVCEnabled() {
  return base::FeatureList::IsEnabled(kTabSwitcherPresentsBVC);
}
