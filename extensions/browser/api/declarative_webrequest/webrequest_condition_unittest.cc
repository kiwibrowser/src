// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_webrequest/webrequest_condition.h"

#include <memory>
#include <set>

#include "base/message_loop/message_loop.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "components/url_matcher/url_matcher_constants.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/previews_state.h"
#include "extensions/browser/api/declarative_webrequest/webrequest_constants.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "net/base/request_priority.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using url_matcher::URLMatcher;
using url_matcher::URLMatcherConditionSet;

namespace extensions {

TEST(WebRequestConditionTest, CreateCondition) {
  // Necessary for TestURLRequest.
  base::MessageLoopForIO message_loop;
  URLMatcher matcher;

  std::string error;
  std::unique_ptr<WebRequestCondition> result;

  // Test wrong condition name passed.
  error.clear();
  result = WebRequestCondition::Create(
      NULL,
      matcher.condition_factory(),
      *base::test::ParseJson(
           "{ \"invalid\": \"foobar\", \n"
           "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
           "}"),
      &error);
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(result.get());

  // Test wrong datatype in host_suffix.
  error.clear();
  result = WebRequestCondition::Create(
      NULL,
      matcher.condition_factory(),
      *base::test::ParseJson(
           "{ \n"
           "  \"url\": [], \n"
           "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
           "}"),
      &error);
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(result.get());

  // Test success (can we support multiple criteria?)
  error.clear();
  result = WebRequestCondition::Create(
      NULL,
      matcher.condition_factory(),
      *base::test::ParseJson(
           "{ \n"
           "  \"resourceType\": [\"main_frame\"], \n"
           "  \"url\": { \"hostSuffix\": \"example.com\" }, \n"
           "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
           "}"),
      &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(result.get());

  URLMatcherConditionSet::Vector url_matcher_condition_set;
  result->GetURLMatcherConditionSets(&url_matcher_condition_set);
  matcher.AddConditionSets(url_matcher_condition_set);

  net::TestURLRequestContext context;
  const GURL http_url("http://www.example.com");
  std::unique_ptr<net::URLRequest> match_request(context.CreateRequest(
      http_url, net::DEFAULT_PRIORITY, nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  content::ResourceRequestInfo::AllocateForTesting(
      match_request.get(), content::RESOURCE_TYPE_MAIN_FRAME,
      NULL,   // context
      -1,     // render_process_id
      -1,     // render_view_id
      -1,     // render_frame_id
      true,   // is_main_frame
      true,   // allow_download
      false,  // is_async
      content::PREVIEWS_OFF,
      nullptr);  // navigation_ui_data
  WebRequestInfo match_request_info(match_request.get());
  WebRequestData data(&match_request_info, ON_BEFORE_REQUEST);
  WebRequestDataWithMatchIds request_data(&data);
  request_data.url_match_ids = matcher.MatchURL(http_url);
  EXPECT_EQ(1u, request_data.url_match_ids.size());
  EXPECT_TRUE(result->IsFulfilled(request_data));

  const GURL https_url("https://www.example.com");
  std::unique_ptr<net::URLRequest> wrong_resource_type(context.CreateRequest(
      https_url, net::DEFAULT_PRIORITY, nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  content::ResourceRequestInfo::AllocateForTesting(
      wrong_resource_type.get(), content::RESOURCE_TYPE_SUB_FRAME,
      NULL,   // context
      -1,     // render_process_id
      -1,     // render_view_id
      -1,     // render_frame_id
      false,  // is_main_frame
      true,   // allow_download
      false,  // is_async
      content::PREVIEWS_OFF,
      nullptr);  // navigation_ui_data
  WebRequestInfo wrong_resource_type_request_info(wrong_resource_type.get());
  data.request = &wrong_resource_type_request_info;
  request_data.url_match_ids = matcher.MatchURL(http_url);
  // Make sure IsFulfilled does not fail because of URL matching.
  EXPECT_EQ(1u, request_data.url_match_ids.size());
  EXPECT_FALSE(result->IsFulfilled(request_data));
}

TEST(WebRequestConditionTest, CreateConditionFirstPartyForCookies) {
  // Necessary for TestURLRequest.
  base::MessageLoopForIO message_loop;
  URLMatcher matcher;

  std::string error;
  std::unique_ptr<WebRequestCondition> result;

  result = WebRequestCondition::Create(
      NULL,
      matcher.condition_factory(),
      *base::test::ParseJson(
           "{ \n"
           "  \"firstPartyForCookiesUrl\": { \"hostPrefix\": \"fpfc\"}, \n"
           "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
           "}"),
      &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(result.get());

  URLMatcherConditionSet::Vector url_matcher_condition_set;
  result->GetURLMatcherConditionSets(&url_matcher_condition_set);
  matcher.AddConditionSets(url_matcher_condition_set);

  net::TestURLRequestContext context;
  const GURL http_url("http://www.example.com");
  const GURL first_party_url("http://fpfc.example.com");
  std::unique_ptr<net::URLRequest> match_request(context.CreateRequest(
      http_url, net::DEFAULT_PRIORITY, nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  WebRequestInfo match_request_info(match_request.get());
  WebRequestData data(&match_request_info, ON_BEFORE_REQUEST);
  WebRequestDataWithMatchIds request_data(&data);
  request_data.url_match_ids = matcher.MatchURL(http_url);
  EXPECT_EQ(0u, request_data.url_match_ids.size());
  request_data.first_party_url_match_ids = matcher.MatchURL(first_party_url);
  EXPECT_EQ(1u, request_data.first_party_url_match_ids.size());
  content::ResourceRequestInfo::AllocateForTesting(
      match_request.get(), content::RESOURCE_TYPE_MAIN_FRAME,
      NULL,   // context
      -1,     // render_process_id
      -1,     // render_view_id
      -1,     // render_frame_id
      true,   // is_main_frame
      true,   // allow_download
      false,  // is_async
      content::PREVIEWS_OFF,
      nullptr);  // navigation_ui_data
  EXPECT_TRUE(result->IsFulfilled(request_data));
}

// Conditions without UrlFilter attributes need to be independent of URL
// matching results. We test here that:
//   1. A non-empty condition without UrlFilter attributes is fulfilled iff its
//      attributes are fulfilled.
//   2. An empty condition (in particular, without UrlFilter attributes) is
//      always fulfilled.
TEST(WebRequestConditionTest, NoUrlAttributes) {
  // Necessary for TestURLRequest.
  base::MessageLoopForIO message_loop;
  URLMatcher matcher;
  std::string error;

  // The empty condition.
  error.clear();
  std::unique_ptr<WebRequestCondition> condition_empty =
      WebRequestCondition::Create(
          NULL, matcher.condition_factory(),
          *base::test::ParseJson(
              "{ \n"
              "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
              "}"),
          &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(condition_empty.get());

  // A condition without a UrlFilter attribute, which is always true.
  error.clear();
  std::unique_ptr<WebRequestCondition> condition_no_url_true =
      WebRequestCondition::Create(
          NULL, matcher.condition_factory(),
          *base::test::ParseJson(
              "{ \n"
              "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", "
              "\n"
              // There is no "1st party for cookies" URL in the requests below,
              // therefore all requests are considered first party for cookies.
              "  \"thirdPartyForCookies\": false, \n"
              "}"),
          &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(condition_no_url_true.get());

  // A condition without a UrlFilter attribute, which is always false.
  error.clear();
  std::unique_ptr<WebRequestCondition> condition_no_url_false =
      WebRequestCondition::Create(
          NULL, matcher.condition_factory(),
          *base::test::ParseJson(
              "{ \n"
              "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", "
              "\n"
              "  \"thirdPartyForCookies\": true, \n"
              "}"),
          &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(condition_no_url_false.get());

  net::TestURLRequestContext context;
  std::unique_ptr<net::URLRequest> https_request(context.CreateRequest(
      GURL("https://www.example.com"), net::DEFAULT_PRIORITY, nullptr,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  WebRequestInfo https_request_info(https_request.get());

  // 1. A non-empty condition without UrlFilter attributes is fulfilled iff its
  //    attributes are fulfilled.
  WebRequestData data(&https_request_info, ON_BEFORE_REQUEST);
  EXPECT_FALSE(
      condition_no_url_false->IsFulfilled(WebRequestDataWithMatchIds(&data)));

  data = WebRequestData(&https_request_info, ON_BEFORE_REQUEST);
  EXPECT_TRUE(
      condition_no_url_true->IsFulfilled(WebRequestDataWithMatchIds(&data)));

  // 2. An empty condition (in particular, without UrlFilter attributes) is
  //    always fulfilled.
  data = WebRequestData(&https_request_info, ON_BEFORE_REQUEST);
  EXPECT_TRUE(condition_empty->IsFulfilled(WebRequestDataWithMatchIds(&data)));
}

TEST(WebRequestConditionTest, CreateConditionSet) {
  // Necessary for TestURLRequest.
  base::MessageLoopForIO message_loop;
  URLMatcher matcher;

  WebRequestConditionSet::Values conditions;
  conditions.push_back(base::test::ParseJson(
      "{ \n"
      "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
      "  \"url\": { \n"
      "    \"hostSuffix\": \"example.com\", \n"
      "    \"schemes\": [\"http\"], \n"
      "  }, \n"
      "}"));
  conditions.push_back(base::test::ParseJson(
      "{ \n"
      "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
      "  \"url\": { \n"
      "    \"hostSuffix\": \"example.com\", \n"
      "    \"hostPrefix\": \"www\", \n"
      "    \"schemes\": [\"https\"], \n"
      "  }, \n"
      "}"));

  // Test insertion
  std::string error;
  std::unique_ptr<WebRequestConditionSet> result =
      WebRequestConditionSet::Create(NULL, matcher.condition_factory(),
                                     conditions, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(result.get());
  EXPECT_EQ(2u, result->conditions().size());

  // Tell the URLMatcher about our shiny new patterns.
  URLMatcherConditionSet::Vector url_matcher_condition_set;
  result->GetURLMatcherConditionSets(&url_matcher_condition_set);
  matcher.AddConditionSets(url_matcher_condition_set);

  // Test that the result is correct and matches http://www.example.com and
  // https://www.example.com
  GURL http_url("http://www.example.com");
  net::TestURLRequestContext context;
  std::unique_ptr<net::URLRequest> http_request(context.CreateRequest(
      http_url, net::DEFAULT_PRIORITY, nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  WebRequestInfo http_request_info(http_request.get());
  WebRequestData data(&http_request_info, ON_BEFORE_REQUEST);
  WebRequestDataWithMatchIds request_data(&data);
  request_data.url_match_ids = matcher.MatchURL(http_url);
  EXPECT_EQ(1u, request_data.url_match_ids.size());
  EXPECT_TRUE(result->IsFulfilled(*(request_data.url_match_ids.begin()),
                                  request_data));

  GURL https_url("https://www.example.com");
  request_data.url_match_ids = matcher.MatchURL(https_url);
  EXPECT_EQ(1u, request_data.url_match_ids.size());
  std::unique_ptr<net::URLRequest> https_request(context.CreateRequest(
      https_url, net::DEFAULT_PRIORITY, nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  WebRequestInfo https_request_info(https_request.get());
  data.request = &https_request_info;
  EXPECT_TRUE(result->IsFulfilled(*(request_data.url_match_ids.begin()),
                                  request_data));

  // Check that both, hostPrefix and hostSuffix are evaluated.
  GURL https_foo_url("https://foo.example.com");
  request_data.url_match_ids = matcher.MatchURL(https_foo_url);
  EXPECT_EQ(0u, request_data.url_match_ids.size());
  std::unique_ptr<net::URLRequest> https_foo_request(
      context.CreateRequest(https_foo_url, net::DEFAULT_PRIORITY, nullptr,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  WebRequestInfo https_foo_request_info(https_foo_request.get());
  data.request = &https_foo_request_info;
  EXPECT_FALSE(result->IsFulfilled(-1, request_data));
}

TEST(WebRequestConditionTest, TestPortFilter) {
  // Necessary for TestURLRequest.
  base::MessageLoopForIO message_loop;
  URLMatcher matcher;

  WebRequestConditionSet::Values conditions;
  conditions.push_back(base::test::ParseJson(
      "{ \n"
      "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
      "  \"url\": { \n"
      "    \"ports\": [80, [1000, 1010]], \n"  // Allow 80;1000-1010.
      "    \"hostSuffix\": \"example.com\", \n"
      "  }, \n"
      "}"));

  // Test insertion
  std::string error;
  std::unique_ptr<WebRequestConditionSet> result =
      WebRequestConditionSet::Create(NULL, matcher.condition_factory(),
                                     conditions, &error);
  EXPECT_EQ("", error);
  ASSERT_TRUE(result.get());
  EXPECT_EQ(1u, result->conditions().size());

  // Tell the URLMatcher about our shiny new patterns.
  URLMatcherConditionSet::Vector url_matcher_condition_set;
  result->GetURLMatcherConditionSets(&url_matcher_condition_set);
  matcher.AddConditionSets(url_matcher_condition_set);

  std::set<URLMatcherConditionSet::ID> url_match_ids;

  // Test various URLs.
  GURL http_url("http://www.example.com");
  net::TestURLRequestContext context;
  std::unique_ptr<net::URLRequest> http_request(context.CreateRequest(
      http_url, net::DEFAULT_PRIORITY, nullptr, TRAFFIC_ANNOTATION_FOR_TESTS));
  url_match_ids = matcher.MatchURL(http_url);
  ASSERT_EQ(1u, url_match_ids.size());

  GURL http_url_80("http://www.example.com:80");
  std::unique_ptr<net::URLRequest> http_request_80(
      context.CreateRequest(http_url_80, net::DEFAULT_PRIORITY, nullptr,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  url_match_ids = matcher.MatchURL(http_url_80);
  ASSERT_EQ(1u, url_match_ids.size());

  GURL http_url_1000("http://www.example.com:1000");
  std::unique_ptr<net::URLRequest> http_request_1000(
      context.CreateRequest(http_url_1000, net::DEFAULT_PRIORITY, nullptr,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  url_match_ids = matcher.MatchURL(http_url_1000);
  ASSERT_EQ(1u, url_match_ids.size());

  GURL http_url_2000("http://www.example.com:2000");
  std::unique_ptr<net::URLRequest> http_request_2000(
      context.CreateRequest(http_url_2000, net::DEFAULT_PRIORITY, nullptr,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  url_match_ids = matcher.MatchURL(http_url_2000);
  ASSERT_EQ(0u, url_match_ids.size());
}

// Create a condition with two attributes: one on the request header and one on
// the response header. The Create() method should fail and complain that it is
// impossible that both conditions are fulfilled at the same time.
TEST(WebRequestConditionTest, ConditionsWithConflictingStages) {
  // Necessary for TestURLRequest.
  base::MessageLoopForIO message_loop;
  URLMatcher matcher;

  std::string error;
  std::unique_ptr<WebRequestCondition> result;

  // Test error on incompatible application stages for involved attributes.
  error.clear();
  result = WebRequestCondition::Create(
      NULL,
      matcher.condition_factory(),
      *base::test::ParseJson(
           "{ \n"
           "  \"instanceType\": \"declarativeWebRequest.RequestMatcher\", \n"
           // Pass a JS array with one empty object to each of the header
           // filters.
           "  \"requestHeaders\": [{}], \n"
           "  \"responseHeaders\": [{}], \n"
           "}"),
      &error);
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(result.get());
}

}  // namespace extensions
