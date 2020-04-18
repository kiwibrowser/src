// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CHROME_APPS_CHROME_APPS_RESOURCE_UTIL_H_
#define COMPONENTS_CHROME_APPS_CHROME_APPS_RESOURCE_UTIL_H_

#include <stddef.h>

#include "components/chrome_apps/chrome_apps_export.h"

struct GritResourceMap;

namespace chrome_apps {

// Get the list of resources. |size| is populated with the number of resources
// in the returned array.
CHROME_APPS_EXPORT const GritResourceMap* GetChromeAppsResources(
    size_t* size);

}  // namespace chrome_apps

#endif  // COMPONENTS_CHROME_APPS_CHROME_APPS_RESOURCE_UTIL_H_
