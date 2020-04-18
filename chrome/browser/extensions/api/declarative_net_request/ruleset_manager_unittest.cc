// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/api/declarative_net_request/dnr_test_base.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/extensions/extension_util.h"
#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"
#include "extensions/browser/api/declarative_net_request/test_utils.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/test_utils.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/url_pattern.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace extensions {
namespace declarative_net_request {
namespace {

constexpr char kJSONRulesFilename[] = "rules_file.json";
const base::FilePath::CharType kJSONRulesetFilepath[] =
    FILE_PATH_LITERAL("rules_file.json");

class RulesetManagerTest : public DNRTestBase {
 public:
  RulesetManagerTest() {}

 protected:
  using Action = RulesetManager::Action;

  // Helper to create a ruleset matcher instance for the given |rules|.
  void CreateMatcherForRules(const std::vector<TestRule>& rules,
                             const std::string& extension_dirname,
                             std::unique_ptr<RulesetMatcher>* matcher,
                             const std::vector<std::string>& host_permissions =
                                 {URLPattern::kAllUrlsPattern},
                             bool has_background_script = false) {
    base::FilePath extension_dir =
        temp_dir().GetPath().AppendASCII(extension_dirname);

    // Create extension directory.
    ASSERT_TRUE(base::CreateDirectory(extension_dir));
    WriteManifestAndRuleset(extension_dir, kJSONRulesetFilepath,
                            kJSONRulesFilename, rules, host_permissions,
                            has_background_script);

    last_loaded_extension_ =
        CreateExtensionLoader()->LoadExtension(extension_dir);
    ASSERT_TRUE(last_loaded_extension_);

    // This is required since we mock ExtensionSystem in our tests.
    SimulateAddExtensionOnIOThread(last_loaded_extension(),
                                   false /*incognito_enabled*/);

    int expected_checksum;
    EXPECT_TRUE(ExtensionPrefs::Get(browser_context())
                    ->GetDNRRulesetChecksum(last_loaded_extension_->id(),
                                            &expected_checksum));

    EXPECT_EQ(
        RulesetMatcher::kLoadSuccess,
        RulesetMatcher::CreateVerifiedMatcher(
            file_util::GetIndexedRulesetPath(last_loaded_extension_->path()),
            expected_checksum, matcher));
  }

  void SetIncognitoEnabled(const Extension* extension, bool incognito_enabled) {
    util::SetIsIncognitoEnabled(extension->id(), browser_context(),
                                incognito_enabled);

    // This is required since we mock ExtensionSystem in our tests.
    SimulateAddExtensionOnIOThread(extension, incognito_enabled);
  }

  InfoMap* info_map() {
    return ExtensionSystem::Get(browser_context())->info_map();
  }

  const Extension* last_loaded_extension() const {
    return last_loaded_extension_.get();
  }

  std::unique_ptr<net::URLRequest> GetRequestForURL(const std::string url) {
    return request_context_.CreateRequest(GURL(url), net::DEFAULT_PRIORITY,
                                          &delegate_,
                                          TRAFFIC_ANNOTATION_FOR_TESTS);
  }

 private:
  void SimulateAddExtensionOnIOThread(const Extension* extension,
                                      bool incognito_enabled) {
    info_map()->AddExtension(extension, base::Time(), incognito_enabled,
                             false /*notifications_disabled*/);
  }

  net::TestURLRequestContext request_context_;
  net::TestDelegate delegate_;
  scoped_refptr<const Extension> last_loaded_extension_;

