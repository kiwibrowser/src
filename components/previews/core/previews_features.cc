// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_features.h"

#include "build/build_config.h"

namespace previews {
namespace features {

// Kill switch (or holdback) for all previews. No previews will be allowed
// if this feature is disabled. If enabled, which specific previews that
// are enabled are controlled by other features.
const base::Feature kPreviews {
  "Previews",
#if defined(OS_ANDROID) || defined(OS_LINUX)
      // Previews allowed for Android (but also allow on Linux for dev/debug).
      base::FEATURE_ENABLED_BY_DEFAULT
#else   // !defined(OS_ANDROID) || defined(OS_LINUX)
      base::FEATURE_DISABLED_BY_DEFAULT
#endif  // defined(OS_ANDROID) || defined(OS_LINUX)
};

// Enables the Offline previews on android slow connections.
const base::Feature kOfflinePreviews{"OfflinePreviews",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

// Enables the Client Lo-Fi previews.
const base::Feature kClientLoFi {
  "ClientLoFi",
#if defined(OS_ANDROID)
      base::FEATURE_ENABLED_BY_DEFAULT
#else   // !defined(OS_ANDROID)
      base::FEATURE_DISABLED_BY_DEFAULT
#endif  // defined(OS_ANDROID)
};

// Enables the NoScript previews for Android.
const base::Feature kNoScriptPreviews{"NoScriptPreviews",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the Stale Previews timestamp on Previews infobars.
const base::Feature kStalePreviewsTimestamp{"StalePreviewsTimestamp",
                                            base::FEATURE_ENABLED_BY_DEFAULT};

const base::Feature kAMPRedirection{"AMPRedirectionPreviews",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// Enables the syncing of the Optimization Hints component, which provides
// hints for what Previews can be applied on a page load.
const base::Feature kOptimizationHints{"OptimizationHints",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace previews
