// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ios_chrome_io_thread.h"

#include "base/run_loop.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/testing_pref_store.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_filter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// A delegate interface for users of URLFetcher.
class TestURLFetcherDelegate : public net::URLFetcherDelegate {
 public:
  TestURLFetcherDelegate() {}
  ~TestURLFetcherDelegate() override {}

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override {}
};

}  // namespace

class IOSChromeIOThreadTest : public PlatformTest {
 public:
  IOSChromeIOThreadTest()
      : thread_bundle_(web::TestWebThreadBundle::IO_MAINLOOP) {
    net::URLRequestFailedJob::AddUrlHandler();
  }

  ~IOSChromeIOThreadTest() override {
    net::URLRequestFilter::GetInstance()->ClearHandlers();
  }

 private:
  web::TestWebThreadBundle thread_bundle_;
};

TEST_F(IOSChromeIOThreadTest, AssertNoUrlRequests) {
  PrefServiceFactory pref_service_factory;
  pref_service_factory.set_user_prefs(base::MakeRefCounted<TestingPrefStore>());

  scoped_refptr<PrefRegistrySimple> pref_registry = new PrefRegistrySimple;
  PrefProxyConfigTrackerImpl::RegisterPrefs(pref_registry.get());

  std::unique_ptr<PrefService> pref_service(
      pref_service_factory.Create(pref_registry.get()));

  // Create and init IOSChromeIOThread.
  std::unique_ptr<IOSChromeIOThread> ios_chrome_io_thread(
      new IOSChromeIOThread(pref_service.get(), nullptr));
  web::WebThreadDelegate* web_thread_delegate =
      static_cast<web::WebThreadDelegate*>(ios_chrome_io_thread.get());
  web_thread_delegate->Init();

  // Create and start fetcher.
  TestURLFetcherDelegate fetcher_delegate;
  std::unique_ptr<net::URLFetcher> fetcher = net::URLFetcher::Create(
      net::URLRequestFailedJob::GetMockHttpUrl(net::ERR_IO_PENDING),
      net::URLFetcher::GET, &fetcher_delegate);
  fetcher->SetRequestContext(
      ios_chrome_io_thread->system_url_request_context_getter());
  fetcher->Start();
  base::RunLoop().RunUntilIdle();
  // Verify that there is no AssertNoUrlRequests triggered during CleanUp.
  web_thread_delegate->CleanUp();
}
