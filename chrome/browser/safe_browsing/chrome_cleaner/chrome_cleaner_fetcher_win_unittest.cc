// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_fetcher_win.h"

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_field_trial_win.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/net_errors.h"
#include "net/base/network_change_notifier.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace safe_browsing {
namespace {

class ChromeCleanerFetcherTest : public ::testing::Test,
                                 public net::TestURLFetcher::DelegateForTests {
 public:
  void SetUp() override {
    FetchChromeCleaner(base::Bind(&ChromeCleanerFetcherTest::FetchedCallback,
                                  base::Unretained(this)));

    // Get the TestURLFetcher instance used by FetchChromeCleaner.
    fetcher_ = factory_.GetFetcherByID(0);
    fetcher_->SetDelegateForTests(this);

    ASSERT_TRUE(fetcher_);

    // Continue only when URLFetcher's Start() method has been called.
    run_loop_.Run();
  }

  void TearDown() override {
    if (!response_path_.empty()) {
      base::DeleteFile(response_path_, /*recursive=*/false);
      if (base::IsDirectoryEmpty(response_path_.DirName()))
        base::DeleteFile(response_path_.DirName(), /*recursive=*/false);
    }
  }

  void FetchedCallback(base::FilePath downloaded_path,
                       ChromeCleanerFetchStatus fetch_status) {
    callback_called_ = true;
    downloaded_path_ = downloaded_path;
    fetch_status_ = fetch_status;
  }

  // net::TestURLFetcher::DelegateForTests overrides.

  void OnRequestStart(int fetcher_id) override {
    // Save the file path where the response will be saved for later tests.
    EXPECT_TRUE(fetcher_->GetResponseAsFilePath(/*take_ownership=*/false,
                                                &response_path_));

    run_loop_.QuitWhenIdle();
  }

  void OnChunkUpload(int fetcher_id) override {}
  void OnRequestEnd(int fetcher_id) override {}

  void SetNetworkSuccess(int response_code) {
    fetcher_->set_status(net::URLRequestStatus{});
    fetcher_->set_response_code(response_code);
    fetcher_->delegate()->OnURLFetchComplete(fetcher_);
  }

 protected:
  // TestURLFetcher requires a MessageLoop and an IO thread to release
  // URLRequestContextGetter in URLFetcher::Core.
  content::TestBrowserThreadBundle thread_bundle_;

  // TestURLFetcherFactory automatically sets itself as URLFetcher's factory
  net::TestURLFetcherFactory factory_;

  // The TestURLFetcher instance used by the FetchChromeCleaner.
  net::TestURLFetcher* fetcher_ = nullptr;

  base::RunLoop run_loop_;

  // File path where TestULRFetcher will save a response as intercepted by
  // OnRequestStart(). Used to clean up during teardown.
  base::FilePath response_path_;

  // Variables set by FetchedCallback().
  bool callback_called_ = false;
  base::FilePath downloaded_path_;
  ChromeCleanerFetchStatus fetch_status_ =
      ChromeCleanerFetchStatus::kOtherFailure;

  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_ =
      base::WrapUnique(net::NetworkChangeNotifier::CreateMock());
};

TEST_F(ChromeCleanerFetcherTest, FetchSuccess) {
  EXPECT_EQ(GURL(fetcher_->GetOriginalURL()), GetSRTDownloadURL());

  SetNetworkSuccess(net::HTTP_OK);

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(downloaded_path_, response_path_);
  EXPECT_EQ(fetch_status_, ChromeCleanerFetchStatus::kSuccess);
}

TEST_F(ChromeCleanerFetcherTest, NotFoundOnServer) {
  // Set up the fetcher to return a HTTP_NOT_FOUND failure. Notice that the
  // net error in this case is OK, since there was no error preventing any
  // response (even 404) from being received.
  SetNetworkSuccess(net::HTTP_NOT_FOUND);

  EXPECT_TRUE(callback_called_);
  EXPECT_TRUE(downloaded_path_.empty());
  EXPECT_EQ(fetch_status_, ChromeCleanerFetchStatus::kNotFoundOnServer);
}

TEST_F(ChromeCleanerFetcherTest, NetworkError) {
  // Set up the fetcher to return failure other than HTTP_NOT_FOUND.
  fetcher_->set_status(net::URLRequestStatus::FromError(net::ERR_FAILED));
  // For this test, just use any http response code other than net::HTTP_OK
  // and net::HTTP_NOT_FOUND.
  fetcher_->set_response_code(net::HTTP_INTERNAL_SERVER_ERROR);
  fetcher_->delegate()->OnURLFetchComplete(fetcher_);

  EXPECT_TRUE(callback_called_);
  EXPECT_TRUE(downloaded_path_.empty());
  EXPECT_EQ(fetch_status_, ChromeCleanerFetchStatus::kOtherFailure);
}

}  // namespace
}  // namespace safe_browsing
