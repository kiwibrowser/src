// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A handful of resource-like constants related to the Chrome application.

#ifndef CHROME_COMMON_METRICS_CONSTANTS_UTIL_WIN_H_
#define CHROME_COMMON_METRICS_CONSTANTS_UTIL_WIN_H_

#include "base/strings/string16.h"

namespace chrome {

// Returns the registry path where exit code are stored for this product. This
// is used by browser exit code metrics reporting.
base::string16 GetBrowserExitCodesRegistryPath();

}  // namespace chrome

#endif  // CHROME_COMMON_METRICS_CONSTANTS_UTIL_WIN_H_
