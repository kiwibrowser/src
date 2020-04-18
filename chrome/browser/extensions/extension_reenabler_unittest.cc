// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/extension_reenabler.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/extension_system_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "components/crx_file/id_util.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/management_policy.h"
#include "extensions/browser/test_extensions_browser_client.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"

namespace extensions {

namespace {

// A simple provider that says all extensions must remain disabled.
class TestManagementProvider : public ManagementPolicy::Provider {
 public:
  TestManagementProvider() {}
  ~TestManagementProvider() override {}

 private:
  // MananagementPolicy::Provider:
  std::string GetDebugPolicyProviderName() const override { return "test"; }
  bool MustRemainDisabled(const Extension* extension,
                          disable_reason::DisableReason* reason,
                          base::string16* error) const override {
    return true;
  }

  DISALLOW_COPY_AND_ASSIGN(TestManagementProvider);
};

// A helper class for all the various callbacks associated with reenabling an
// extension. This class also helps store the results of the run.
class CallbackHelper {
 public:
  CallbackHelper() {}
  ~CallbackHelper() {}

  // Get a callback to run on the completion of the reenable process and reset
  // |result_|.
  ExtensionReenabler::Callback GetCallback() {
    result_.reset();
    return base::Bind(&CallbackHelper::OnComplete,
                      base::Unretained(this));
  }

  // Check if we have receved any result, and if it matches the expected one.
  bool has_result() const { return result_ != nullptr; }
  bool result_matches(ExtensionReenabler::ReenableResult expected) const {
    return result_.get() && *result_ == expected;
  }

  // Create a test ExtensionInstallPrompt that will not display any UI (which
  // causes unit tests to crash), but rather runs the given |quit_closure| (with
  // the prompt still active|.
  ExtensionInstallPrompt::ShowDialogCallback CreateShowCallback(
      base::OnceClosure quit_closure) {
    quit_closure_ = std::move(quit_closure);
    return base::Bind(&CallbackHelper::OnShow, base::Unretained(this));
  }

 private:
  // The callback to run once the reenable process finishes.
  void OnComplete(ExtensionReenabler::ReenableResult result) {
    result_.reset(new ExtensionReenabler::ReenableResult(result));
  }

  // The callback to run when a test ExtensionInstallPrompt is ready to show.
  void OnShow(ExtensionInstallPromptShowParams* show_params,
              const ExtensionInstallPrompt::DoneCallback& done_callback,
              std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt) {
    DCHECK(quit_closure_);
    std::move(quit_closure_).Run();
  }

  // The closure to quit the currently-running loop; used with test
  // ExtensionInstallPrompts.
  base::OnceClosure quit_closure_;

  // The result of the reenable process, or null if the process hasn't finished.
  std::unique_ptr<ExtensionReenabler::ReenableResult> result_;

  DISALLOW_COPY_AND_ASSIGN(CallbackHelper);
};

}  // namespace

class ExtensionReenablerUnitTest : public ExtensionServiceTestBase {
 public:
  ExtensionReenablerUnitTest() {}
  ~ExtensionReenablerUnitTest() override {}

 private:
  void SetUp() override;
  void TearDown() override;

  std::unique_ptr<TestExtensionsBrowserClient> test_browser_client_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionReenablerUnitTest);
};

void ExtensionReenablerUnitTest::SetUp() {
  ExtensionServiceTestBase::SetUp();
  InitializeEmptyExtensionService();
  // We need a TestExtensionsBrowserClient because the real one tries to
  // implicitly convert any browser context to a (non-Testing)Profile.
  test_browser_client_.reset(new TestExtensionsBrowserClient(profile()));
  test_browser_client_->set_extension_system_factory(
      ExtensionSystemFactory::GetInstance());
  ExtensionsBrowserClient::Set(test_browser_client_.get());
}

void ExtensionReenablerUnitTest::TearDown() {
  profile_.reset();
  ExtensionsBrowserClient::Set(nullptr);
  test_browser_client_.reset();
  ExtensionServiceTestBase::TearDown();
}

