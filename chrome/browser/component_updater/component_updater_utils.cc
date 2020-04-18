// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/component_updater_utils.h"

#include "build/build_config.h"
#if defined(OS_WIN)
#include "chrome/installer/util/install_util.h"
#endif  // OS_WIN

namespace component_updater {

bool IsPerUserInstall() {
#if defined(OS_WIN)
  // The installer computes and caches this value in memory during the
  // process start up.
  return InstallUtil::IsPerUserInstall();
#else
  return true;
#endif
}

}  // namespace component_updater
