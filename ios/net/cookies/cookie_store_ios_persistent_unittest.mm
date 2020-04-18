// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/net/cookies/cookie_store_ios_persistent.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/bind_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#import "ios/net/cookies/cookie_store_ios_test_util.h"
#include "net/cookies/canonical_cookie_test_helpers.h"
#include "net/cookies/cookie_store_change_unittest.h"
#include "net/cookies/cookie_store_test_callbacks.h"
#include "net/cookies/cookie_store_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

struct PersistentCookieStoreIOSTestTraits {
  static std::unique_ptr<net::CookieStore> Create() {
    return std::make_unique<CookieStoreIOSPersistent>(nullptr);
  }

  static void DeliverChangeNotifications() { base::RunLoop().RunUntilIdle(); }

  static const bool is_cookie_monster = false;
  static const bool supports_http_only = false;
  static const bool supports_non_dotted_domains = true;
  static const bool preserves_trailing_dots = true;
  static const bool filters_schemes = false;
  static const bool has_path_prefix_bug = false;
  static const bool forbids_setting_empty_name = false;
  static const bool supports_global_cookie_tracking = false;
  // TODO(crbug.com/813931): Fix the bugs uncovered by these tests.
  static const bool supports_named_cookie_tracking = false;
  static const bool supports_url_cookie_tracking = false;
  static const bool supports_multiple_tracking_callbacks = false;
  static const bool has_exact_change_cause = false;
  static const bool has_exact_change_ordering = false;
  static const int creation_time_granularity_in_ms = 0;
  static const int enforces_prefixes = true;
  static const bool enforce_strict_secure = false;

  base::MessageLoop loop_;
};

INSTANTIATE_TYPED_TEST_CASE_P(PersistentCookieStoreIOS,
                              CookieStoreTest,
                              PersistentCookieStoreIOSTestTraits);
INSTANTIATE_TYPED_TEST_CASE_P(PersistentCookieStoreIOS,
                              CookieStoreChangeGlobalTest,
                              PersistentCookieStoreIOSTestTraits);
INSTANTIATE_TYPED_TEST_CASE_P(PersistentCookieStoreIOS,
                              CookieStoreChangeUrlTest,
                              PersistentCookieStoreIOSTestTraits);
INSTANTIATE_TYPED_TEST_CASE_P(PersistentCookieStoreIOS,
                              CookieStoreChangeNamedTest,
                              PersistentCookieStoreIOSTestTraits);

namespace {

// Test fixture to exercise net::CookieStoreIOSPersistent created with
// TestPersistentCookieStore back-end and not synchronized with
// SystemCookieStore.
class CookieStoreIOSPersistentTest : public PlatformTest {
 public:
  CookieStoreIOSPersistentTest()
      : kTestCookieURL("http://foo.google.com/bar"),
        scoped_cookie_store_ios_client_(
            std::make_unique<TestCookieStoreIOSClient>()),
        backend_(new net::TestPersistentCookieStore),
        store_(
            std::make_unique<net::CookieStoreIOSPersistent>(backend_.get())) {
    cookie_change_subscription_ =
        store_->GetChangeDispatcher().AddCallbackForCookie(
            kTestCookieURL, "abc",
            base::BindRepeating(&net::RecordCookieChanges, &cookies_changed_,
                                &cookies_removed_));
  }

  ~CookieStoreIOSPersistentTest() override {}

  // Gets the cookies. |callback| will be called on completion.
  void GetCookies(net::CookieStore::GetCookieListCallback callback) {
    net::CookieOptions options;
    options.set_include_httponly();
    store_->GetCookieListWithOptionsAsync(kTestCookieURL, options,
                                          std::move(callback));
  }

  // Sets a cookie.
  void SetCookie(const std::string& cookie_line) {
    net::SetCookie(cookie_line, kTestCookieURL, store_.get());
  }

 private:
  const GURL kTestCookieURL;

 protected:
  base::MessageLoop loop_;
  ScopedTestingCookieStoreIOSClient scoped_cookie_store_ios_client_;
  scoped_refptr<net::TestPersistentCookieStore> backend_;
  std::unique_ptr<net::CookieStoreIOS> store_;
  std::unique_ptr<net::CookieChangeSubscription> cookie_change_subscription_;
  std::vector<net::CanonicalCookie> cookies_changed_;
  std::vector<bool> cookies_removed_;
};

}  // namespace

TEST_F(CookieStoreIOSPersistentTest, SetCookieCallsHook) {
  ClearCookies();
  SetCookie("abc=def");
  EXPECT_EQ(0U, cookies_changed_.size());
  EXPECT_EQ(0U, cookies_removed_.size());
  backend_->RunLoadedCallback();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1U, cookies_changed_.size());
  EXPECT_EQ(1U, cookies_removed_.size());
  EXPECT_EQ("abc", cookies_changed_[0].Name());
  EXPECT_EQ("def", cookies_changed_[0].Value());
  EXPECT_FALSE(cookies_removed_[0]);

  // Replacing an existing cookie is actually a two-phase delete + set
  // operation, so we get an extra notification.
  SetCookie("abc=ghi");
  EXPECT_EQ(3U, cookies_changed_.size());
  EXPECT_EQ(3U, cookies_removed_.size());
  EXPECT_EQ("abc", cookies_changed_[1].Name());
  EXPECT_EQ("def", cookies_changed_[1].Value());
  EXPECT_TRUE(cookies_removed_[1]);
  EXPECT_EQ("abc", cookies_changed_[2].Name());
  EXPECT_EQ("ghi", cookies_changed_[2].Value());
  EXPECT_FALSE(cookies_removed_[2]);
}

// Tests that cookies can be read before the backend is loaded.
TEST_F(CookieStoreIOSPersistentTest, NotSynchronized) {
  // Start fetching the cookie.
  GetCookieListCallback callback;
  GetCookies(
      base::BindOnce(&GetCookieListCallback::Run, base::Unretained(&callback)));
  // Backend loading completes.
  backend_->RunLoadedCallback();
  EXPECT_THAT(callback.cookies(), MatchesCookieLine("a=b"));
}

}  // namespace net
