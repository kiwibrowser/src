// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/webui/url_fetcher_block_adapter.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

// Test fixture for URLFetcherBlockAdapter.
class URLFetcherBlockAdapterTest : public PlatformTest {
 protected:
  URLFetcherBlockAdapterTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  // Required for base::MessageLoopCurrent::Get().
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

// Tests that URLFetcherBlockAdapter calls its completion handler with the
// appropriate data for a text resource.
TEST_F(URLFetcherBlockAdapterTest, FetchTextResource) {
  GURL test_url("http://test");
  std::string response("<html><body>Hello World!</body></html>");
  NSData* expected_data =
      [NSData dataWithBytes:response.c_str() length:response.size()];
  web::URLFetcherBlockAdapterCompletion completion_handler =
      ^(NSData* data, web::URLFetcherBlockAdapter* fetcher) {
        EXPECT_NSEQ(expected_data, data);
      };
  web::URLFetcherBlockAdapter web_ui_fetcher(test_url, nil, completion_handler);
  net::FakeURLFetcher fake_fetcher(test_url, &web_ui_fetcher, response,
                                   net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);
  fake_fetcher.Start();
  base::RunLoop().RunUntilIdle();
}

// Tests that URLFetcherBlockAdapter calls its completion handler with the
// appropriate data for a png resource.
TEST_F(URLFetcherBlockAdapterTest, FetchPNGResource) {
  GURL test_url("http://test");
  base::FilePath favicon_path;
  ASSERT_TRUE(base::PathService::Get(base::DIR_SOURCE_ROOT, &favicon_path));
  favicon_path = favicon_path.AppendASCII("ios/web/test/data/testfavicon.png");
  NSData* expected_data = [NSData
      dataWithContentsOfFile:base::SysUTF8ToNSString(favicon_path.value())];
  web::URLFetcherBlockAdapterCompletion completion_handler =
      ^(NSData* data, URLFetcherBlockAdapter* fetcher) {
        EXPECT_NSEQ(expected_data, data);
      };
  web::URLFetcherBlockAdapter web_ui_fetcher(test_url, nil, completion_handler);
  std::string response;
  EXPECT_TRUE(ReadFileToString(favicon_path, &response));
  net::FakeURLFetcher fake_fetcher(test_url, &web_ui_fetcher, response,
                                   net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);
  fake_fetcher.Start();
  base::RunLoop().RunUntilIdle();
}

}  // namespace web