// Test that the ExtensionReenabler reenables disabled extensions.
TEST_F(ExtensionReenablerUnitTest, TestReenablingDisabledExtension) {
  // Create a simple extension and add it to the service.
  scoped_refptr<const Extension> extension =
      ExtensionBuilder()
          .SetManifest(DictionaryBuilder()
                           .Set("name", "test ext")
                           .Set("version", "1.0")
                           .Set("manifest_version", 2)
                           .Set("description", "a test ext")
                           .Build())
          .SetID(crx_file::id_util::GenerateId("test ext"))
          .Build();
  service()->AddExtension(extension.get());
  EXPECT_TRUE(registry()->enabled_extensions().Contains(extension->id()));

  CallbackHelper callback_helper;

  // Check that the ExtensionReenabler can re-enable disabled extensions.
  {
    // Disable the extension due to a permissions increase (the only type of
    // disablement we handle with the ExtensionReenabler so far).
    service()->DisableExtension(extension->id(),
                                disable_reason::DISABLE_PERMISSIONS_INCREASE);
    // Sanity check that it's disabled.
    EXPECT_TRUE(registry()->disabled_extensions().Contains(extension->id()));

    // Automatically confirm install prompts.
    ScopedTestDialogAutoConfirm auto_confirm(
        ScopedTestDialogAutoConfirm::ACCEPT);

    // Run the ExtensionReenabler.
    std::unique_ptr<ExtensionReenabler> extension_reenabler =
        ExtensionReenabler::PromptForReenable(extension, profile(),
                                              nullptr,  // No web contents.
                                              GURL(),   // No referrer.
                                              callback_helper.GetCallback());
    base::RunLoop().RunUntilIdle();

    // The extension should be enabled.
    EXPECT_TRUE(registry()->enabled_extensions().Contains(extension->id()));
    EXPECT_TRUE(
        callback_helper.result_matches(ExtensionReenabler::REENABLE_SUCCESS));
  }

  // Check that we don't re-enable extensions that must remain disabled, and
  // that the re-enabler reports failure correctly.
  {
    ScopedTestDialogAutoConfirm auto_confirm(
        ScopedTestDialogAutoConfirm::ACCEPT);

    ManagementPolicy* management_policy =
        ExtensionSystem::Get(browser_context())->management_policy();
    ASSERT_TRUE(management_policy);
    TestManagementProvider test_provider;
    management_policy->RegisterProvider(&test_provider);
    service()->DisableExtension(extension->id(),
                                disable_reason::DISABLE_PERMISSIONS_INCREASE);

    std::unique_ptr<ExtensionReenabler> extension_reenabler =
        ExtensionReenabler::PromptForReenable(extension, profile(),
                                              nullptr,  // No web contents.
                                              GURL(),   // No referrer.
                                              callback_helper.GetCallback());
    base::RunLoop().RunUntilIdle();

    // The extension should be enabled.
    EXPECT_TRUE(registry()->disabled_extensions().Contains(extension->id()));
    EXPECT_TRUE(
        callback_helper.result_matches(ExtensionReenabler::NOT_ALLOWED));

    management_policy->UnregisterProvider(&test_provider);
  }

  // Check that canceling the re-enable prompt doesn't re-enable the extension.
  {
    // Disable it again, and try canceling the prompt.
    service()->DisableExtension(extension->id(),
                                disable_reason::DISABLE_PERMISSIONS_INCREASE);
    ScopedTestDialogAutoConfirm auto_confirm(
        ScopedTestDialogAutoConfirm::CANCEL);
    std::unique_ptr<ExtensionReenabler> extension_reenabler =
        ExtensionReenabler::PromptForReenable(extension, profile(),
                                              nullptr,  // No web contents.
                                              GURL(),   // No referrer.
                                              callback_helper.GetCallback());
    base::RunLoop().RunUntilIdle();

    // The extension should remain disabled.
    EXPECT_TRUE(registry()->disabled_extensions().Contains(extension->id()));
    EXPECT_TRUE(
        callback_helper.result_matches(ExtensionReenabler::USER_CANCELED));
  }

  // Test that if the extension is re-enabled while the prompt is active, the
  // prompt exits and reports success.
  {
    base::RunLoop run_loop;
    std::unique_ptr<ExtensionReenabler> extension_reenabler =
        ExtensionReenabler::PromptForReenableWithCallbackForTest(
            extension, profile(), callback_helper.GetCallback(),
            callback_helper.CreateShowCallback(run_loop.QuitClosure()));
    run_loop.Run();

    // We shouldn't have any result yet (the user hasn't confirmed or canceled).
    EXPECT_FALSE(callback_helper.has_result());

    // Reenable the extension. This should count as a success for reenabling.
    service()->GrantPermissionsAndEnableExtension(extension.get());
    EXPECT_TRUE(
        callback_helper.result_matches(ExtensionReenabler::REENABLE_SUCCESS));
  }

  // Test that prematurely destroying the re-enable prompt doesn't crash and
  // reports an "aborted" result.
  {
    // Disable again, and create another prompt.
    service()->DisableExtension(extension->id(),
                                disable_reason::DISABLE_PERMISSIONS_INCREASE);
    base::RunLoop run_loop;
    std::unique_ptr<ExtensionReenabler> extension_reenabler =
        ExtensionReenabler::PromptForReenableWithCallbackForTest(
            extension, profile(), callback_helper.GetCallback(),
            callback_helper.CreateShowCallback(run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_FALSE(callback_helper.has_result());
    // Destroy the reenabler to simulate the owning context being shut down
    // (e.g., the tab closing).
    extension_reenabler.reset();
    EXPECT_TRUE(
        callback_helper.result_matches(ExtensionReenabler::ABORTED));
  }
}

}  // namespace extensions
