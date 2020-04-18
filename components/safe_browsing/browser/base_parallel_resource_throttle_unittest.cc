// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/browser/base_parallel_resource_throttle.h"

#include "base/memory/ref_counted.h"
#include "components/safe_browsing/browser/url_checker_delegate.h"
#include "components/safe_browsing/db/test_database_manager.h"
#include "components/security_interstitials/content/unsafe_resource.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

class TestResourceThrottleDelegate
    : public content::ResourceThrottle::Delegate {
 public:
  int cancel_called() const { return cancel_called_; }
  int resume_called() const { return resume_called_; }

  void Cancel() override { cancel_called_++; }
  void CancelWithError(int error_code) override { cancel_called_++; }
  void Resume() override { resume_called_++; }

 private:
  int cancel_called_ = 0;
  int resume_called_ = 0;
};

class TestDatabaseManager : public TestSafeBrowsingDatabaseManager {
 public:
  ThreatSource GetThreatSource() const override {
    return ThreatSource::UNKNOWN;
  }

  bool IsSupported() const override { return true; }

  bool CanCheckResourceType(
      content::ResourceType resource_type) const override {
    return true;
  }

  bool ChecksAreAlwaysAsync() const override { return false; }

  bool CheckBrowseUrl(const GURL& url,
                      const SBThreatTypeSet& threat_types,
                      Client* client) override {
    DCHECK(!last_client_);

    check_browse_url_called_++;

    // Immediately return that the url is safe.
    if (delay_check_browse_url_calls_ == 0)
      return true;

    delay_check_browse_url_calls_--;
    last_client_ = client;
    last_url_ = url;
    return false;
  }

  void CancelCheck(Client* client) override {}

  int check_browse_url_called() const { return check_browse_url_called_; }

  void DelayCheckBrowseUrlResult(int num_calls) {
    delay_check_browse_url_calls_ += num_calls;
  }

  void CompleteNextCheckBrowseUrl(bool safe) {
    DCHECK(last_client_);

    Client* temp = last_client_;
    last_client_ = nullptr;
    temp->OnCheckBrowseUrlResult(
        last_url_, safe ? SB_THREAT_TYPE_SAFE : SB_THREAT_TYPE_URL_MALWARE,
        ThreatMetadata());
  }

 private:
  ~TestDatabaseManager() override {}

  int check_browse_url_called_ = 0;
  Client* last_client_ = nullptr;
  GURL last_url_;
  int delay_check_browse_url_calls_ = 0;
};

class TestUrlCheckerDelegate : public UrlCheckerDelegate {
 public:
  explicit TestUrlCheckerDelegate(
      scoped_refptr<TestDatabaseManager> database_manager)
      : threat_types_(CreateSBThreatTypeSet(
            {safe_browsing::SB_THREAT_TYPE_URL_MALWARE,
             safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
             safe_browsing::SB_THREAT_TYPE_URL_UNWANTED})),
        database_manager_(std::move(database_manager)) {}

  void MaybeDestroyPrerenderContents(
      const base::Callback<content::WebContents*()>& web_contents_getter)
      override {}

  void StartDisplayingBlockingPageHelper(
      const security_interstitials::UnsafeResource& resource,
      const std::string& method,
      const net::HttpRequestHeaders& headers,
      bool is_main_frame,
      bool has_user_gesture) override {
    resource.callback.Run(false);
  }

  bool IsUrlWhitelisted(const GURL& url) override { return false; }

  bool ShouldSkipRequestCheck(content::ResourceContext* resource_context,
                              const GURL& original_url,
                              int frame_tree_node_id,
                              int render_process_id,
                              int render_frame_id,
                              bool originated_from_service_worker) override {
    return false;
  }

  void NotifySuspiciousSiteDetected(
      const base::RepeatingCallback<content::WebContents*()>&
          web_contents_getter) override {}
  const SBThreatTypeSet& GetThreatTypes() override { return threat_types_; }
  SafeBrowsingDatabaseManager* GetDatabaseManager() override {
    return database_manager_.get();
  }
  BaseUIManager* GetUIManager() override { return nullptr; }

 private:
  ~TestUrlCheckerDelegate() override {}

  SBThreatTypeSet threat_types_;
  scoped_refptr<TestDatabaseManager> database_manager_;
};

class TestParallelResourceThrottle : public BaseParallelResourceThrottle {
 public:
  TestParallelResourceThrottle(
      const net::URLRequest* request,
      content::ResourceType resource_type,
      scoped_refptr<UrlCheckerDelegate> url_checker_delegate)
      : BaseParallelResourceThrottle(request,
                                     resource_type,
                                     std::move(url_checker_delegate)) {}

  // BaseParallelResourceThrottle overrides to expose them as public methods.
  void WillStartRequest(bool* defer) override {
    BaseParallelResourceThrottle::WillStartRequest(defer);
  }

  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override {
    BaseParallelResourceThrottle::WillRedirectRequest(redirect_info, defer);
  }

