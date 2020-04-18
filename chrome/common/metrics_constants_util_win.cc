// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/metrics_constants_util_win.h"

#include "chrome/install_static/install_util.h"

namespace chrome {

base::string16 GetBrowserExitCodesRegistryPath() {
  return install_static::GetRegistryPath().append(L"\\BrowserExitCodes");
}

}  // namespace chrome
