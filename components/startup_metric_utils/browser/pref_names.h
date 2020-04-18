// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_STARTUP_METRIC_UTILS_BROWSER_PREF_NAMES_H_
#define COMPONENTS_STARTUP_METRIC_UTILS_BROWSER_PREF_NAMES_H_

namespace startup_metric_utils {
namespace prefs {

// Alphabetical list of preference names specific to the startup_metric_utils
// component. Keep alphabetized, and document each in the .cc file.

extern const char kLastStartupTimestamp[];
extern const char kLastStartupVersion[];
extern const char kSameVersionStartupCount[];

}  // namespace prefs
}  // namespace startup_metric_utils

#endif  // COMPONENTS_STARTUP_METRIC_UTILS_BROWSER_PREF_NAMES_H_
