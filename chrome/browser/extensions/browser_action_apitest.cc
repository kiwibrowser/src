// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "extensions/browser/extension_registry.h"

// Tests that tooltips of a browser action icon can be specified using UTF8.
// See http://crbug.com/25349.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, TitleLocalizationBrowserAction) {
  extension::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  const size_t size_before = registry->enabled_extensions().size();
  base::FilePath extension_path(test_data_dir_.AppendASCII("browsertest")
                                        .AppendASCII("title_localized"));
  const Extension* extension = LoadExtension(extension_path);
  ASSERT_TRUE(extension);

  ASSERT_EQ(size_before + 1, registry->enabled_extensions().size());

  EXPECT_STREQ(base::WideToUTF8(
                   L"Hreggvi\u00F0ur: l10n browser action").c_str(),
               extension->description().c_str());
  EXPECT_STREQ(base::WideToUTF8(L"Hreggvi\u00F0ur is my name").c_str(),
               extension->name().c_str());
  int tab_id = ExtensionTabUtil::GetTabId(
      browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_STREQ(base::WideToUTF8(L"Hreggvi\u00F0ur").c_str(),
               extension->browser_action()->GetTitle(tab_id).c_str());
}
