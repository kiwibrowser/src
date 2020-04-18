// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_language_list.h"

#include <string>
#include <vector>

#include "base/test/scoped_command_line.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_url_util.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace translate {

// Test that the supported languages can be explicitly set using
// SetSupportedLanguages().
TEST(TranslateLanguageListTest, SetSupportedLanguages) {
  const std::string language_list(
      "{"
      "\"sl\":{\"en\":\"English\",\"ja\":\"Japanese\"},"
      "\"tl\":{\"en\":\"English\",\"ja\":\"Japanese\"}"
      "}");

  base::test::ScopedTaskEnvironment scoped_task_environment;
  TranslateDownloadManager* manager = TranslateDownloadManager::GetInstance();
  manager->set_application_locale("en");
  manager->set_request_context(new net::TestURLRequestContextGetter(
      base::ThreadTaskRunnerHandle::Get()));
  EXPECT_TRUE(manager->language_list()->SetSupportedLanguages(language_list));

  std::vector<std::string> results;
  manager->language_list()->GetSupportedLanguages(true /* translate_allowed */,
                                                  &results);
  ASSERT_EQ(2u, results.size());
  EXPECT_EQ("en", results[0]);
  EXPECT_EQ("ja", results[1]);
  manager->ResetForTesting();
}

// Test that the language code back-off of locale is done correctly (where
// required).
TEST(TranslateLanguageListTest, GetLanguageCode) {
  TranslateLanguageList language_list;
  EXPECT_EQ("en", language_list.GetLanguageCode("en"));
  // Test backoff of unsupported locale.
  EXPECT_EQ("en", language_list.GetLanguageCode("en-US"));
  // Test supported locale not backed off.
  EXPECT_EQ("zh-CN", language_list.GetLanguageCode("zh-CN"));
}

// Test that the translation URL is correctly generated, and that the
// translate-security-origin command-line flag correctly overrides the default
// value.
TEST(TranslateLanguageListTest, TranslateLanguageUrl) {
  TranslateLanguageList language_list;

  // Test default security origin.
  // The command-line override switch should not be set by default.
  EXPECT_FALSE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      "translate-security-origin"));
  EXPECT_EQ("https://translate.googleapis.com/translate_a/l?client=chrome",
            language_list.TranslateLanguageUrl().spec());

  // Test command-line security origin.
  base::test::ScopedCommandLine scoped_command_line;
  // Set the override switch.
  scoped_command_line.GetProcessCommandLine()->AppendSwitchASCII(
      "translate-security-origin", "https://example.com");
  EXPECT_EQ("https://example.com/translate_a/l?client=chrome",
            language_list.TranslateLanguageUrl().spec());
}

// Test that IsSupportedLanguage() is true for languages that should be
// supported, and false for invalid languages.
TEST(TranslateLanguageListTest, IsSupportedLanguage) {
  TranslateLanguageList language_list;
  EXPECT_TRUE(language_list.IsSupportedLanguage("en"));
  EXPECT_TRUE(language_list.IsSupportedLanguage("zh-CN"));
  EXPECT_FALSE(language_list.IsSupportedLanguage("xx"));
}

// Sanity test for the default set of supported languages. The default set of
// languages should be large (> 100) and must contain very common languages.
// If either of these tests are not true, the default language configuration is
// likely to be incorrect.
TEST(TranslateLanguageListTest, GetSupportedLanguages) {
  TranslateLanguageList language_list;
  std::vector<std::string> languages;
  language_list.GetSupportedLanguages(true /* translate_allowed */, &languages);
  // Check there are a lot of default languages.
  EXPECT_GE(languages.size(), 100ul);
  // Check that some very common languages are there.
  const auto begin = languages.begin();
  const auto end = languages.end();
  EXPECT_NE(end, std::find(begin, end, "en"));
  EXPECT_NE(end, std::find(begin, end, "es"));
  EXPECT_NE(end, std::find(begin, end, "fr"));
  EXPECT_NE(end, std::find(begin, end, "ru"));
  EXPECT_NE(end, std::find(begin, end, "zh-CN"));
  EXPECT_NE(end, std::find(begin, end, "zh-TW"));
}

// Check that we contact the translate server to update the supported language
// list when translate is enabled by policy.
TEST(TranslateLanguageListTest, GetSupportedLanguagesFetch) {
  // Set up fake network environment.
  base::test::ScopedTaskEnvironment scoped_task_environment;
  net::TestURLFetcherFactory url_fetcher_factory;
  TranslateDownloadManager::GetInstance()->set_application_locale("en");
  TranslateDownloadManager::GetInstance()->set_request_context(
      new net::TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get()));

  // Populate supported languages.
  std::vector<std::string> languages;
  TranslateLanguageList language_list;
  language_list.SetResourceRequestsAllowed(true);
  language_list.GetSupportedLanguages(true /* translate_allowed */, &languages);

  // Since translate is allowed by policy, we should have also scheduled a
  // language list fetch.
  net::TestURLFetcher* const fetcher =
      url_fetcher_factory.GetFetcherByID(TranslateLanguageList::kFetcherId);
  ASSERT_NE(nullptr, fetcher);

  // Check that the correct URL is requested.
  const GURL expected_url =
      AddApiKeyToUrl(AddHostLocaleToUrl(language_list.TranslateLanguageUrl()));
  const GURL actual_url = fetcher->GetOriginalURL();
  EXPECT_TRUE(actual_url.is_valid());
  EXPECT_EQ(expected_url.spec(), actual_url.spec());

  // Simulate fetch completion with just Italian in the supported language list.
  fetcher->set_status(net::URLRequestStatus());
  fetcher->set_response_code(net::HTTP_OK);
  fetcher->SetResponseString(R"({"tl" : {"it" : "Italian"}})");
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Check that the language list has been updated correctly.
  languages.clear();
  language_list.GetSupportedLanguages(true /* translate_allowed */, &languages);
  EXPECT_EQ(std::vector<std::string>(1, "it"), languages);
}

// Check that we don't send any network data when translate is disabled by
// policy.
TEST(TranslateLanguageListTest, GetSupportedLanguagesNoFetch) {
  // Set up fake network environment.
  base::test::ScopedTaskEnvironment scoped_task_environment;
  net::TestURLFetcherFactory url_fetcher_factory;
  TranslateDownloadManager::GetInstance()->set_application_locale("en");
  TranslateDownloadManager::GetInstance()->set_request_context(
      new net::TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get()));

  // Populate supported languages.
  std::vector<std::string> languages;
  TranslateLanguageList language_list;
  language_list.SetResourceRequestsAllowed(true);
  language_list.GetSupportedLanguages(false /* translate_allowed */,
                                      &languages);

  // Since translate is disabled by policy, we should *not* have scheduled a
  // language list fetch.
  net::TestURLFetcher* const fetcher =
      url_fetcher_factory.GetFetcherByID(TranslateLanguageList::kFetcherId);
  ASSERT_EQ(nullptr, fetcher);
}

}  // namespace translate
