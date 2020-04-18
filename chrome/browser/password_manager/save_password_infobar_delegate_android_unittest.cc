// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/save_password_infobar_delegate_android.h"

#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/fake_form_fetcher.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/stub_form_saver.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/stub_password_manager_driver.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using password_manager::PasswordFormMetricsRecorder;

namespace {

class MockPasswordFormManager : public password_manager::PasswordFormManager {
 public:
  MOCK_METHOD0(PermanentlyBlacklist, void());

  MockPasswordFormManager(
      password_manager::PasswordManager* password_manager,
      password_manager::PasswordManagerClient* client,
      base::WeakPtr<password_manager::PasswordManagerDriver> driver,
      const autofill::PasswordForm& form,
      password_manager::FormFetcher* form_fetcher)
      : PasswordFormManager(
            password_manager,
            client,
            driver,
            form,
            base::WrapUnique(new password_manager::StubFormSaver),
            form_fetcher) {}

  ~MockPasswordFormManager() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPasswordFormManager);
};

class TestSavePasswordInfobarDelegate : public SavePasswordInfoBarDelegate {
 public:
  TestSavePasswordInfobarDelegate(
      content::WebContents* web_contents,
      std::unique_ptr<password_manager::PasswordFormManager> form_to_save)
      : SavePasswordInfoBarDelegate(web_contents,
                                    std::move(form_to_save),
                                    true /* is_smartlock_branding_enabled */) {}

  ~TestSavePasswordInfobarDelegate() override {}
};

}  // namespace

class SavePasswordInfoBarDelegateTest : public ChromeRenderViewHostTestHarness {
 public:
  SavePasswordInfoBarDelegateTest();
  ~SavePasswordInfoBarDelegateTest() override {}

  void SetUp() override;
  void TearDown() override;

  PrefService* prefs();
  const autofill::PasswordForm& test_form() { return test_form_; }
  std::unique_ptr<MockPasswordFormManager> CreateMockFormManager(
      scoped_refptr<PasswordFormMetricsRecorder> metrics_recorder);

 protected:
  std::unique_ptr<ConfirmInfoBarDelegate> CreateDelegate(
      std::unique_ptr<password_manager::PasswordFormManager>
      password_form_manager);

  password_manager::StubPasswordManagerClient client_;
  password_manager::StubPasswordManagerDriver driver_;
  password_manager::PasswordManager password_manager_;

  autofill::PasswordForm test_form_;

 private:
  password_manager::FakeFormFetcher fetcher_;

  DISALLOW_COPY_AND_ASSIGN(SavePasswordInfoBarDelegateTest);
};

SavePasswordInfoBarDelegateTest::SavePasswordInfoBarDelegateTest()
    : password_manager_(&client_) {
  test_form_.origin = GURL("http://example.com");
  test_form_.username_value = base::ASCIIToUTF16("username");
  test_form_.password_value = base::ASCIIToUTF16("12345");
}

PrefService* SavePasswordInfoBarDelegateTest::prefs() {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  return profile->GetPrefs();
}

std::unique_ptr<MockPasswordFormManager>
SavePasswordInfoBarDelegateTest::CreateMockFormManager(
    scoped_refptr<PasswordFormMetricsRecorder> metrics_recorder) {
  auto manager = std::make_unique<MockPasswordFormManager>(
      &password_manager_, &client_, driver_.AsWeakPtr(), test_form(),
      &fetcher_);
  manager->Init(metrics_recorder);
  manager->ProvisionallySave(test_form());
  return manager;
}

std::unique_ptr<ConfirmInfoBarDelegate>
SavePasswordInfoBarDelegateTest::CreateDelegate(
    std::unique_ptr<password_manager::PasswordFormManager>
        password_form_manager) {
  std::unique_ptr<ConfirmInfoBarDelegate> delegate(
      new TestSavePasswordInfobarDelegate(web_contents(),
                                          std::move(password_form_manager)));
  return delegate;
}

void SavePasswordInfoBarDelegateTest::SetUp() {
  ChromeRenderViewHostTestHarness::SetUp();
}

void SavePasswordInfoBarDelegateTest::TearDown() {
  ChromeRenderViewHostTestHarness::TearDown();
}

