// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/counters/browsing_data_counter_utils.h"

#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browsing_data/counters/cache_counter.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "base/strings/string_split.h"
#include "chrome/browser/browsing_data/counters/hosted_apps_counter.h"
#endif

class BrowsingDataCounterUtilsTest : public testing::Test {
 public:
  BrowsingDataCounterUtilsTest() {}
  ~BrowsingDataCounterUtilsTest() override {}

  TestingProfile* GetProfile() { return &profile_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
};

TEST_F(BrowsingDataCounterUtilsTest, CacheCounterResult) {
  // This test assumes that the strings are served exactly as defined,
  // i.e. that the locale is set to the default "en".
  ASSERT_EQ("en", TestingBrowserProcess::GetGlobal()->GetApplicationLocale());
  const int kBytesInAMegabyte = 1024 * 1024;

  // Test the output for various forms of CacheResults.
  const struct TestCase {
    int bytes;
    bool is_upper_limit;
    bool is_basic_tab;
    std::string expected_output;
  } kTestCases[] = {
      {42, false, false, "Less than 1 MB"},
      {42, false, true,
       "Frees up less than 1 MB. Some sites may load more slowly on your next "
       "visit."},
      {2.312 * kBytesInAMegabyte, false, false, "2.3 MB"},
      {2.312 * kBytesInAMegabyte, false, true,
       "Frees up 2.3 MB. Some sites may load more slowly on your next visit."},
      {2.312 * kBytesInAMegabyte, true, false, "Less than 2.3 MB"},
      {2.312 * kBytesInAMegabyte, true, true,
       "Frees up less than 2.3 MB. Some sites may load more slowly on your "
       "next visit."},
      {500.2 * kBytesInAMegabyte, false, false, "500 MB"},
      {500.2 * kBytesInAMegabyte, true, false, "Less than 500 MB"},
  };

  for (const TestCase& test_case : kTestCases) {
    CacheCounter counter(GetProfile());
    browsing_data::ClearBrowsingDataTab tab =
        test_case.is_basic_tab ? browsing_data::ClearBrowsingDataTab::BASIC
                               : browsing_data::ClearBrowsingDataTab::ADVANCED;
    counter.Init(GetProfile()->GetPrefs(), tab,
                 browsing_data::BrowsingDataCounter::Callback());
    CacheCounter::CacheResult result(&counter, test_case.bytes,
                                     test_case.is_upper_limit);
    SCOPED_TRACE(base::StringPrintf(
        "Test params: %d bytes, %d is_upper_limit, %d is_basic_tab.",
        test_case.bytes, test_case.is_upper_limit, test_case.is_basic_tab));

    base::string16 output =
        GetChromeCounterTextFromResult(&result, GetProfile());
    EXPECT_EQ(output, base::ASCIIToUTF16(test_case.expected_output));
  }
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
// Tests the complex output of the hosted apps counter.
TEST_F(BrowsingDataCounterUtilsTest, HostedAppsCounterResult) {
  HostedAppsCounter counter(GetProfile());

  // This test assumes that the strings are served exactly as defined,
  // i.e. that the locale is set to the default "en".
  ASSERT_EQ("en", TestingBrowserProcess::GetGlobal()->GetApplicationLocale());

  // Test the output for various numbers of hosted apps.
  const struct TestCase {
    std::string apps_list;
    std::string expected_output;
  } kTestCases[] = {
      {"", "None"},
      {"App1", "1 app (App1)"},
      {"App1, App2", "2 apps (App1, App2)"},
      {"App1, App2, App3", "3 apps (App1, App2, and 1 more)"},
      {"App1, App2, App3, App4", "4 apps (App1, App2, and 2 more)"},
      {"App1, App2, App3, App4, App5", "5 apps (App1, App2, and 3 more)"},
  };

  for (const TestCase& test_case : kTestCases) {
    // Split the list of installed apps by commas.
    std::vector<std::string> apps = base::SplitString(
        test_case.apps_list, ",",
        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    // The first two apps in the list are used as examples.
    std::vector<std::string> examples;
    examples.assign(
        apps.begin(), apps.begin() + (apps.size() > 2 ? 2 : apps.size()));

    HostedAppsCounter::HostedAppsResult result(
        &counter,
        apps.size(),
        examples);

    base::string16 output =
        GetChromeCounterTextFromResult(&result, GetProfile());
    EXPECT_EQ(output, base::ASCIIToUTF16(test_case.expected_output));
  }
}
#endif
