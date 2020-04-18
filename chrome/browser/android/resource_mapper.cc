// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/resource_mapper.h"

#include <map>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/grit/components_scaled_resources.h"

namespace {

typedef std::map<int, int> ResourceMap;
base::LazyInstance<ResourceMap>::Leaky g_id_map = LAZY_INSTANCE_INITIALIZER;

} // namespace

const int ResourceMapper::kMissingId = -1;

int ResourceMapper::MapFromChromiumId(int resource_id) {
  if (g_id_map.Get().empty()) {
    ConstructMap();
  }

  ResourceMap::iterator iterator = g_id_map.Get().find(resource_id);
  if (iterator != g_id_map.Get().end()) {
    return iterator->second;
  }

  // The resource couldn't be found.
  NOTREACHED();
  return kMissingId;
}

void ResourceMapper::ConstructMap() {
  DCHECK(g_id_map.Get().empty());
  int next_id = 0;

#define LINK_RESOURCE_ID(c_id, java_id) g_id_map.Get()[c_id] = next_id++;
#define DECLARE_RESOURCE_ID(c_id, java_id) g_id_map.Get()[c_id] = next_id++;
#include "chrome/browser/android/resource_id.h"
#undef LINK_RESOURCE_ID
#undef DECLARE_RESOURCE_ID
}