TEST_F(SavePasswordInfoBarDelegateTest, CancelTestCredentialSourceAPI) {
  std::unique_ptr<MockPasswordFormManager> password_form_manager(
      CreateMockFormManager(nullptr));
  EXPECT_CALL(*password_form_manager.get(), PermanentlyBlacklist());
  std::unique_ptr<ConfirmInfoBarDelegate> infobar(
      CreateDelegate(std::move(password_form_manager)));
  EXPECT_TRUE(infobar->Cancel());
}

TEST_F(SavePasswordInfoBarDelegateTest,
       CancelTestCredentialSourcePasswordManager) {
  std::unique_ptr<MockPasswordFormManager> password_form_manager(
      CreateMockFormManager(nullptr));
  EXPECT_CALL(*password_form_manager.get(), PermanentlyBlacklist());
  std::unique_ptr<ConfirmInfoBarDelegate> infobar(
      CreateDelegate(std::move(password_form_manager)));
  EXPECT_TRUE(infobar->Cancel());
}

class SavePasswordInfoBarDelegateTestForUKMs
    : public SavePasswordInfoBarDelegateTest,
      public ::testing::WithParamInterface<
          PasswordFormMetricsRecorder::BubbleDismissalReason> {
 public:
  SavePasswordInfoBarDelegateTestForUKMs() = default;
  ~SavePasswordInfoBarDelegateTestForUKMs() = default;
};

// Verify that URL keyed metrics are recorded for showing and interacting with
// the password save prompt.
TEST_P(SavePasswordInfoBarDelegateTestForUKMs, VerifyUKMRecording) {
  using BubbleTrigger = PasswordFormMetricsRecorder::BubbleTrigger;
  using BubbleDismissalReason =
      PasswordFormMetricsRecorder::BubbleDismissalReason;
  using UkmEntry = ukm::builders::PasswordForm;

  BubbleDismissalReason dismissal_reason = GetParam();
  SCOPED_TRACE(::testing::Message() << "dismissal_reason = "
                                    << static_cast<int64_t>(dismissal_reason));

  ukm::SourceId expected_source_id = ukm::UkmRecorder::GetNewSourceID();
  ukm::TestAutoSetUkmRecorder test_ukm_recorder;
  {
    // Setup metrics recorder
    auto recorder = base::MakeRefCounted<PasswordFormMetricsRecorder>(
        true /*is_main_frame_secure*/, expected_source_id);

    // Exercise delegate.
    std::unique_ptr<MockPasswordFormManager> password_form_manager(
        CreateMockFormManager(recorder));
    if (dismissal_reason == BubbleDismissalReason::kDeclined)
      EXPECT_CALL(*password_form_manager.get(), PermanentlyBlacklist());
    std::unique_ptr<ConfirmInfoBarDelegate> infobar(
        CreateDelegate(std::move(password_form_manager)));
    switch (dismissal_reason) {
      case BubbleDismissalReason::kAccepted:
        EXPECT_TRUE(infobar->Accept());
        break;
      case BubbleDismissalReason::kDeclined:
        EXPECT_TRUE(infobar->Cancel());
        break;
      case BubbleDismissalReason::kIgnored:
        // Do nothing.
        break;
      case BubbleDismissalReason::kUnknown:
        NOTREACHED();
        break;
    }
  }

  // Verify metrics.
  const auto& entries =
      test_ukm_recorder.GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    EXPECT_EQ(expected_source_id, entry->source_id);
    test_ukm_recorder.ExpectEntryMetric(entry,
                                        UkmEntry::kSaving_Prompt_ShownName, 1);
    test_ukm_recorder.ExpectEntryMetric(
        entry, UkmEntry::kSaving_Prompt_TriggerName,
        static_cast<int64_t>(
            BubbleTrigger::kPasswordManagerSuggestionAutomatic));
    test_ukm_recorder.ExpectEntryMetric(
        entry, UkmEntry::kSaving_Prompt_InteractionName,
        static_cast<int64_t>(dismissal_reason));
  }
}

INSTANTIATE_TEST_CASE_P(
    /*no extra name*/,
    SavePasswordInfoBarDelegateTestForUKMs,
    ::testing::Values(
        PasswordFormMetricsRecorder::BubbleDismissalReason::kAccepted,
        PasswordFormMetricsRecorder::BubbleDismissalReason::kDeclined,
        PasswordFormMetricsRecorder::BubbleDismissalReason::kIgnored));
