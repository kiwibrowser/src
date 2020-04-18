// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/language_settings_private/language_settings_private_api.h"
#include "chrome/browser/extensions/api/language_settings_private/language_settings_private_delegate.h"
#include "chrome/browser/extensions/api/language_settings_private/language_settings_private_delegate_factory.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "extensions/browser/event_router_factory.h"
#include "extensions/browser/extension_prefs.h"

namespace extensions {

typedef api::language_settings_private::SpellcheckDictionaryStatus
    DictionaryStatus;

class MockLanguageSettingsPrivateDelegate
    : public LanguageSettingsPrivateDelegate {
 public:
  explicit MockLanguageSettingsPrivateDelegate(content::BrowserContext* context)
      : LanguageSettingsPrivateDelegate(context) {}
  ~MockLanguageSettingsPrivateDelegate() override {}

  // LanguageSettingsPrivateDelegate:
  std::vector<DictionaryStatus> GetHunspellDictionaryStatuses() override;
  void RetryDownloadHunspellDictionary(const std::string& language) override;

  std::vector<std::string> retry_download_hunspell_dictionary_called_with() {
    return retry_download_hunspell_dictionary_called_with_;
  }

 private:
  std::vector<std::string> retry_download_hunspell_dictionary_called_with_;
};

std::vector<DictionaryStatus>
MockLanguageSettingsPrivateDelegate::GetHunspellDictionaryStatuses() {
  std::vector<DictionaryStatus> statuses;
  DictionaryStatus status;
  status.language_code = "fr";
  status.is_ready = false;
  status.is_downloading = std::make_unique<bool>(true);
  status.download_failed = std::make_unique<bool>(false);
  statuses.push_back(std::move(status));
  return statuses;
}

void MockLanguageSettingsPrivateDelegate::RetryDownloadHunspellDictionary(
    const std::string& language) {
  retry_download_hunspell_dictionary_called_with_.push_back(language);
}

namespace {

std::unique_ptr<KeyedService> BuildEventRouter(
    content::BrowserContext* profile) {
  return std::make_unique<EventRouter>(profile, ExtensionPrefs::Get(profile));
}

std::unique_ptr<KeyedService> BuildLanguageSettingsPrivateDelegate(
    content::BrowserContext* profile) {
  return std::make_unique<MockLanguageSettingsPrivateDelegate>(profile);
}

}  // namespace

class LanguageSettingsPrivateApiTest : public ExtensionServiceTestBase {
 public:
  LanguageSettingsPrivateApiTest() = default;
  ~LanguageSettingsPrivateApiTest() override = default;

 private:
  void SetUp() override {
    ExtensionServiceTestBase::SetUp();
    ExtensionServiceTestBase::InitializeEmptyExtensionService();
    EventRouterFactory::GetInstance()->SetTestingFactory(profile(),
                                                         &BuildEventRouter);

    LanguageSettingsPrivateDelegateFactory::GetInstance()->SetTestingFactory(
        profile(), &BuildLanguageSettingsPrivateDelegate);
  }

  void TearDown() override { ExtensionServiceTestBase::TearDown(); }

  std::unique_ptr<TestBrowserWindow> browser_window_;
  std::unique_ptr<Browser> browser_;
};

TEST_F(LanguageSettingsPrivateApiTest, RetryDownloadHunspellDictionaryTest) {
  MockLanguageSettingsPrivateDelegate* mock_delegate =
      static_cast<MockLanguageSettingsPrivateDelegate*>(
          LanguageSettingsPrivateDelegateFactory::GetForBrowserContext(
              browser_context()));

  auto function = base::MakeRefCounted<
      LanguageSettingsPrivateRetryDownloadDictionaryFunction>();

  EXPECT_EQ(
      0u,
      mock_delegate->retry_download_hunspell_dictionary_called_with().size());
  EXPECT_TRUE(
      api_test_utils::RunFunction(function.get(), "[\"fr\"]", profile()))
      << function->GetError();
  EXPECT_EQ(
      1u,
      mock_delegate->retry_download_hunspell_dictionary_called_with().size());
  EXPECT_EQ(
      "fr",
      mock_delegate->retry_download_hunspell_dictionary_called_with().front());
}

TEST_F(LanguageSettingsPrivateApiTest, GetSpellcheckDictionaryStatusesTest) {
  auto function = base::MakeRefCounted<
      LanguageSettingsPrivateGetSpellcheckDictionaryStatusesFunction>();

  std::unique_ptr<base::Value> actual =
      api_test_utils::RunFunctionAndReturnSingleResult(function.get(), "[]",
                                                       profile());
  EXPECT_TRUE(actual) << function->GetError();

  base::ListValue expected;
  auto expected_status = std::make_unique<base::DictionaryValue>();
  expected_status->SetString("languageCode", "fr");
  expected_status->SetBoolean("isReady", false);
  expected_status->SetBoolean("isDownloading", true);
  expected_status->SetBoolean("downloadFailed", false);
  expected.Append(std::move(expected_status));
  EXPECT_EQ(expected, *actual);
}

}  // namespace extensions
