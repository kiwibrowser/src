// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/feedback_uploader.h"

#include <string>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "components/feedback/feedback_uploader_factory.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_http_header_provider.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feedback {

namespace {

constexpr base::TimeDelta kTestRetryDelay =
    base::TimeDelta::FromMilliseconds(1);

constexpr int kHttpPostSuccessNoContent = 204;
constexpr int kHttpPostFailClientError = 400;
constexpr int kHttpPostFailServerError = 500;

}  // namespace

class FeedbackUploaderDispatchTest : public ::testing::Test {
 protected:
  FeedbackUploaderDispatchTest()
      : browser_thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {}

  ~FeedbackUploaderDispatchTest() override {
    // Clean up registered ids.
    variations::testing::ClearAllVariationIDs();
  }

  // Registers a field trial with the specified name and group and an associated
  // google web property variation id.
  void CreateFieldTrialWithId(const std::string& trial_name,
                              const std::string& group_name,
                              int variation_id) {
    variations::AssociateGoogleVariationID(
        variations::GOOGLE_WEB_PROPERTIES, trial_name, group_name,
        static_cast<variations::VariationID>(variation_id));
    base::FieldTrialList::CreateFieldTrial(trial_name, group_name)->group();
  }

  content::BrowserContext* context() { return &context_; }

 private:
  content::TestBrowserThreadBundle browser_thread_bundle_;
  content::TestBrowserContext context_;

  DISALLOW_COPY_AND_ASSIGN(FeedbackUploaderDispatchTest);
};

TEST_F(FeedbackUploaderDispatchTest, VariationHeaders) {
  // Register a trial and variation id, so that there is data in variations
  // headers. Also, the variations header provider may have been registered to
  // observe some other field trial list, so reset it.
  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
  base::FieldTrialList field_trial_list_(nullptr);
  CreateFieldTrialWithId("Test", "Group1", 123);

  FeedbackUploader uploader(
      context(), FeedbackUploaderFactory::CreateUploaderTaskRunner());

  net::TestURLFetcherFactory factory;
  uploader.QueueReport("test");

  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string value;
  EXPECT_TRUE(headers.GetHeader("X-Client-Data", &value));
  EXPECT_FALSE(value.empty());
  // The fetcher's delegate is responsible for freeing the fetcher (and itself).
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  variations::VariationsHttpHeaderProvider::GetInstance()->ResetForTesting();
}

TEST_F(FeedbackUploaderDispatchTest, TestVariousServerResponses) {
  FeedbackUploader::SetMinimumRetryDelayForTesting(kTestRetryDelay);
  FeedbackUploader uploader(
      context(), FeedbackUploaderFactory::CreateUploaderTaskRunner());

  EXPECT_EQ(kTestRetryDelay, uploader.retry_delay());
  // Successful reports should not introduce any retries, and should not
  // increase the backoff delay.
  net::TestURLFetcherFactory factory;
  factory.set_remove_fetcher_on_delete(true);
  uploader.QueueReport("Successful report");
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(0);
  fetcher->set_response_code(kHttpPostSuccessNoContent);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(kTestRetryDelay, uploader.retry_delay());
  EXPECT_TRUE(uploader.QueueEmpty());

  // Failed reports due to client errors are not retried. No backoff delay
  // should be doubled.
  uploader.QueueReport("Client error failed report");
  fetcher = factory.GetFetcherByID(0);
  fetcher->set_response_code(kHttpPostFailClientError);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(kTestRetryDelay, uploader.retry_delay());
  EXPECT_TRUE(uploader.QueueEmpty());

  // Failed reports due to server errors are retried.
  uploader.QueueReport("Server error failed report");
  fetcher = factory.GetFetcherByID(0);
  fetcher->set_response_code(kHttpPostFailServerError);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(kTestRetryDelay * 2, uploader.retry_delay());
  EXPECT_FALSE(uploader.QueueEmpty());
}

}  // namespace feedback
