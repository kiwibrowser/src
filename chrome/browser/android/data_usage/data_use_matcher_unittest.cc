// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/data_use_matcher.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/android/data_usage/data_use_tab_model.h"
#include "chrome/browser/android/data_usage/external_data_use_observer_bridge.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const char kUMAMatchingRulesCountValidHistogram[] =
    "DataUsage.MatchingRulesCount.Valid";
const char kUMAMatchingRulesCountInvalidHistogram[] =
    "DataUsage.MatchingRulesCount.Invalid";
const char kUMAURLRegexMatchDurationHistogram[] =
    "DataUsage.Perf.URLRegexMatchDuration";

const char kRegexFoo[] = "http://foo.com/";
const char kLabelFoo[] = "label_foo";
const char kAppFoo[] = "com.example.foo";

const uint32_t kDefaultMatchingRuleExpirationDurationSeconds =
    60 * 60 * 24;  // 24 hours.

class TestExternalDataUseObserverBridge
    : public android::ExternalDataUseObserverBridge {
 public:
  TestExternalDataUseObserverBridge() {}
  void FetchMatchingRules() const override {}
  void ShouldRegisterAsDataUseObserver(bool should_register) const override {}
};

}  // namespace

namespace android {

class ExternalDataUseObserver;

class DataUseMatcherTest : public testing::Test {
 public:
  DataUseMatcherTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        external_data_use_observer_bridge_(
            new TestExternalDataUseObserverBridge()),
        data_use_matcher_(
            base::Bind(&DataUseTabModel::OnTrackingLabelRemoved,
                       base::WeakPtr<DataUseTabModel>()),
            base::Bind(
                &ExternalDataUseObserverBridge::ShouldRegisterAsDataUseObserver,
                base::Unretained(external_data_use_observer_bridge_.get())),
            base::TimeDelta::FromSeconds(
                kDefaultMatchingRuleExpirationDurationSeconds)) {}

  DataUseMatcher* data_use_matcher() { return &data_use_matcher_; }

  void RegisterURLRegexes(const std::vector<std::string>& app_package_name,
                          const std::vector<std::string>& domain_path_regex,
                          const std::vector<std::string>& label) {
    data_use_matcher_.RegisterURLRegexes(app_package_name, domain_path_regex,
                                         label);
  }

  // Returns true if the matching rule at |index| is expired.
  bool IsExpired(size_t index) {
    DCHECK_LT(index, data_use_matcher_.matching_rules_.size());
    return data_use_matcher_.matching_rules_[index]->expiration() <
           data_use_matcher_.tick_clock_->NowTicks();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<ExternalDataUseObserverBridge>
      external_data_use_observer_bridge_;
  DataUseMatcher data_use_matcher_;
  DISALLOW_COPY_AND_ASSIGN(DataUseMatcherTest);
};

TEST_F(DataUseMatcherTest, SingleRegex) {
  const struct {
    std::string url;
    std::string regex;
    bool expect_match;
    int expect_count_valid_rules;
    int expect_count_url_match_duration_samples;
  } tests[] = {
      {"http://www.google.com", "http://www.google.com/", true, 1, 1},
      {"http://www.Google.com", "http://www.google.com/", true, 1, 1},
      {"http://www.googleacom", "http://www.google.com/", true, 1, 1},
      {"http://www.googleaacom", "http://www.google.com/", false, 1, 1},
      {"http://www.google.com", "https://www.google.com/", false, 1, 1},
      {"http://www.google.com", "{http|https}://www[.]google[.]com/search.*",
       false, 1, 1},
      {"https://www.google.com/search=test",
       "https://www[.]google[.]com/search.*", true, 1, 1},
      {"https://www.googleacom/search=test",
       "https://www[.]google[.]com/search.*", false, 1, 1},
      {"https://www.google.com/Search=test",
       "https://www[.]google[.]com/search.*", true, 1, 1},
      {"www.google.com", "http://www.google.com", false, 1, 0},
      {"www.google.com:80", "http://www.google.com", false, 1, 1},
      {"http://www.google.com:80", "http://www.google.com", false, 1, 1},
      {"http://www.google.com:80/", "http://www.google.com/", true, 1, 1},
      {"", "http://www.google.com", false, 1, 0},
      {"", "", false, 0, 0},
      {"https://www.google.com", "http://www.google.com", false, 1, 1},
      {"https://www.google.com", "[", false, 0},
      {"https://www.google.com", "]", false, 1, 1},
  };

  for (size_t i = 0; i < arraysize(tests); ++i) {
    base::HistogramTester histogram_tester;
    std::string label("");
    RegisterURLRegexes(
        // App package name not specified in the matching rule.
        std::vector<std::string>(1, std::string()),
        std::vector<std::string>(1, tests[i].regex),
        std::vector<std::string>(1, "label"));
    histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountValidHistogram,
                                        tests[i].expect_count_valid_rules, 1);
    histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountInvalidHistogram,
                                        1 - tests[i].expect_count_valid_rules,
                                        1);
    EXPECT_EQ(tests[i].expect_match,
              data_use_matcher()->MatchesURL(GURL(tests[i].url), &label))
        << i;
    histogram_tester.ExpectTotalCount(
        kUMAURLRegexMatchDurationHistogram,
        tests[i].expect_count_url_match_duration_samples);

