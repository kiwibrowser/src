// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/app/blink_test_platform_support.h"

#include "base/test/fontconfig_util_linux.h"

namespace content {

bool CheckLayoutSystemDeps() {
  return true;
}

bool BlinkTestPlatformInitialize() {
  base::SetUpFontconfig();
  return true;
}

}  // namespace content
