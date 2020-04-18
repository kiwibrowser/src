// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

namespace extensions {

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, GetViews) {
  ASSERT_TRUE(RunExtensionTest("get_views")) << message_;
}

}  // namespace extensions
