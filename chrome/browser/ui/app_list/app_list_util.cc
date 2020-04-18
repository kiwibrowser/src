// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/app_list_util.h"

#include "build/build_config.h"

bool IsAppLauncherEnabled() {
#if defined(OS_CHROMEOS)
  return true;
#else
  return false;
#endif
}
