// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/net/retryable_url_fetcher.h"

#include "base/message_loop/message_loop.h"
#import "base/strings/sys_string_conversions.h"
#include "ios/web/public/test/test_web_thread.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_test_util.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// An arbitrary text string for a fake response.
NSString* const kFakeResponseString = @"Something interesting here.";
}

// Delegate object to provide data for RetryableURLFetcher and
// handles the callback when URL is fetched.
@interface TestRetryableURLFetcherDelegate
    : NSObject<RetryableURLFetcherDelegate>
// Counts the number of times that a successful response has been processed.
@property(nonatomic, assign) NSUInteger responsesProcessed;
@end

@implementation TestRetryableURLFetcherDelegate
@synthesize responsesProcessed;

- (NSString*)urlToFetch {
  return @"http://www.google.com";
}

- (void)processSuccessResponse:(NSString*)response {
  if (response) {
    EXPECT_NSEQ(kFakeResponseString, response);
    ++responsesProcessed;
  }
}

@end

@interface TestFailingURLFetcherDelegate : NSObject<RetryableURLFetcherDelegate>
@property(nonatomic, assign) BOOL responsesProcessed;
@end

@implementation TestFailingURLFetcherDelegate
@synthesize responsesProcessed;

- (NSString*)urlToFetch {
  return nil;
}

- (void)processSuccessResponse:(NSString*)response {
  EXPECT_FALSE(response);
  responsesProcessed = YES;
}

@end

namespace {

class RetryableURLFetcherTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    test_delegate_ = [[TestRetryableURLFetcherDelegate alloc] init];
    io_thread_.reset(
        new web::TestWebThread(web::WebThread::IO, &message_loop_));
  }

  net::TestURLFetcherFactory factory_;
  std::unique_ptr<web::TestWebThread> io_thread_;
  base::MessageLoop message_loop_;
  TestRetryableURLFetcherDelegate* test_delegate_;
};

TEST_F(RetryableURLFetcherTest, TestResponse200) {
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop_.task_runner());
  RetryableURLFetcher* retryableFetcher = [[RetryableURLFetcher alloc]
      initWithRequestContextGetter:request_context_getter.get()
                          delegate:test_delegate_
                     backoffPolicy:nil];
  [retryableFetcher startFetch];

  // Manually calls the delegate.
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  DCHECK(fetcher);
  DCHECK(fetcher->delegate());
  [test_delegate_ setResponsesProcessed:0U];
  fetcher->set_response_code(200);
  fetcher->SetResponseString(base::SysNSStringToUTF8(kFakeResponseString));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1U, [test_delegate_ responsesProcessed]);
}

TEST_F(RetryableURLFetcherTest, TestResponse404) {
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop_.task_runner());
  RetryableURLFetcher* retryableFetcher = [[RetryableURLFetcher alloc]
      initWithRequestContextGetter:request_context_getter.get()
                          delegate:test_delegate_
                     backoffPolicy:nil];
  [retryableFetcher startFetch];

  // Manually calls the delegate.
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  DCHECK(fetcher);
  DCHECK(fetcher->delegate());
  [test_delegate_ setResponsesProcessed:0U];
  fetcher->set_response_code(404);
  fetcher->SetResponseString("");
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(0U, [test_delegate_ responsesProcessed]);
}

// Tests that response callback method is called if delegate returns an
// invalid URL.
TEST_F(RetryableURLFetcherTest, TestFailingURLNoRetry) {
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop_.task_runner());
  TestFailingURLFetcherDelegate* failing_delegate =
      [[TestFailingURLFetcherDelegate alloc] init];
  RetryableURLFetcher* retryable_fetcher = [[RetryableURLFetcher alloc]
      initWithRequestContextGetter:request_context_getter.get()
                          delegate:failing_delegate
                     backoffPolicy:nil];
  [retryable_fetcher startFetch];

  // |failing_delegate| does not have URL to fetch, so a fetcher should never
  // be created.
  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  EXPECT_FALSE(fetcher);

  // Verify that response has been called.
  EXPECT_TRUE([failing_delegate responsesProcessed]);
}

}  // namespace