    // Verify label matches the expected label.
    std::string expected_label = "";
    if (tests[i].expect_match)
      expected_label = "label";

    EXPECT_EQ(expected_label, label);
    EXPECT_FALSE(data_use_matcher()->MatchesAppPackageName(
        "com.example.helloworld", &label))
        << i;
    // Empty package name should not match against empty package name in the
    // matching rule.
    EXPECT_FALSE(
        data_use_matcher()->MatchesAppPackageName(std::string(), &label))
        << i;
  }
}

TEST_F(DataUseMatcherTest, TwoRegex) {
  const struct {
    std::string url;
    std::string regex1;
    std::string regex2;
    bool expect_match;
    int expect_count_valid_rules;
    int expect_count_url_match_duration_samples;
  } tests[] = {
      {"http://www.google.com", "http://www.google.com/",
       "https://www.google.com/", true, 1, 1},
      {"http://www.googleacom", "http://www.google.com/",
       "http://www.google.com/", true, 1, 1},
      {"https://www.google.com", "http://www.google.com/",
       "https://www.google.com/", true, 1, 1},
      {"https://www.googleacom", "http://www.google.com/",
       "https://www.google.com/", true, 1, 1},
      {"http://www.google.com", "{http|https}://www[.]google[.]com/search.*",
       "", false, 1, 1},
      {"http://www.google.com/search=test",
       "http://www[.]google[.]com/search.*",
       "https://www[.]google[.]com/search.*", true, 1, 1},
      {"https://www.google.com/search=test",
       "http://www[.]google[.]com/search.*",
       "https://www[.]google[.]com/search.*", true, 1, 1},
      {"http://google.com/search=test", "http://www[.]google[.]com/search.*",
       "https://www[.]google[.]com/search.*", false, 1, 1},
      {"https://www.googleacom/search=test", "",
       "https://www[.]google[.]com/search.*", false, 1, 1},
      {"https://www.google.com/Search=test", "",
       "https://www[.]google[.]com/search.*", true, 1, 1},
      {"www.google.com", "http://www.google.com", "", false, 1, 0},
      {"www.google.com:80", "http://www.google.com", "", false, 1, 1},
      {"http://www.google.com:80", "http://www.google.com", "", false, 1, 1},
      {"", "http://www.google.com", "", false, 1, 0},
      {"https://www.google.com", "http://www.google.com", "", false, 1, 1},
  };

  for (size_t i = 0; i < arraysize(tests); ++i) {
    base::HistogramTester histogram_tester;
    std::string got_label("");
    std::vector<std::string> url_regexes;
    url_regexes.push_back(tests[i].regex1 + "|" + tests[i].regex2);
    const std::string label("label");
    RegisterURLRegexes(
        std::vector<std::string>(url_regexes.size(), "com.example.helloworld"),
        url_regexes, std::vector<std::string>(url_regexes.size(), label));
    histogram_tester.ExpectTotalCount(kUMAMatchingRulesCountValidHistogram, 1);
    histogram_tester.ExpectTotalCount(kUMAMatchingRulesCountInvalidHistogram,
                                      1);
    histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountValidHistogram,
                                        tests[i].expect_count_valid_rules, 1);
    histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountInvalidHistogram,
                                        1 - tests[i].expect_count_valid_rules,
                                        1);
    EXPECT_EQ(tests[i].expect_match,
              data_use_matcher()->MatchesURL(GURL(tests[i].url), &got_label))
        << i;
    histogram_tester.ExpectTotalCount(
        kUMAURLRegexMatchDurationHistogram,
        tests[i].expect_count_url_match_duration_samples);
    const std::string expected_label =
        tests[i].expect_match ? label : std::string();
    EXPECT_EQ(expected_label, got_label);

    EXPECT_TRUE(data_use_matcher()->MatchesAppPackageName(
        "com.example.helloworld", &got_label))
        << i;
    EXPECT_EQ(label, got_label);
  }
}

