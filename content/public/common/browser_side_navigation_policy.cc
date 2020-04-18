// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/browser_side_navigation_policy.h"

#include "base/command_line.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "services/network/public/cpp/features.h"

namespace content {

bool IsBrowserSideNavigationEnabled() {
  return true;
}

bool IsPerNavigationMojoInterfaceEnabled() {
  return base::FeatureList::IsEnabled(features::kPerNavigationMojoInterface);
}

}  // namespace content
