// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "chrome/browser/extensions/extension_service_test_with_install.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"

namespace extensions {
namespace {

class BrowserActionUnitTest : public ExtensionServiceTestWithInstall {
};

TEST_F(BrowserActionUnitTest, MultiIcons) {
  InitializeEmptyExtensionService();
  base::FilePath path =
      data_dir().AppendASCII("api_test/browser_action/multi_icons");
  ASSERT_TRUE(base::PathExists(path));

  const Extension* extension = PackAndInstallCRX(path, INSTALL_NEW);

  EXPECT_EQ(0U, extension->install_warnings().size());
  const ActionInfo* browser_action_info =
      ActionInfo::GetBrowserActionInfo(extension);
  ASSERT_TRUE(browser_action_info);

  const ExtensionIconSet& icons = browser_action_info->default_icon;

  // Extension can provide arbitrary sizes.
  EXPECT_EQ(4u, icons.map().size());
  EXPECT_EQ("icon19.png", icons.Get(19, ExtensionIconSet::MATCH_EXACTLY));
  EXPECT_EQ("icon24.png", icons.Get(24, ExtensionIconSet::MATCH_EXACTLY));
  EXPECT_EQ("icon24.png", icons.Get(31, ExtensionIconSet::MATCH_EXACTLY));
  EXPECT_EQ("icon38.png", icons.Get(38, ExtensionIconSet::MATCH_EXACTLY));
}

} // namespace
} // namespace extensions
