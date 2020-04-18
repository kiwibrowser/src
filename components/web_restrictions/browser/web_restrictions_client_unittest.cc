// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "components/web_restrictions/browser/mock_web_restrictions_client.h"
#include "components/web_restrictions/browser/web_restrictions_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

using web_restrictions::WebRestrictionsClient;
using web_restrictions::WebRestrictionsClientResult;
using web_restrictions::MockWebRestrictionsClient;

namespace {

bool g_returned_result;

void ResultCallback(const base::Closure& quit_closure, bool result) {
  g_returned_result = result;
  quit_closure.Run();
}

}  // namespace

namespace web_restrictions {

class WebRestrictionsClientTest : public testing::Test {
 protected:
  void SetAuthority(std::string authority) {
    client_.SetAuthorityTask(authority);
  }
  // Mock the Java WebRestrictionsClient. The real version
  // would need a content provider to do anything.
  MockWebRestrictionsClient mock_;
  content::TestBrowserThreadBundle thread_bundle_;
  WebRestrictionsClient client_;
};

TEST_F(WebRestrictionsClientTest, ShouldProceed) {
  SetAuthority("Good");
  // First call should go to Web Restrictions Content Provider, and return a
  // delayed result.
  {
    g_returned_result = false;
    base::RunLoop run_loop;
    ASSERT_EQ(web_restrictions::PENDING,
              client_.ShouldProceed(
                  true, "http://example.com",
                  base::Bind(&ResultCallback, run_loop.QuitClosure())));
    run_loop.Run();
    EXPECT_TRUE(g_returned_result);
  }
  // A repeated call should go to the cache and return a result immediately.
  {
    base::RunLoop run_loop;
    ASSERT_EQ(web_restrictions::ALLOW,
              client_.ShouldProceed(
                  true, "http://example.com",
                  base::Bind(&ResultCallback, run_loop.QuitClosure())));
  }
  // However a different url should miss the cache
  {
    g_returned_result = false;
    base::RunLoop run_loop;
    ASSERT_EQ(web_restrictions::PENDING,
              client_.ShouldProceed(
                  true, "http://example.com/2",
                  base::Bind(&ResultCallback, run_loop.QuitClosure())));
    run_loop.Run();
    EXPECT_TRUE(g_returned_result);
  }
  // Switching the authority should clear the cache.
  {
    SetAuthority("Good2");
    g_returned_result = false;
    base::RunLoop run_loop;
    ASSERT_EQ(web_restrictions::PENDING,
              client_.ShouldProceed(
                  true, "http://example.com/2",
                  base::Bind(&ResultCallback, run_loop.QuitClosure())));
    run_loop.Run();
    EXPECT_TRUE(g_returned_result);
  }
  // Try getting a bad result
  {
    SetAuthority("Bad");
    g_returned_result = true;
    base::RunLoop run_loop;
    ASSERT_EQ(web_restrictions::PENDING,
              client_.ShouldProceed(
                  true, "http://example.com/2",
                  base::Bind(&ResultCallback, run_loop.QuitClosure())));
    run_loop.Run();
    EXPECT_FALSE(g_returned_result);
    std::unique_ptr<const WebRestrictionsClientResult> result =
        client_.GetCachedWebRestrictionsResult("http://example.com/2");
    ASSERT_NE(nullptr, result.get());
    EXPECT_EQ(42, result->GetInt(1));
    EXPECT_EQ("http://example.com/2", result->GetString(2));
  }
}

TEST_F(WebRestrictionsClientTest, RequestPermission) {
  {
    SetAuthority("Good");
    base::RunLoop run_loop;
    g_returned_result = false;
    client_.RequestPermission(
        "http://example.com",
        base::Bind(&ResultCallback, run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_TRUE(g_returned_result);
  }
  {
    SetAuthority("Bad");
    base::RunLoop run_loop;
    g_returned_result = true;
    client_.RequestPermission(
        "http://example.com",
        base::Bind(&ResultCallback, run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_FALSE(g_returned_result);
  }
}

}  // namespace web_restrictions
