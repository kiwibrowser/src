// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback_helpers.h"
#include "base/run_loop.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/navigation_observer.h"
#include "chrome/test/base/ui_test_utils.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"

namespace extensions {

// Test that visiting an url associated with a disabled extension offers to
// re-enable it.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest,
                       PromptToReEnableExtensionsOnNavigation) {
  NavigationObserver::SetAllowedRepeatedPromptingForTesting(true);
  base::ScopedClosureRunner reset_repeated_prompting(base::BindOnce([]() {
    NavigationObserver::SetAllowedRepeatedPromptingForTesting(false);
  }));
  scoped_refptr<const Extension> extension =
      ChromeTestExtensionLoader(profile()).LoadExtension(
          test_data_dir_.AppendASCII("simple_with_file"));
  ASSERT_TRUE(extension);
  const std::string kExtensionId = extension->id();
  const GURL kUrl = extension->GetResourceURL("file.html");

  // We always navigate in a new tab because when we disable the extension, it
  // closes all tabs for that extension. If we only opened in the current tab,
  // this would result in the only open tab being closed, and the test quitting.
  auto navigate_to_url_in_new_tab = [this](const GURL& url) {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  };

  ExtensionRegistry* registry = ExtensionRegistry::Get(profile());
  EXPECT_TRUE(registry->enabled_extensions().Contains(kExtensionId));

  // Disable the extension due to a permissions increase.
  extension_service()->DisableExtension(
      kExtensionId, disable_reason::DISABLE_PERMISSIONS_INCREASE);
  EXPECT_TRUE(registry->disabled_extensions().Contains(kExtensionId));

  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile());
  EXPECT_EQ(disable_reason::DISABLE_PERMISSIONS_INCREASE,
            prefs->GetDisableReasons(kExtensionId));

  {
    // Visit an associated url and deny the prompt. The extension should remain
    // disabled.
    ScopedTestDialogAutoConfirm auto_deny(ScopedTestDialogAutoConfirm::CANCEL);
    navigate_to_url_in_new_tab(kUrl);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(registry->disabled_extensions().Contains(kExtensionId));
    EXPECT_EQ(disable_reason::DISABLE_PERMISSIONS_INCREASE,
              prefs->GetDisableReasons(kExtensionId));
  }

  {
    // Visit an associated url and accept the prompt. The extension should get
    // re-enabled.
    ScopedTestDialogAutoConfirm auto_accept(
        ScopedTestDialogAutoConfirm::ACCEPT);
    navigate_to_url_in_new_tab(kUrl);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(registry->enabled_extensions().Contains(kExtensionId));
    EXPECT_EQ(disable_reason::DISABLE_NONE,
              prefs->GetDisableReasons(kExtensionId));
  }

  // Disable the extension for something other than a permissions increase.
  extension_service()->DisableExtension(kExtensionId,
                                        disable_reason::DISABLE_USER_ACTION);
  EXPECT_TRUE(registry->disabled_extensions().Contains(kExtensionId));
  EXPECT_EQ(disable_reason::DISABLE_USER_ACTION,
            prefs->GetDisableReasons(kExtensionId));

  {
    // We only prompt for permissions increases, not any other disable reason.
    // As such, the extension should stay disabled.
    ScopedTestDialogAutoConfirm auto_accept(
        ScopedTestDialogAutoConfirm::ACCEPT);
    navigate_to_url_in_new_tab(kUrl);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(registry->disabled_extensions().Contains(kExtensionId));
    EXPECT_EQ(disable_reason::DISABLE_USER_ACTION,
              prefs->GetDisableReasons(kExtensionId));
  }

  // Load a hosted app and disable it for a permissions increase.
  scoped_refptr<const Extension> hosted_app =
      ChromeTestExtensionLoader(profile()).LoadExtension(
          test_data_dir_.AppendASCII("hosted_app"));
  ASSERT_TRUE(hosted_app);
  const std::string kHostedAppId = hosted_app->id();
  const GURL kHostedAppUrl("http://localhost/extensions/hosted_app/main.html");
  EXPECT_EQ(hosted_app, registry->enabled_extensions().GetExtensionOrAppByURL(
                            kHostedAppUrl));

  extension_service()->DisableExtension(
      kHostedAppId, disable_reason::DISABLE_PERMISSIONS_INCREASE);
  EXPECT_TRUE(registry->disabled_extensions().Contains(kHostedAppId));
  EXPECT_EQ(disable_reason::DISABLE_PERMISSIONS_INCREASE,
            prefs->GetDisableReasons(kHostedAppId));

  {
    // When visiting a site that's associated with a hosted app, but not a
    // chrome-extension url, we don't prompt to re-enable. This is to avoid
    // prompting when visiting a regular website like calendar.google.com.
    // See crbug.com/678631.
    ScopedTestDialogAutoConfirm auto_accept(
        ScopedTestDialogAutoConfirm::ACCEPT);
    navigate_to_url_in_new_tab(kHostedAppUrl);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(registry->disabled_extensions().Contains(kHostedAppId));
    EXPECT_EQ(disable_reason::DISABLE_PERMISSIONS_INCREASE,
              prefs->GetDisableReasons(kHostedAppId));
  }
}

}  // namespace extensions