  DISALLOW_COPY_AND_ASSIGN(RulesetManagerTest);
};

// Tests that the RulesetManager handles multiple rulesets correctly.
TEST_P(RulesetManagerTest, MultipleRulesets) {
  enum RulesetMask {
    kEnableRulesetOne = 1 << 0,
    kEnableRulesetTwo = 1 << 1,
  };

  TestRule rule_one = CreateGenericRule();
  rule_one.condition->url_filter = std::string("one.com");

  TestRule rule_two = CreateGenericRule();
  rule_two.condition->url_filter = std::string("two.com");

  RulesetManager* manager = info_map()->GetRulesetManager();
  ASSERT_TRUE(manager);

  std::unique_ptr<net::URLRequest> request_one =
      GetRequestForURL("http://one.com");
  WebRequestInfo request_one_info(request_one.get());
  std::unique_ptr<net::URLRequest> request_two =
      GetRequestForURL("http://two.com");
  WebRequestInfo request_two_info(request_two.get());
  std::unique_ptr<net::URLRequest> request_three =
      GetRequestForURL("http://three.com");
  WebRequestInfo request_three_info(request_three.get());

  auto should_block_request = [manager](const WebRequestInfo& request) {
    GURL redirect_url;
    return manager->EvaluateRequest(request, false /*is_incognito_context*/,
                                    &redirect_url) == Action::BLOCK;
  };

  for (int mask = 0; mask < 4; mask++) {
    SCOPED_TRACE(base::StringPrintf("Testing ruleset mask %d", mask));

    ASSERT_EQ(0u, manager->GetMatcherCountForTest());

    std::string extension_id_one, extension_id_two;
    size_t expected_matcher_count = 0;

    // Add the required rulesets.
    if (mask & kEnableRulesetOne) {
      ++expected_matcher_count;
      std::unique_ptr<RulesetMatcher> matcher;
      ASSERT_NO_FATAL_FAILURE(CreateMatcherForRules(
          {rule_one}, std::to_string(mask) + "_one", &matcher));
      extension_id_one = last_loaded_extension()->id();
      manager->AddRuleset(extension_id_one, std::move(matcher));
    }
    if (mask & kEnableRulesetTwo) {
      ++expected_matcher_count;
      std::unique_ptr<RulesetMatcher> matcher;
      ASSERT_NO_FATAL_FAILURE(CreateMatcherForRules(
          {rule_two}, std::to_string(mask) + "_two", &matcher));
      extension_id_two = last_loaded_extension()->id();
      manager->AddRuleset(extension_id_two, std::move(matcher));
    }

    ASSERT_EQ(expected_matcher_count, manager->GetMatcherCountForTest());

    EXPECT_EQ((mask & kEnableRulesetOne) != 0,
              should_block_request(request_one_info));
    EXPECT_EQ((mask & kEnableRulesetTwo) != 0,
              should_block_request(request_two_info));
    EXPECT_FALSE(should_block_request(request_three_info));

    // Remove the rulesets.
    if (mask & kEnableRulesetOne)
      manager->RemoveRuleset(extension_id_one);
    if (mask & kEnableRulesetTwo)
      manager->RemoveRuleset(extension_id_two);
  }
}

// Tests that only extensions enabled in incognito mode can modify requests made
// from the incognito context.
TEST_P(RulesetManagerTest, IncognitoRequests) {
  RulesetManager* manager = info_map()->GetRulesetManager();
  ASSERT_TRUE(manager);

  // Add an extension ruleset blocking "example.com".
  TestRule rule_one = CreateGenericRule();
  rule_one.condition->url_filter = std::string("example.com");
  std::unique_ptr<RulesetMatcher> matcher;
  ASSERT_NO_FATAL_FAILURE(
      CreateMatcherForRules({rule_one}, "test_extension", &matcher));
  manager->AddRuleset(last_loaded_extension()->id(), std::move(matcher));

  std::unique_ptr<net::URLRequest> request =
      GetRequestForURL("http://example.com");
  WebRequestInfo request_info(request.get());

  GURL redirect_url;

  // By default, the extension is disabled in incognito mode. So requests from
  // incognito contexts should not be evaluated.
  EXPECT_FALSE(util::IsIncognitoEnabled(last_loaded_extension()->id(),
                                        browser_context()));
  EXPECT_EQ(Action::NONE,
            manager->EvaluateRequest(
                request_info, true /*is_incognito_context*/, &redirect_url));
  EXPECT_EQ(Action::BLOCK,
            manager->EvaluateRequest(
                request_info, false /*is_incognito_context*/, &redirect_url));

  // Enabling the extension in incognito mode, should cause requests from
  // incognito contexts to also be evaluated.
  SetIncognitoEnabled(last_loaded_extension(), true /*incognito_enabled*/);
  EXPECT_TRUE(util::IsIncognitoEnabled(last_loaded_extension()->id(),
                                       browser_context()));
  EXPECT_EQ(Action::BLOCK,
            manager->EvaluateRequest(
                request_info, true /*is_incognito_context*/, &redirect_url));
  EXPECT_EQ(Action::BLOCK,
            manager->EvaluateRequest(
                request_info, false /*is_incognito_context*/, &redirect_url));
}

// Test redirect rules.
TEST_P(RulesetManagerTest, Redirect) {
  RulesetManager* manager = info_map()->GetRulesetManager();
  ASSERT_TRUE(manager);

  // Add an extension ruleset which redirects "example.com" to "google.com".
  TestRule rule = CreateGenericRule();
  rule.condition->url_filter = std::string("example.com");
  rule.priority = kMinValidPriority;
  rule.action->type = std::string("redirect");
  rule.action->redirect_url = std::string("http://google.com");
  std::unique_ptr<RulesetMatcher> matcher;
  ASSERT_NO_FATAL_FAILURE(
      CreateMatcherForRules({rule}, "test_extension", &matcher,
                            {"*://example.com/*", "*://abc.com/*"}));
  manager->AddRuleset(last_loaded_extension()->id(), std::move(matcher));

  // Create a request to "example.com" with an empty initiator. It should be
  // redirected to "google.com".
  const bool is_incognito_context = false;
  GURL redirect_url1;
  std::unique_ptr<net::URLRequest> request =
      GetRequestForURL("http://example.com");
  request->set_initiator(base::nullopt);
  extensions::WebRequestInfo request_info1(request.get());
  EXPECT_EQ(Action::REDIRECT,
            manager->EvaluateRequest(request_info1, is_incognito_context,
                                     &redirect_url1));
  EXPECT_EQ(GURL("http://google.com"), redirect_url1);

  // Change the initiator to "xyz.com". It should not be redirected since we
  // don't have host permissions to the request initiator.
  GURL redirect_url2;
  request->set_initiator(url::Origin::Create(GURL("http://xyz.com")));
  extensions::WebRequestInfo request_info2(request.get());
  EXPECT_EQ(Action::NONE,
            manager->EvaluateRequest(request_info2, is_incognito_context,
                                     &redirect_url2));

  // Change the initiator to "abc.com". It should be redirected since we have
  // the required host permissions.
  GURL redirect_url3;
  request->set_initiator(url::Origin::Create(GURL("http://abc.com")));
  extensions::WebRequestInfo request_info3(request.get());
  EXPECT_EQ(Action::REDIRECT,
            manager->EvaluateRequest(request_info3, is_incognito_context,
                                     &redirect_url3));
  EXPECT_EQ(GURL("http://google.com"), redirect_url3);

  // Ensure web-socket requests are not redirected.
  GURL redirect_url4;
  request = GetRequestForURL("ws://example.com");
  request->set_initiator(base::nullopt);
  extensions::WebRequestInfo request_info4(request.get());
  EXPECT_EQ(Action::NONE,
            manager->EvaluateRequest(request_info4, is_incognito_context,
                                     &redirect_url4));
}

// Tests that an extension can't block or redirect resources on the chrome-
// extension scheme.
TEST_P(RulesetManagerTest, ExtensionScheme) {
  RulesetManager* manager = info_map()->GetRulesetManager();
  ASSERT_TRUE(manager);

  const Extension* extension_1 = nullptr;
  const Extension* extension_2 = nullptr;
  // Add an extension with a background page which blocks all requests.
  {
    std::unique_ptr<RulesetMatcher> matcher;
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = std::string("*");
    ASSERT_NO_FATAL_FAILURE(CreateMatcherForRules(
        {rule}, "test extension", &matcher,
        std::vector<std::string>({URLPattern::kAllUrlsPattern}),
        true /* has_background_script*/));
    extension_1 = last_loaded_extension();
    manager->AddRuleset(extension_1->id(), std::move(matcher));
  }

  // Add another extension with a background page which redirects all requests
  // to "http://google.com".
  {
    std::unique_ptr<RulesetMatcher> matcher;
    TestRule rule = CreateGenericRule();
    rule.condition->url_filter = std::string("*");
    rule.priority = kMinValidPriority;
    rule.action->type = std::string("redirect");
    rule.action->redirect_url = std::string("http://google.com");
    ASSERT_NO_FATAL_FAILURE(CreateMatcherForRules(
        {rule}, "test extension_2", &matcher,
        std::vector<std::string>({URLPattern::kAllUrlsPattern}),
        true /* has_background_script*/));
    extension_2 = last_loaded_extension();
    manager->AddRuleset(extension_2->id(), std::move(matcher));
  }

  EXPECT_EQ(2u, manager->GetMatcherCountForTest());

  // Ensure that "http://example.com" will be blocked (with blocking taking
  // priority over redirection).
  std::unique_ptr<net::URLRequest> request =
      GetRequestForURL("http://example.com");
  GURL redirect_url;
  EXPECT_EQ(Action::BLOCK, manager->EvaluateRequest(
                               WebRequestInfo(request.get()),
                               false /*is_incognito_context*/, &redirect_url));

  // Ensure that the background page for |extension_1| won't be blocked or
  // redirected.
  GURL background_page_url_1 = BackgroundInfo::GetBackgroundURL(extension_1);
  EXPECT_TRUE(!background_page_url_1.is_empty());
  request = GetRequestForURL(background_page_url_1.spec());
  EXPECT_EQ(Action::NONE, manager->EvaluateRequest(
                              WebRequestInfo(request.get()),
                              false /*is_incognito_context*/, &redirect_url));

  // Ensure that the background page for |extension_2| won't be blocked or
  // redirected.
  GURL background_page_url_2 = BackgroundInfo::GetBackgroundURL(extension_2);
  EXPECT_TRUE(!background_page_url_2.is_empty());
  request = GetRequestForURL(background_page_url_2.spec());
  EXPECT_EQ(Action::NONE, manager->EvaluateRequest(
                              WebRequestInfo(request.get()),
                              false /*is_incognito_context*/, &redirect_url));

  // Also ensure that an arbitrary url on the chrome extension scheme is also
  // not blocked or redirected.
  request = GetRequestForURL(base::StringPrintf("%s://%s/%s", kExtensionScheme,
                                                "extension_id", "path"));
  EXPECT_EQ(Action::NONE, manager->EvaluateRequest(
                              WebRequestInfo(request.get()),
                              false /*is_incognito_context*/, &redirect_url));
}

INSTANTIATE_TEST_CASE_P(,
                        RulesetManagerTest,
                        ::testing::Values(ExtensionLoadType::PACKED,
                                          ExtensionLoadType::UNPACKED));

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
