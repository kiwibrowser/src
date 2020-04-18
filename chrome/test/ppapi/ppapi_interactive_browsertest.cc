// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/ppapi/ppapi_test.h"

#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

//
// Interface tests.
//

// Disable tests under ASAN.  http://crbug.com/104832.
// This is a bit heavy handed, but the majority of these tests fail under ASAN.
// See bug for history.
#if !defined(ADDRESS_SANITIZER)

// Disabled due to timeouts: http://crbug.com/136548
IN_PROC_BROWSER_TEST_F(
    OutOfProcessPPAPITest, DISABLED_MouseLock_SucceedWhenAllowed) {
  RunTestViaHTTP("MouseLock_SucceedWhenAllowed");
}

#endif // ADDRESS_SANITIZER
