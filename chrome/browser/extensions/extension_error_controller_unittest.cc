// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/extension_error_controller.h"
#include "chrome/browser/extensions/extension_error_ui.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"

namespace extensions {

namespace {

// Create a mock for the UI component of the error alert that is shown for
// blacklisted extensions. This allows us to test which extensions the alert
// is showing, and also eliminates the UI component (since this is a unit
// test).
class MockExtensionErrorUI : public ExtensionErrorUI {
 public:
  explicit MockExtensionErrorUI(ExtensionErrorUI::Delegate* delegate);
  ~MockExtensionErrorUI() override;

  // Wrappers around the similar methods in ExtensionErrorUI.
  void CloseUI();
  void Accept();
  void Details();

  ExtensionErrorUI::Delegate* delegate() { return delegate_; }

 private:
  // ExtensionErrorUI implementation.
  bool ShowErrorInBubbleView() override;
  void ShowExtensions() override;
  void Close() override;

  // Keep a copy of the delegate around for ourselves.
  ExtensionErrorUI::Delegate* delegate_;
};

// We use this as a slight hack to get the created Error UI, if any. We should
// only ever have one (since this is a single-profile test), and this avoids
// the need for any kind of accessor to the ErrorController from
// ExtensionService.
MockExtensionErrorUI* g_error_ui = NULL;

MockExtensionErrorUI::MockExtensionErrorUI(
    ExtensionErrorUI::Delegate* delegate)
    : ExtensionErrorUI(delegate),
      delegate_(delegate) {
  // We should never make more than one of these in a test.
  DCHECK(!g_error_ui);
  g_error_ui = this;
}

MockExtensionErrorUI::~MockExtensionErrorUI() {
  g_error_ui = NULL;
}

void MockExtensionErrorUI::CloseUI() {
  BubbleViewDidClose();
}

void MockExtensionErrorUI::Accept() {
  BubbleViewAcceptButtonPressed();
}

void MockExtensionErrorUI::Details() {
  BubbleViewCancelButtonPressed();
}

bool MockExtensionErrorUI::ShowErrorInBubbleView() {
  return true;
}

void MockExtensionErrorUI::ShowExtensions() {}

void MockExtensionErrorUI::Close() {
  CloseUI();
}

ExtensionErrorUI* CreateMockUI(ExtensionErrorUI::Delegate* delegate) {
  return new MockExtensionErrorUI(delegate);
}

// Builds and returns a simple extension.
scoped_refptr<const Extension> BuildExtension() {
  return ExtensionBuilder()
      .SetManifest(DictionaryBuilder().Set("name", "My Wonderful Extension")
                                      .Set("version", "0.1.1.0")
                                      .Set("manifest_version", 2)
                                      .Build())
      .Build();
}

}  // namespace

class ExtensionErrorControllerUnitTest : public ExtensionServiceTestBase {
 protected:
  void SetUp() override;

  // Add an extension to chrome, and mark it as blacklisted in the prefs.
  testing::AssertionResult AddBlacklistedExtension(const Extension* extension);

  // Return the ExtensionPrefs associated with the test.
  ExtensionPrefs* GetPrefs();

  Profile* profile() { return profile_.get(); }
};

void ExtensionErrorControllerUnitTest::SetUp() {
  ExtensionServiceTestBase::SetUp();
  // Make sure we use the mock UI instead of the real UI.
  ExtensionErrorController::SetUICreateMethodForTesting(CreateMockUI);

  // We don't want a first-run ExtensionService, since we ignore warnings
  // for new profiles.
  ExtensionServiceInitParams params = CreateDefaultInitParams();
  params.is_first_run = false;
  InitializeExtensionService(params);
}

testing::AssertionResult
ExtensionErrorControllerUnitTest::AddBlacklistedExtension(
    const Extension* extension) {
  GetPrefs()->SetExtensionBlacklistState(extension->id(),
                                         BLACKLISTED_MALWARE);
  service_->AddExtension(extension);

  // Make sure the extension is added to the blacklisted set.
  if (!ExtensionRegistry::Get(profile())->blacklisted_extensions()
          .Contains(extension->id())) {
    return testing::AssertionFailure()
        << "Failed to add blacklisted extension.";
  }

  return testing::AssertionSuccess();
}

ExtensionPrefs* ExtensionErrorControllerUnitTest::GetPrefs() {
  return ExtensionPrefs::Get(profile());
}

// Test that closing the extension alert for blacklisted extensions counts
// as acknowledging them in the prefs.
TEST_F(ExtensionErrorControllerUnitTest, ClosingAcknowledgesBlacklisted) {
  // Add a blacklisted extension.
  scoped_refptr<const Extension> extension = BuildExtension();
  ASSERT_TRUE(AddBlacklistedExtension(extension.get()));

  service_->Init();

  // Make sure that we created an error "ui" to warn about the blacklisted
  // extension.
  ASSERT_TRUE(g_error_ui);
  ExtensionErrorUI::Delegate* delegate = g_error_ui->delegate();
  ASSERT_TRUE(delegate);

  // Make sure that the blacklisted extension is reported (and that no other
  // extensions are).
  const ExtensionSet& delegate_blacklisted_extensions =
      delegate->GetBlacklistedExtensions();
  EXPECT_EQ(1u, delegate_blacklisted_extensions.size());
  EXPECT_TRUE(delegate_blacklisted_extensions.Contains(extension->id()));

  // Close, and verify that the extension ids now acknowledged.
  g_error_ui->CloseUI();
  EXPECT_TRUE(GetPrefs()->IsBlacklistedExtensionAcknowledged(extension->id()));
  // Verify we cleaned up after ourselves.
  EXPECT_FALSE(g_error_ui);
}

// Test that clicking "accept" on the extension alert counts as acknowledging
// blacklisted extensions.
TEST_F(ExtensionErrorControllerUnitTest, AcceptingAcknowledgesBlacklisted) {
  // Add a blacklisted extension.
  scoped_refptr<const Extension> extension = BuildExtension();
  ASSERT_TRUE(AddBlacklistedExtension(extension.get()));

  service_->Init();

  // Make sure that we created an error "ui" to warn about the blacklisted
  // extension.
  ASSERT_TRUE(g_error_ui);

  // Accept, and verify that the extension ids now acknowledged.
  g_error_ui->Accept();
  EXPECT_TRUE(GetPrefs()->IsBlacklistedExtensionAcknowledged(extension->id()));
  // Verify we cleaned up after ourselves.
  EXPECT_FALSE(g_error_ui);
}

// Test that we don't warn for extensions which are blacklisted, but have
// already been acknowledged.
TEST_F(ExtensionErrorControllerUnitTest, DontWarnForAcknowledgedBlacklisted) {
  scoped_refptr<const Extension> extension = BuildExtension();
  ASSERT_TRUE(AddBlacklistedExtension(extension.get()));

  GetPrefs()->AcknowledgeBlacklistedExtension(extension->id());

  service_->Init();

  // We should never have made an alert, because the extension should already
  // be acknowledged.
  ASSERT_FALSE(g_error_ui);
}

}  // namespace extensions