TEST_F(DataUseMatcherTest, MultipleRegex) {
  base::HistogramTester histogram_tester;
  std::vector<std::string> url_regexes;
  url_regexes.push_back(
      "https?://www[.]google[.]com/#q=.*|https?://www[.]google[.]com[.]ph/"
      "#q=.*|https?://www[.]google[.]com[.]ph/[?]gws_rd=ssl#q=.*");
  RegisterURLRegexes(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      std::vector<std::string>(url_regexes.size(), "label"));
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountValidHistogram, 1,
                                      1);
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountInvalidHistogram, 0,
                                      1);

  const struct {
    std::string url;
    bool expect_match;
  } tests[] = {
      {"", false},
      {"http://www.google.com", false},
      {"http://www.googleacom", false},
      {"https://www.google.com", false},
      {"https://www.googleacom", false},
      {"https://www.google.com", false},
      {"quic://www.google.com/q=test", false},
      {"http://www.google.com/q=test", false},
      {"http://www.google.com/.q=test", false},
      {"http://www.google.com/#q=test", true},
      {"https://www.google.com/#q=test", true},
      {"https://www.google.com.ph/#q=test+abc", true},
      {"https://www.google.com.ph/?gws_rd=ssl#q=test+abc", true},
      {"http://www.google.com.ph/#q=test", true},
      {"https://www.google.com.ph/#q=test", true},
      {"http://www.google.co.in/#q=test", false},
      {"http://google.com/#q=test", false},
      {"https://www.googleacom/#q=test", false},
      {"https://www.google.com/#Q=test", true},  // case in-sensitive
      {"www.google.com/#q=test", false},
      {"www.google.com:80/#q=test", false},
      {"http://www.google.com:80/#q=test", true},
      {"http://www.google.com:80/search?=test", false},
  };

  for (size_t i = 0; i < arraysize(tests); ++i) {
    std::string label("");
    EXPECT_EQ(tests[i].expect_match,
              data_use_matcher()->MatchesURL(GURL(tests[i].url), &label))
        << i << " " << tests[i].url;
  }
}

TEST_F(DataUseMatcherTest, ChangeRegex) {
  std::string label;
  // When no regex is specified, the URL match should fail.
  EXPECT_FALSE(data_use_matcher()->MatchesURL(GURL(""), &label));
  EXPECT_FALSE(
      data_use_matcher()->MatchesURL(GURL("http://www.google.com"), &label));

  std::vector<std::string> url_regexes;
  url_regexes.push_back("http://www[.]google[.]com/#q=.*");
  url_regexes.push_back("https://www[.]google[.]com/#q=.*");
  RegisterURLRegexes(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      std::vector<std::string>(url_regexes.size(), "label"));

  EXPECT_FALSE(data_use_matcher()->MatchesURL(GURL(""), &label));
  EXPECT_TRUE(data_use_matcher()->MatchesURL(
      GURL("http://www.google.com#q=abc"), &label));
  EXPECT_FALSE(data_use_matcher()->MatchesURL(
      GURL("http://www.google.co.in#q=abc"), &label));

  // Change the regular expressions to verify that the new regexes replace
  // the ones specified before.
  url_regexes.clear();
  url_regexes.push_back("http://www[.]google[.]co[.]in/#q=.*");
  url_regexes.push_back("https://www[.]google[.]co[.]in/#q=.*");
  RegisterURLRegexes(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      std::vector<std::string>(url_regexes.size(), "label"));
  EXPECT_FALSE(data_use_matcher()->MatchesURL(GURL(""), &label));
  EXPECT_FALSE(data_use_matcher()->MatchesURL(
      GURL("http://www.google.com#q=abc"), &label));
  EXPECT_TRUE(data_use_matcher()->MatchesURL(
      GURL("http://www.google.co.in#q=abc"), &label));
}

