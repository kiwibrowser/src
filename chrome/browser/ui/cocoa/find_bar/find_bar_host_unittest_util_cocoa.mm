// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/find_bar/find_bar_host_unittest_util.h"

#include "build/buildflag.h"
#include "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"
#include "ui/base/ui_features.h"

namespace chrome {

void DisableFindBarCocoaAnimationsDuringTesting(bool /* disable */) {
  FindBarBridge::disable_animations_during_testing_ = true;
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
void DisableFindBarAnimationsDuringTesting(bool disable) {
  DisableFindBarCocoaAnimationsDuringTesting(disable);
}
#endif

}  // namespace chrome
