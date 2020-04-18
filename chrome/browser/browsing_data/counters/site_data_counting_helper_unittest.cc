// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "chrome/browser/browsing_data/counters/site_data_counting_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::BrowserThread;

class SiteDataCountingHelperTest : public testing::Test {
 public:
  const int64_t kTimeoutMs = 10;

  void SetUp() override {
    profile_.reset(new TestingProfile());
    run_loop_.reset(new base::RunLoop());
    tasks_ = 0;
    cookie_callback_ = base::Bind(&SiteDataCountingHelperTest::CookieCallback,
                                  base::Unretained(this));
  }

  void TearDown() override {
    profile_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void CookieCallback(int count) {
    // Negative values represent an unexpected error.
    DCHECK(count >= 0);
    last_count_ = count;

    if (run_loop_)
      run_loop_->Quit();
  }

  void DoneOnIOThread(bool success) {
    DCHECK(success);
    if (--tasks_ > 0)
      return;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&SiteDataCountingHelperTest::DoneCallback,
                       base::Unretained(this)));
  }

  void DoneCallback() { run_loop_->Quit(); }

  void WaitForTasksOnIOThread() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
  }

  void CreateCookies(base::Time creation_time,
                     const std::vector<std::string>& urls) {
    content::StoragePartition* partition =
        content::BrowserContext::GetDefaultStoragePartition(profile());
    net::URLRequestContextGetter* rq_context =
        partition->GetURLRequestContext();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&SiteDataCountingHelperTest::CreateCookiesOnIOThread,
                       base::Unretained(this), base::WrapRefCounted(rq_context),
                       creation_time, urls));
  }

  void CreateLocalStorage(
      base::Time creation_time,
      const std::vector<base::FilePath::StringPieceType>& storage_origins) {
    // Note: This test depends on details of how the dom_storage library
    // stores data in the host file system.
    base::FilePath storage_path =
        profile()->GetPath().AppendASCII("Local Storage");
    base::CreateDirectory(storage_path);

    // Write some files.
    for (const auto& origin : storage_origins) {
      base::WriteFile(storage_path.Append(origin), NULL, 0);
      base::TouchFile(storage_path.Append(origin), creation_time,
                      creation_time);
    }
  }

  void CreateCookiesOnIOThread(
      const scoped_refptr<net::URLRequestContextGetter>& rq_context,
      base::Time creation_time,
      std::vector<std::string> urls) {
    net::CookieStore* cookie_store =
        rq_context->GetURLRequestContext()->cookie_store();

    tasks_ = urls.size();
    int i = 0;
    for (const std::string& url_string : urls) {
      GURL url(url_string);
      // Cookies need a unique creation time.
      base::Time time = creation_time + base::TimeDelta::FromMilliseconds(i++);

      cookie_store->SetCanonicalCookieAsync(
          net::CanonicalCookie::CreateSanitizedCookie(
              url, "name", "A=1", url.host(), url.path(), time, base::Time(),
              time, url.SchemeIsCryptographic(), false,
              net::CookieSameSite::DEFAULT_MODE, net::COOKIE_PRIORITY_DEFAULT),
          url.SchemeIsCryptographic(), true /*modify_http_only*/,
          base::BindOnce(&SiteDataCountingHelperTest::DoneOnIOThread,
                         base::Unretained(this)));
    }
  }

  void CountEntries(base::Time begin_time) {
    last_count_ = -1;
    auto* helper =
        new SiteDataCountingHelper(profile(), begin_time, cookie_callback_);
    helper->CountAndDestroySelfWhenFinished();
  }

  int64_t GetResult() {
    DCHECK_GE(last_count_, 0);
    return last_count_;
  }

  Profile* profile() { return profile_.get(); }

 private:
  base::Callback<void(int)> cookie_callback_;
  std::unique_ptr<base::RunLoop> run_loop_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;

  int tasks_;
  int64_t last_count_;
};

TEST_F(SiteDataCountingHelperTest, CheckEmptyResult) {
  CountEntries(base::Time());
  WaitForTasksOnIOThread();

  DCHECK_EQ(0, GetResult());
}

TEST_F(SiteDataCountingHelperTest, CountCookies) {
  base::Time now = base::Time::Now();
  base::Time last_hour = now - base::TimeDelta::FromHours(1);
  base::Time yesterday = now - base::TimeDelta::FromDays(1);

  CreateCookies(last_hour, {"https://example.com"});
  WaitForTasksOnIOThread();

  CreateCookies(yesterday, {"https://google.com", "https://bing.com"});
  WaitForTasksOnIOThread();

  CountEntries(base::Time());
  WaitForTasksOnIOThread();
  DCHECK_EQ(3, GetResult());

  CountEntries(yesterday);
  WaitForTasksOnIOThread();
  DCHECK_EQ(3, GetResult());

  CountEntries(last_hour);
  WaitForTasksOnIOThread();
  DCHECK_EQ(1, GetResult());

  CountEntries(now);
  WaitForTasksOnIOThread();
  DCHECK_EQ(0, GetResult());
}

TEST_F(SiteDataCountingHelperTest, LocalStorage) {
  base::Time now = base::Time::Now();
  CreateLocalStorage(now,
                     {FILE_PATH_LITERAL("https_example.com_443.localstorage"),
                      FILE_PATH_LITERAL("https_bing.com_443.localstorage")});

  CountEntries(base::Time());
  WaitForTasksOnIOThread();
  DCHECK_EQ(2, GetResult());
}

TEST_F(SiteDataCountingHelperTest, CookiesAndLocalStorage) {
  base::Time now = base::Time::Now();
  CreateCookies(now, {"http://example.com", "https://google.com"});
  CreateLocalStorage(now,
                     {FILE_PATH_LITERAL("https_example.com_443.localstorage"),
                      FILE_PATH_LITERAL("https_bing.com_443.localstorage")});
  WaitForTasksOnIOThread();

  CountEntries(base::Time());
  WaitForTasksOnIOThread();
  DCHECK_EQ(3, GetResult());
}

TEST_F(SiteDataCountingHelperTest, SameHostDifferentScheme) {
  base::Time now = base::Time::Now();
  CreateCookies(now, {"http://google.com", "https://google.com"});
  CreateLocalStorage(now,
                     {FILE_PATH_LITERAL("https_google.com_443.localstorage"),
                      FILE_PATH_LITERAL("http_google.com_80.localstorage")});
  WaitForTasksOnIOThread();
  CountEntries(base::Time());
  WaitForTasksOnIOThread();
  DCHECK_EQ(1, GetResult());
}
