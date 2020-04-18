// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/extension_enable_flow.h"

#include "base/optional.h"
#include "base/run_loop.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/extensions/extension_enable_flow_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_id.h"

namespace {

class TestManagementProvider : public extensions::ManagementPolicy::Provider {
 public:
  explicit TestManagementProvider(const extensions::ExtensionId& extension_id)
      : extension_id_(extension_id) {}
  ~TestManagementProvider() override {}

  // MananagementPolicy::Provider:
  std::string GetDebugPolicyProviderName() const override { return "test"; }
  bool MustRemainDisabled(const extensions::Extension* extension,
                          extensions::disable_reason::DisableReason* reason,
                          base::string16* error) const override {
    return extension->id() == extension_id_;
  }

 private:
  const extensions::ExtensionId extension_id_;

  DISALLOW_COPY_AND_ASSIGN(TestManagementProvider);
};

class TestDelegate : public ExtensionEnableFlowDelegate {
 public:
  TestDelegate() {}
  ~TestDelegate() override {}

  enum Result {
    ABORTED,
    FINISHED,
  };

  void ExtensionEnableFlowFinished() override {
    result_ = FINISHED;
    run_loop_.Quit();
  }
  void ExtensionEnableFlowAborted(bool user_initiated) override {
    result_ = ABORTED;
    run_loop_.Quit();
  }

  void Wait() { run_loop_.Run(); }

  const base::Optional<Result>& result() const { return result_; }

 private:
  base::Optional<Result> result_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TestDelegate);
};

}  // namespace

using ExtensionEnableFlowBrowserTest = extensions::ExtensionBrowserTest;

// Test that trying to enable an extension that's blocked by policy fails
// gracefully. See https://crbug.com/783831.
IN_PROC_BROWSER_TEST_F(ExtensionEnableFlowBrowserTest,
                       TryEnablingPolicyForbiddenExtension) {
  scoped_refptr<const extensions::Extension> extension =
      extensions::ExtensionBuilder("extension").Build();
  extension_service()->AddExtension(extension.get());

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile());
  {
    extensions::ScopedTestDialogAutoConfirm auto_confirm(
        extensions::ScopedTestDialogAutoConfirm::ACCEPT);

    extensions::ManagementPolicy* management_policy =
        extensions::ExtensionSystem::Get(profile())->management_policy();
    ASSERT_TRUE(management_policy);
    TestManagementProvider test_provider(extension->id());
    management_policy->RegisterProvider(&test_provider);
    extension_service()->DisableExtension(
        extension->id(), extensions::disable_reason::DISABLE_BLOCKED_BY_POLICY);
    EXPECT_TRUE(registry->disabled_extensions().Contains(extension->id()));

    TestDelegate delegate;

    ExtensionEnableFlow enable_flow(profile(), extension->id(), &delegate);

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    enable_flow.StartForWebContents(web_contents);
    delegate.Wait();

    ASSERT_TRUE(delegate.result());
    EXPECT_EQ(TestDelegate::ABORTED, *delegate.result());

    EXPECT_TRUE(registry->disabled_extensions().Contains(extension->id()));

    management_policy->UnregisterProvider(&test_provider);
  }
}
