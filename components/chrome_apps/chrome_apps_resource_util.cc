// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/chrome_apps/chrome_apps_resource_util.h"

#include "base/logging.h"
#include "components/chrome_apps/grit/chrome_apps_resources_map.h"

namespace chrome_apps {

const GritResourceMap* GetChromeAppsResources(size_t* size) {
  DCHECK(size);

  *size = kChromeAppsResourcesSize;
  return kChromeAppsResources;
}

}  // namespace chrome_apps
