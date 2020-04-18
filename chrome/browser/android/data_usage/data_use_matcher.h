// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_MATCHER_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_MATCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
namespace base {
class TickClock;
}

namespace re2 {
class RE2;
}

class GURL;

namespace android {

// DataUseMatcher stores the matching URL patterns and package names along with
// the labels. It also provides functionality to get the matching label for a
// given URL or package. DataUseMatcher is not thread safe. It is created on IO
// thread, but immediately moved to the UI thread, and afterwards accessible
// only on the UI thread.
class DataUseMatcher {
 public:
  // |on_tracking_label_removed_callback| is the callback to be run when a
  // tracking label is removed from the list of matching rules.
  // |on_matching_rules_fetched_callback| is the callback to be run after
  // matching rules are fetched, indicating if at least one valid matching rule
  // is available.
  DataUseMatcher(
      const base::Callback<void(const std::string&)>&
          on_tracking_label_removed_callback,
      const base::Callback<void(bool)>& on_matching_rules_fetched_callback,
      const base::TimeDelta& default_matching_rule_expiration_duration);

  ~DataUseMatcher();

  // Called by FetchMatchingRulesDoneOnIOThread to register multiple
  // case-insensitive regular expressions. If the url of the data use request
  // matches any of the regular expression, the observation is passed to the
  // Java listener.
  void RegisterURLRegexes(const std::vector<std::string>& app_package_names,
                          const std::vector<std::string>& domain_path_regexes,
                          const std::vector<std::string>& labels);

  // Returns true if the |url| matches the registered regular expressions.
  // |label| must not be null. If a match is found, the |label| is set to the
  // matching rule's label.
  bool MatchesURL(const GURL& url, std::string* label) const WARN_UNUSED_RESULT;

  // Returns true if the |app_package_name| matches the registered package
  // names. |label| must not be null. If a match is found, the |label| is set
  // to the matching rule's label.
  bool MatchesAppPackageName(const std::string& app_package_name,
                             std::string* label) const WARN_UNUSED_RESULT;

  // Returns true if there is any matching rule. HasRules may return true even
  // if all rules are expired.
  bool HasRules() const;

  // Returns true if there is any valid matching rule with label |label|.
  bool HasValidRuleWithLabel(const std::string& label) const;

 private:
  friend class DataUseMatcherTest;
  FRIEND_TEST_ALL_PREFIXES(DataUseMatcherTest,
                           EncodeExpirationTimeInPackageName);
  FRIEND_TEST_ALL_PREFIXES(DataUseMatcherTest,
                           EncodeJavaExpirationTimeInPackageName);
  FRIEND_TEST_ALL_PREFIXES(DataUseMatcherTest, MatchesIgnoresExpiredRules);
  FRIEND_TEST_ALL_PREFIXES(DataUseMatcherTest, ParsePackageField);

  // Stores the matching rules.
  class MatchingRule {
   public:
    MatchingRule(const std::string& app_package_name,
                 std::unique_ptr<re2::RE2> pattern,
                 const std::string& label,
                 const base::TimeTicks& expiration);
    ~MatchingRule();

    const re2::RE2* pattern() const;
    const std::string& app_package_name() const;
    const std::string& label() const;
    const base::TimeTicks& expiration() const;

   private:
    // Package name of the app that should be matched.
    const std::string app_package_name_;

    // RE2 pattern to match against URLs.
    std::unique_ptr<re2::RE2> pattern_;

    // Opaque label that uniquely identifies this matching rule.
    const std::string label_;

    // Expiration time of this matching rule.
    const base::TimeTicks expiration_;

    DISALLOW_COPY_AND_ASSIGN(MatchingRule);
  };

  // Parses the app package name and expiration time of the matching rule
  // encoded in the format "app_package_name|milliseconds_since_epoch" in
  // |app_package_name|. |new_app_package_name| and |expiration| should not be
  // null. Parsed expiration time is set in |expiration| and app package name
  // is set in |new_app_package_name|. If |app_package_name| is not in the
  // expected format, |expiration| will be set to default expiration duration
  // from now, and |new_app_package_name| will be set to the |app_package_name|.
  void ParsePackageField(const std::string& app_package_name,
                         std::string* new_app_package_name,
                         base::TimeTicks* expiration) const;

  base::ThreadChecker thread_checker_;

  std::vector<std::unique_ptr<MatchingRule>> matching_rules_;

  // Default expiration duration of a matching rule, if expiration is not
  // specified in the rule.
  const base::TimeDelta default_matching_rule_expiration_duration_;

  // TickClock used for obtaining the current time.
  const base::TickClock* tick_clock_;

  // Callback to be run when a label is removed from the set of matching labels.
  const base::Callback<void(const std::string&)>
      on_tracking_label_removed_callback_;

  // Callback to be run when matching rules are fetched.
  const base::Callback<void(bool)> on_matching_rules_fetched_callback_;

  DISALLOW_COPY_AND_ASSIGN(DataUseMatcher);
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_MATCHER_H_
