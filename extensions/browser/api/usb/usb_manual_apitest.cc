// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/permissions/permissions_api.h"
#include "chrome/browser/extensions/extension_apitest.h"

using UsbManualApiTest = extensions::ExtensionApiTest;

IN_PROC_BROWSER_TEST_F(UsbManualApiTest, MANUAL_ListInterfaces) {
  extensions::PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  extensions::PermissionsRequestFunction::SetAutoConfirmForTests(true);
  ASSERT_TRUE(RunExtensionTest("usb_manual/list_interfaces"));
}
