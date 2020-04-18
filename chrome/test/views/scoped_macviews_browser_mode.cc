// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/views/scoped_macviews_browser_mode.h"

#include "build/build_config.h"
#include "build/buildflag.h"
#include "ui/base/ui_base_features.h"

namespace test {

ScopedMacViewsBrowserMode::ScopedMacViewsBrowserMode(bool is_views) {
#if defined(OS_MACOSX) && BUILDFLAG(MAC_VIEWS_BROWSER)
  if (is_views)
    feature_list_.InitAndEnableFeature(features::kViewsBrowserWindows);
  else
    feature_list_.InitAndDisableFeature(features::kViewsBrowserWindows);
#endif
}

ScopedMacViewsBrowserMode::~ScopedMacViewsBrowserMode() {}

}  // namespace test