TEST_F(DataUseMatcherTest, MultipleAppPackageName) {
  base::HistogramTester histogram_tester;
  std::vector<std::string> url_regexes;
  url_regexes.push_back(
      "http://www[.]foo[.]com/#q=.*|https://www[.]foo[.]com/#q=.*");
  url_regexes.push_back(
      "http://www[.]bar[.]com/#q=.*|https://www[.]bar[.]com/#q=.*");
  url_regexes.push_back("");

  std::vector<std::string> labels;
  const char kLabelBar[] = "label_bar";
  const char kLabelBaz[] = "label_baz";
  labels.push_back(kLabelFoo);
  labels.push_back(kLabelBar);
  labels.push_back(kLabelBaz);

  std::vector<std::string> app_package_names;
  const char kAppBar[] = "com.example.bar";
  const char kAppBaz[] = "com.example.baz";
  app_package_names.push_back(kAppFoo);
  app_package_names.push_back(kAppBar);
  app_package_names.push_back(kAppBaz);

  RegisterURLRegexes(app_package_names, url_regexes, labels);
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountValidHistogram, 3,
                                      1);
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountInvalidHistogram, 0,
                                      1);

  // Test if labels are matched properly for app package names.
  std::string got_label;
  EXPECT_TRUE(data_use_matcher()->MatchesAppPackageName(kAppFoo, &got_label));
  EXPECT_EQ(kLabelFoo, got_label);

  EXPECT_TRUE(data_use_matcher()->MatchesAppPackageName(kAppBar, &got_label));
  EXPECT_EQ(kLabelBar, got_label);

  EXPECT_TRUE(data_use_matcher()->MatchesAppPackageName(kAppBaz, &got_label));
  EXPECT_EQ(kLabelBaz, got_label);

  EXPECT_FALSE(data_use_matcher()->MatchesAppPackageName(
      "com.example.unmatched", &got_label));
  EXPECT_EQ(std::string(), got_label);

  EXPECT_FALSE(
      data_use_matcher()->MatchesAppPackageName(std::string(), &got_label));
  EXPECT_EQ(std::string(), got_label);

  EXPECT_FALSE(
      data_use_matcher()->MatchesAppPackageName(std::string(), &got_label));

  // An empty URL pattern should not match with any URL pattern.
  EXPECT_FALSE(data_use_matcher()->MatchesURL(GURL(""), &got_label));
  EXPECT_FALSE(
      data_use_matcher()->MatchesURL(GURL("http://www.baz.com"), &got_label));
}

TEST_F(DataUseMatcherTest, ParsePackageField) {
  const struct {
    std::string app_package_name;
    std::string expected_app_package_name;
    base::TimeDelta expected_expiration_duration;
  } tests[] = {
      {"", "", base::TimeDelta::FromSeconds(
                   kDefaultMatchingRuleExpirationDurationSeconds)},
      {"|", "|", base::TimeDelta::FromSeconds(
                     kDefaultMatchingRuleExpirationDurationSeconds)},
      {"|foo", "|foo", base::TimeDelta::FromSeconds(
                           kDefaultMatchingRuleExpirationDurationSeconds)},
      {"com.example.foo", "com.example.foo",
       base::TimeDelta::FromSeconds(
           kDefaultMatchingRuleExpirationDurationSeconds)},
      {"com.example.foo|", "com.example.foo|",
       base::TimeDelta::FromSeconds(
           kDefaultMatchingRuleExpirationDurationSeconds)},
      {"com.example.foo|foo", "com.example.foo|foo",
       base::TimeDelta::FromSeconds(
           kDefaultMatchingRuleExpirationDurationSeconds)},
      {"|0", "", base::TimeDelta::FromMilliseconds(0)},
      {"|100", "", base::TimeDelta::FromMilliseconds(100)},
      {"com.example.foo|0", "com.example.foo",
       base::TimeDelta::FromMilliseconds(0)},
      {"com.example.foo|10000", "com.example.foo",
       base::TimeDelta::FromMilliseconds(10000)},
  };
  base::SimpleTestTickClock tick_clock;
  // Set current time to to Epoch.
  tick_clock.SetNowTicks(base::TimeTicks::UnixEpoch());
  data_use_matcher()->tick_clock_ = &tick_clock;

  for (const auto& test : tests) {
    std::string new_app_package_name;
    base::TimeTicks expiration;
    data_use_matcher()->ParsePackageField(test.app_package_name,
                                          &new_app_package_name, &expiration);
    DCHECK_EQ(test.expected_app_package_name, new_app_package_name)
        << test.app_package_name;
    DCHECK_EQ(base::TimeTicks::UnixEpoch() + test.expected_expiration_duration,
              expiration)
        << test.app_package_name;
  }
}

