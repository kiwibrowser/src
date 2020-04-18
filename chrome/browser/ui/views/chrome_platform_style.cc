// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_platform_style.h"

#include "build/build_config.h"

#if !defined(OS_MACOSX)
// static
bool ChromePlatformStyle::ShouldOmniboxUseFocusRing() {
  return false;
}
#endif
