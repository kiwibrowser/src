// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resources_util.h"

#include <stddef.h>

#include <utility>

#include "base/containers/flat_map.h"
#include "base/lazy_instance.h"
#include "build/build_config.h"
#include "chrome/grit/theme_resources_map.h"
#include "components/grit/components_scaled_resources_map.h"
#include "ui/resources/grit/ui_resources_map.h"

#if defined(OS_CHROMEOS)
#include "ui/chromeos/resources/grit/ui_chromeos_resources_map.h"
#endif

namespace {

// A wrapper class that holds a map between resource strings and resource
// ids.  This is done so we can use base::LazyInstance which takes care of
// thread safety in initializing the map for us.
class ThemeMap {
 public:
  typedef base::flat_map<std::string, int> StringIntMap;

  ThemeMap() {
    // Construct in one-shot from a moved vector.
    std::vector<StringIntMap::value_type> storage;

    for (size_t i = 0; i < kComponentsScaledResourcesSize; ++i) {
      storage.emplace_back(kComponentsScaledResources[i].name,
                           kComponentsScaledResources[i].value);
    }
    for (size_t i = 0; i < kThemeResourcesSize; ++i) {
      storage.emplace_back(kThemeResources[i].name, kThemeResources[i].value);
    }
    for (size_t i = 0; i < kUiResourcesSize; ++i) {
      storage.emplace_back(kUiResources[i].name, kUiResources[i].value);
    }
#if defined(OS_CHROMEOS)
    for (size_t i = 0; i < kUiChromeosResourcesSize; ++i) {
      storage.emplace_back(kUiChromeosResources[i].name,
                           kUiChromeosResources[i].value);
    }
#endif

    id_map_ = StringIntMap(std::move(storage), base::KEEP_FIRST_OF_DUPES);
  }

  int GetId(const std::string& resource_name) {
    auto it = id_map_.find(resource_name);
    if (it == id_map_.end())
      return -1;
    return it->second;
  }

 private:
  StringIntMap id_map_;
};

static base::LazyInstance<ThemeMap>::DestructorAtExit g_theme_ids =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

int ResourcesUtil::GetThemeResourceId(const std::string& resource_name) {
  return g_theme_ids.Get().GetId(resource_name);
}
