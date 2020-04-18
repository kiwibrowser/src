// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace extensions {

using AccessibilityPrivateApiTest = ExtensionApiTest;

#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(AccessibilityPrivateApiTest, SendSyntheticKeyEvent) {
  // Not yet supported on mash.
  if (chromeos::GetAshConfig() == ash::Config::MASH)
    return;

  ASSERT_TRUE(RunExtensionSubtest("accessibility_private/",
                                  "send_synthetic_key_event.html"))
      << message_;
}
#endif  // defined (OS_CHROMEOS)

}  // namespace extensions
