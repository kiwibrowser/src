// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spellcheck/common/spellcheck_features.h"

#include "base/sys_info.h"
#include "components/spellcheck/spellcheck_buildflags.h"

namespace spellcheck {

#if BUILDFLAG(ENABLE_SPELLCHECK) && defined(OS_ANDROID)

// Enables/disables Android spellchecker.
const base::Feature kAndroidSpellChecker{
    "AndroidSpellChecker", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables/disables Android spellchecker on non low-end Android devices.
const base::Feature kAndroidSpellCheckerNonLowEnd{
    "AndroidSpellCheckerNonLowEnd", base::FEATURE_ENABLED_BY_DEFAULT};

bool IsAndroidSpellCheckFeatureEnabled() {
  if (base::FeatureList::IsEnabled(spellcheck::kAndroidSpellCheckerNonLowEnd)) {
    return !base::SysInfo::IsLowEndDevice();
  }

  if (base::FeatureList::IsEnabled(spellcheck::kAndroidSpellChecker)) {
    return true;
  }

  return false;
}

#endif  // BUILDFLAG(ENABLE_SPELLCHECK) && defined(OS_ANDROID)

}  // namespace spellcheck