// Tests if the expiration time encoded as milliseconds since epoch is parsed
// correctly.
TEST_F(DataUseMatcherTest, EncodeExpirationTimeInPackageName) {
  base::HistogramTester histogram_tester;
  base::SimpleTestTickClock tick_clock;
  data_use_matcher()->tick_clock_ = &tick_clock;

  std::vector<std::string> url_regexes, labels, app_package_names;
  url_regexes.push_back(kRegexFoo);
  labels.push_back(kLabelFoo);

  // Set current time to Epoch.
  tick_clock.SetNowTicks(base::TimeTicks::UnixEpoch());

  app_package_names.push_back(base::StringPrintf("%s|%d", kAppFoo, 10000));
  RegisterURLRegexes(app_package_names, url_regexes, labels);
  EXPECT_FALSE(IsExpired(0));
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountValidHistogram, 1,
                                      1);
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountInvalidHistogram, 0,
                                      1);
  // Fast forward 10 seconds, and matching rule expires.
  tick_clock.SetNowTicks(base::TimeTicks::UnixEpoch() +
                         base::TimeDelta::FromMilliseconds(10000 + 1));
  EXPECT_TRUE(IsExpired(0));

  // Empty app package name.
  app_package_names.clear();
  app_package_names.push_back(base::StringPrintf("|%d", 20000));
  RegisterURLRegexes(app_package_names, url_regexes, labels);
  EXPECT_FALSE(IsExpired(0));
  // Fast forward 20 seconds, and matching rule expires.
  tick_clock.SetNowTicks(base::TimeTicks::UnixEpoch() +
                         base::TimeDelta::FromMilliseconds(20000 + 1));
  EXPECT_TRUE(IsExpired(0));
}

// Tests if the expiration time encoded in Java format is parsed correctly.
TEST_F(DataUseMatcherTest, EncodeJavaExpirationTimeInPackageName) {
  base::HistogramTester histogram_tester;
  std::vector<std::string> url_regexes, labels, app_package_names;
  url_regexes.push_back(kRegexFoo);
  labels.push_back(kLabelFoo);

  base::TimeTicks start_ticks = base::TimeTicks::Now();
  base::Time expiration_time =
      base::Time::Now() + base::TimeDelta::FromMilliseconds(10000);

  app_package_names.push_back(base::StringPrintf(
      "%s|%lld", kAppFoo,
      static_cast<long long int>(expiration_time.ToJavaTime())));
  RegisterURLRegexes(app_package_names, url_regexes, labels);
  EXPECT_FALSE(IsExpired(0));
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountValidHistogram, 1,
                                      1);
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountInvalidHistogram, 0,
                                      1);

  // Check if expiration duration is close to 10 seconds.
  EXPECT_GE(base::TimeDelta::FromMilliseconds(10001),
            data_use_matcher()->matching_rules_[0]->expiration() -
                data_use_matcher()->tick_clock_->NowTicks());
  EXPECT_LE(base::TimeDelta::FromMilliseconds(9999),
            data_use_matcher()->matching_rules_[0]->expiration() - start_ticks);
}

// Tests that expired matching rules are ignored by MatchesURL and
// MatchesAppPackageName.
TEST_F(DataUseMatcherTest, MatchesIgnoresExpiredRules) {
  base::HistogramTester histogram_tester;
  std::vector<std::string> url_regexes, labels, app_package_names;
  std::string got_label;
  base::SimpleTestTickClock tick_clock;

  data_use_matcher()->tick_clock_ = &tick_clock;
  tick_clock.SetNowTicks(base::TimeTicks::UnixEpoch());

  url_regexes.push_back(kRegexFoo);
  labels.push_back(kLabelFoo);
  app_package_names.push_back(base::StringPrintf("%s|%d", kAppFoo, 10000));
  RegisterURLRegexes(app_package_names, url_regexes, labels);
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountValidHistogram, 1,
                                      1);
  histogram_tester.ExpectUniqueSample(kUMAMatchingRulesCountInvalidHistogram, 0,
                                      1);

  tick_clock.SetNowTicks(base::TimeTicks::UnixEpoch() +
                         base::TimeDelta::FromMilliseconds(1));

  EXPECT_FALSE(IsExpired(0));
  EXPECT_TRUE(data_use_matcher()->MatchesURL(GURL(kRegexFoo), &got_label));
  EXPECT_EQ(kLabelFoo, got_label);
  EXPECT_TRUE(data_use_matcher()->MatchesAppPackageName(kAppFoo, &got_label));
  EXPECT_EQ(kLabelFoo, got_label);

  // Advance time to make it expired.
  tick_clock.SetNowTicks(base::TimeTicks::UnixEpoch() +
                         base::TimeDelta::FromMilliseconds(10001));

  EXPECT_TRUE(IsExpired(0));
  EXPECT_FALSE(data_use_matcher()->MatchesURL(GURL(kRegexFoo), &got_label));
  EXPECT_FALSE(data_use_matcher()->MatchesAppPackageName(kAppFoo, &got_label));
}

}  // namespace android