  void WillProcessResponse(bool* defer) override {
    BaseParallelResourceThrottle::WillProcessResponse(defer);
  }
};

class BaseParallelResourceThrottleTest : public testing::Test {
 protected:
  BaseParallelResourceThrottleTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    request_ = request_context_.CreateRequest(GURL("http://example.org"),
                                              net::MEDIUM, &request_delegate_,
                                              TRAFFIC_ANNOTATION_FOR_TESTS);
    content::ResourceRequestInfo::AllocateForTesting(
        request_.get(), content::RESOURCE_TYPE_MAIN_FRAME, nullptr, -1, -1, -1,
        true, true, true, content::PREVIEWS_OFF, nullptr);

    database_manager_ = new TestDatabaseManager();
    url_checker_delegate_ = new TestUrlCheckerDelegate(database_manager_);
    throttle_ = std::make_unique<TestParallelResourceThrottle>(
        request_.get(), content::RESOURCE_TYPE_MAIN_FRAME,
        url_checker_delegate_);
    throttle_->set_delegate_for_testing(&resource_throttle_delegate_);
  }

  content::TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext request_context_;
  net::TestDelegate request_delegate_;
  std::unique_ptr<net::URLRequest> request_;
  scoped_refptr<TestDatabaseManager> database_manager_;
  scoped_refptr<UrlCheckerDelegate> url_checker_delegate_;
  TestResourceThrottleDelegate resource_throttle_delegate_;
  std::unique_ptr<TestParallelResourceThrottle> throttle_;
};

TEST_F(BaseParallelResourceThrottleTest, Resume) {
  database_manager_->DelayCheckBrowseUrlResult(2);
  bool defer = false;
  throttle_->WillStartRequest(&defer);

  // Although there is a pending URL check, starting request is not deferred.
  EXPECT_FALSE(defer);
  EXPECT_EQ(1, database_manager_->check_browse_url_called());

  // Following redirects is also not deferred by pending URL checks.
  throttle_->WillRedirectRequest(net::RedirectInfo(), &defer);
  EXPECT_FALSE(defer);
  // The throttle doesn't initiate a new URL check until the previous one is
  // completed.
  EXPECT_EQ(1, database_manager_->check_browse_url_called());

  throttle_->WillProcessResponse(&defer);
  EXPECT_TRUE(defer);

  EXPECT_EQ(0, resource_throttle_delegate_.resume_called());

  database_manager_->CompleteNextCheckBrowseUrl(true);
  EXPECT_EQ(2, database_manager_->check_browse_url_called());
  database_manager_->CompleteNextCheckBrowseUrl(true);

  EXPECT_EQ(1, resource_throttle_delegate_.resume_called());
}

TEST_F(BaseParallelResourceThrottleTest, CancelWhenDeferred) {
  database_manager_->DelayCheckBrowseUrlResult(1);
  bool defer = false;
  throttle_->WillStartRequest(&defer);

  // Although there is a pending URL check, starting request is not deferred.
  EXPECT_FALSE(defer);
  EXPECT_EQ(1, database_manager_->check_browse_url_called());

  throttle_->WillProcessResponse(&defer);
  EXPECT_TRUE(defer);

  EXPECT_EQ(0, resource_throttle_delegate_.cancel_called());

  database_manager_->CompleteNextCheckBrowseUrl(false);

  EXPECT_EQ(1, resource_throttle_delegate_.cancel_called());
}

TEST_F(BaseParallelResourceThrottleTest, CancelBeforeWillRedirectRequest) {
  database_manager_->DelayCheckBrowseUrlResult(1);
  bool defer = false;
  throttle_->WillStartRequest(&defer);

  EXPECT_FALSE(defer);
  EXPECT_EQ(1, database_manager_->check_browse_url_called());
  database_manager_->CompleteNextCheckBrowseUrl(false);

  // Cancellation is delayed to the next throttle notification event (in order
  // to avoid confusing ResourceLoader).
  EXPECT_EQ(0, resource_throttle_delegate_.cancel_called());

  throttle_->WillRedirectRequest(net::RedirectInfo(), &defer);

  EXPECT_EQ(1, resource_throttle_delegate_.cancel_called());
}

TEST_F(BaseParallelResourceThrottleTest, CancelBeforeWillProcessResponse) {
  database_manager_->DelayCheckBrowseUrlResult(1);
  bool defer = false;
  throttle_->WillStartRequest(&defer);

  EXPECT_FALSE(defer);
  EXPECT_EQ(1, database_manager_->check_browse_url_called());
  database_manager_->CompleteNextCheckBrowseUrl(false);

  // Cancellation is delayed to the next throttle notification event (in order
  // to avoid confusing ResourceLoader).
  EXPECT_EQ(0, resource_throttle_delegate_.cancel_called());

  throttle_->WillProcessResponse(&defer);

  EXPECT_EQ(1, resource_throttle_delegate_.cancel_called());
}

}  // namespace safe_browsing
