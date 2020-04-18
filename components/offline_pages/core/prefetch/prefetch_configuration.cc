// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_configuration.h"

#include "components/offline_pages/core/offline_page_feature.h"

namespace offline_pages {

bool PrefetchConfiguration::IsPrefetchingEnabled() {
  return IsPrefetchingOfflinePagesEnabled() && IsPrefetchingEnabledBySettings();
}

}  // namespace offline_pages
