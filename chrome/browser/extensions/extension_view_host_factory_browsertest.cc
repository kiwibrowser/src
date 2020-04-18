// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_view_host_factory.h"

#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_view_host.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "extensions/common/view_type.h"

namespace extensions {

typedef ExtensionBrowserTest ExtensionViewHostFactoryTest;

// Tests that ExtensionHosts are created with the correct type and profiles.
IN_PROC_BROWSER_TEST_F(ExtensionViewHostFactoryTest, CreateExtensionHosts) {
  // Load a very simple extension with just a background page.
  scoped_refptr<const Extension> extension =
      LoadExtension(test_data_dir_.AppendASCII("api_test")
                        .AppendASCII("browser_action")
                        .AppendASCII("none"));
  ASSERT_TRUE(extension.get());

  content::BrowserContext* browser_context = browser()->profile();
  {
    // Popup hosts are created with the correct type and profile.
    std::unique_ptr<ExtensionViewHost> host(
        ExtensionViewHostFactory::CreatePopupHost(extension->url(), browser()));
    EXPECT_EQ(extension.get(), host->extension());
    EXPECT_EQ(browser_context, host->browser_context());
    EXPECT_EQ(VIEW_TYPE_EXTENSION_POPUP, host->extension_host_type());
    EXPECT_TRUE(host->view());
  }

  {
    // Dialog hosts are created with the correct type and profile.
    std::unique_ptr<ExtensionViewHost> host(
        ExtensionViewHostFactory::CreateDialogHost(extension->url(),
                                                   browser()->profile()));
    EXPECT_EQ(extension.get(), host->extension());
    EXPECT_EQ(browser_context, host->browser_context());
    EXPECT_EQ(VIEW_TYPE_EXTENSION_DIALOG, host->extension_host_type());
    EXPECT_TRUE(host->view());
  }
}

// Tests that extensions loaded in incognito mode have the correct profiles
// for split-mode and non-split-mode.
// Crashing intermittently on multiple platforms. http://crbug.com/316334
IN_PROC_BROWSER_TEST_F(ExtensionViewHostFactoryTest,
                       DISABLED_IncognitoExtensionHosts) {
  // Open an incognito browser.
  Browser* incognito_browser =
      OpenURLOffTheRecord(browser()->profile(), GURL("about:blank"));

  // Load a non-split-mode extension, enabled in incognito.
  scoped_refptr<const Extension> regular_extension =
      LoadExtensionIncognito(test_data_dir_.AppendASCII("api_test")
                                 .AppendASCII("browser_action")
                                 .AppendASCII("none"));
  ASSERT_TRUE(regular_extension.get());

  // The ExtensionHost for a regular extension in an incognito window is
  // associated with the original window's profile.
  std::unique_ptr<ExtensionHost> regular_host(
      ExtensionViewHostFactory::CreatePopupHost(regular_extension->url(),
                                                incognito_browser));
  content::BrowserContext* browser_context = browser()->profile();
  EXPECT_EQ(browser_context, regular_host->browser_context());

  // Load a split-mode incognito extension.
  scoped_refptr<const Extension> split_mode_extension =
      LoadExtensionIncognito(test_data_dir_.AppendASCII("api_test")
                                 .AppendASCII("browser_action")
                                 .AppendASCII("split_mode"));
  ASSERT_TRUE(split_mode_extension.get());

  // The ExtensionHost for a split-mode extension is associated with the
  // incognito profile.
  std::unique_ptr<ExtensionHost> split_mode_host(
      ExtensionViewHostFactory::CreatePopupHost(split_mode_extension->url(),
                                                incognito_browser));
  content::BrowserContext* incognito_context = incognito_browser->profile();
  EXPECT_EQ(incognito_context, split_mode_host->browser_context());
}

}  // namespace extensions
