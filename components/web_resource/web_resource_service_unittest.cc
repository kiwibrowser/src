// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/web_resource/resource_request_allowed_notifier.h"
#include "components/web_resource/web_resource_service.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const std::string kTestUrl = "http://www.test.com";
const std::string kCacheUpdatePath = "cache_update_path";
std::string error_message_;
}  // namespace

namespace web_resource {

class TestResourceRequestAllowedNotifier
    : public ResourceRequestAllowedNotifier {
 public:
  TestResourceRequestAllowedNotifier(PrefService* prefs,
                                     const char* disable_network_switch)
      : ResourceRequestAllowedNotifier(prefs, disable_network_switch) {}

  ResourceRequestAllowedNotifier::State GetResourceRequestsAllowedState()
      override {
    return state_;
  }

  void SetState(ResourceRequestAllowedNotifier::State state) { state_ = state; }

  void NotifyState(ResourceRequestAllowedNotifier::State state) {
    SetState(state);
    SetObserverRequestedForTesting(true);
    MaybeNotifyObserver();
  }

 private:
  ResourceRequestAllowedNotifier::State state_;
};

class TestWebResourceService : public WebResourceService {
 public:
  TestWebResourceService(PrefService* prefs,
                         const GURL& web_resource_server,
                         const std::string& application_locale,
                         const char* last_update_time_pref_name,
                         int start_fetch_delay_ms,
                         int cache_update_delay_ms,
                         net::URLRequestContextGetter* request_context,
                         const char* disable_network_switch,
                         const ParseJSONCallback& parse_json_callback)
      : WebResourceService(prefs,
                           web_resource_server,
                           application_locale,
                           last_update_time_pref_name,
                           start_fetch_delay_ms,
                           cache_update_delay_ms,
                           request_context,
                           disable_network_switch,
                           parse_json_callback,
                           TRAFFIC_ANNOTATION_FOR_TESTS){};

  void Unpack(const base::DictionaryValue& parsed_json) override {}
};

class WebResourceServiceTest : public testing::Test {
 public:
  WebResourceServiceTest() {}

  void SetUp() override {
    request_context_getter_ = new net::TestURLRequestContextGetter(
        base::ThreadTaskRunnerHandle::Get());
    local_state_.reset(new TestingPrefServiceSimple());
    local_state_->registry()->RegisterStringPref(kCacheUpdatePath, "0");
    test_web_resource_service_.reset(new TestWebResourceService(
        local_state_.get(), GURL(kTestUrl), "", kCacheUpdatePath.c_str(), 100,
        5000, request_context_getter_.get(), nullptr,
        base::Bind(web_resource::WebResourceServiceTest::Parse)));
    error_message_ = "";
    TestResourceRequestAllowedNotifier* notifier =
        new TestResourceRequestAllowedNotifier(local_state_.get(), nullptr);
    notifier->SetState(ResourceRequestAllowedNotifier::ALLOWED);
    test_web_resource_service_->SetResourceRequestAllowedNotifier(
        std::unique_ptr<ResourceRequestAllowedNotifier>(notifier));
  }

  TestResourceRequestAllowedNotifier* resource_notifier() {
    return static_cast<TestResourceRequestAllowedNotifier*>(
        test_web_resource_service_->resource_request_allowed_notifier_.get());
  }

  bool GetFetchScheduled() {
    return test_web_resource_service_->GetFetchScheduled();
  }

  void CallScheduleFetch(int64_t delay_ms) {
    return test_web_resource_service_->ScheduleFetch(delay_ms);
  }

  static void Parse(const std::string& unsafe_json,
                    const WebResourceService::SuccessCallback& success_callback,
                    const WebResourceService::ErrorCallback& error_callback) {
    std::unique_ptr<base::Value> value;
    if (!error_message_.empty())
      error_callback.Run(error_message_);
    else
      success_callback.Run(std::move(value));
  }

  WebResourceService* web_resource_service() {
    return test_web_resource_service_.get();
  }

  void CallStartFetch() { test_web_resource_service_->StartFetch(); }

 private:
  base::MessageLoop message_loop_;  // needed for TestURLFetcherFactory
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  std::unique_ptr<TestingPrefServiceSimple> local_state_;
  std::unique_ptr<TestWebResourceService> test_web_resource_service_;
};

TEST_F(WebResourceServiceTest, FetchScheduledAfterStartDelayTest) {
  web_resource_service()->StartAfterDelay();
  EXPECT_TRUE(GetFetchScheduled());
}

TEST_F(WebResourceServiceTest, FetchScheduledOnScheduleFetchTest) {
  web_resource_service()->StartAfterDelay();
  resource_notifier()->NotifyState(ResourceRequestAllowedNotifier::ALLOWED);
  EXPECT_TRUE(GetFetchScheduled());
}

TEST_F(WebResourceServiceTest, FetchScheduledOnStartFetchTest) {
  resource_notifier()->NotifyState(
      ResourceRequestAllowedNotifier::DISALLOWED_NETWORK_DOWN);
  CallStartFetch();
  EXPECT_FALSE(GetFetchScheduled());
  resource_notifier()->NotifyState(ResourceRequestAllowedNotifier::ALLOWED);
  EXPECT_TRUE(GetFetchScheduled());
}

}  // namespace web_resource
