// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/startup_metric_utils/browser/pref_names.h"

namespace startup_metric_utils {
namespace prefs {

// Time of the last startup stored as an int64.
const char kLastStartupTimestamp[] = "startup_metric.last_startup_timestamp";

// Version of the product in the startup preceding this one as reported by
// version_info.h.
const char kLastStartupVersion[] = "startup_metric.last_startup_version";

// Number of startups previously seen with kLastStartupVersion.
const char kSameVersionStartupCount[] =
    "startup_metric.same_version_startup_count";

}  // namespace prefs
}  // namespace startup_metric_utils
