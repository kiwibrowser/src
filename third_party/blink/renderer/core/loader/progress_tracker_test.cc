// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/progress_tracker.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/loader/empty_clients.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"

namespace blink {

class ProgressClient : public EmptyLocalFrameClient {
 public:
  ProgressClient() : last_progress_(0.0) {}

  void ProgressEstimateChanged(double progress_estimate) override {
    last_progress_ = progress_estimate;
  }

  double LastProgress() const { return last_progress_; }

 private:
  double last_progress_;
};

class ProgressTrackerTest : public PageTestBase {
 public:
  ProgressTrackerTest()
      : response_(KURL("http://example.com"), "text/html", 1024) {}

  void SetUp() override {
    client_ = new ProgressClient;
    PageTestBase::SetupPageWithClients(nullptr, client_.Get());
  }

  ProgressTracker& Progress() const { return GetFrame().Loader().Progress(); }
  double LastProgress() const { return client_->LastProgress(); }
  const ResourceResponse& ResponseHeaders() const { return response_; }

  // Reports a 1024-byte "main resource" (VeryHigh priority) request/response
  // to ProgressTracker with identifier 1, but tests are responsible for
  // emulating payload and load completion.
  void EmulateMainResourceRequestAndResponse() const {
    Progress().ProgressStarted(kFrameLoadTypeStandard);
    Progress().WillStartLoading(1ul, ResourceLoadPriority::kVeryHigh);
    EXPECT_EQ(0.0, LastProgress());
    Progress().IncrementProgress(1ul, ResponseHeaders());
    EXPECT_EQ(0.0, LastProgress());
  }

 private:
  Persistent<ProgressClient> client_;
  ResourceResponse response_;
};

TEST_F(ProgressTrackerTest, Static) {
  Progress().ProgressStarted(kFrameLoadTypeStandard);
  EXPECT_EQ(0.0, LastProgress());
  Progress().ProgressCompleted();
  EXPECT_EQ(1.0, LastProgress());
}

TEST_F(ProgressTrackerTest, MainResourceOnly) {
  EmulateMainResourceRequestAndResponse();

  // .2 for committing, .25 out of .5 possible for bytes received.
  Progress().IncrementProgress(1ul, 512);
  EXPECT_EQ(0.45, LastProgress());

  // .2 for committing, .5 for all bytes received.
  Progress().CompleteProgress(1ul);
  EXPECT_EQ(0.7, LastProgress());

  Progress().FinishedParsing();
  Progress().DidFirstContentfulPaint();
  EXPECT_EQ(1.0, LastProgress());
}

TEST_F(ProgressTrackerTest, WithHighPriorirySubresource) {
  EmulateMainResourceRequestAndResponse();

  Progress().WillStartLoading(2ul, ResourceLoadPriority::kHigh);
  Progress().IncrementProgress(2ul, ResponseHeaders());
  EXPECT_EQ(0.0, LastProgress());

  // .2 for committing, .25 out of .5 possible for bytes received.
  Progress().IncrementProgress(1ul, 1024);
  Progress().CompleteProgress(1ul);
  EXPECT_EQ(0.45, LastProgress());

  // .4 for finishing parsing/painting,
  // .25 out of .5 possible for bytes received.
  Progress().FinishedParsing();
  Progress().DidFirstContentfulPaint();
  EXPECT_EQ(0.65, LastProgress());

  Progress().CompleteProgress(2ul);
  EXPECT_EQ(1.0, LastProgress());
}

TEST_F(ProgressTrackerTest, WithMediumPrioritySubresource) {
  EmulateMainResourceRequestAndResponse();

  Progress().WillStartLoading(2ul, ResourceLoadPriority::kMedium);
  Progress().IncrementProgress(2ul, ResponseHeaders());
  EXPECT_EQ(0.0, LastProgress());

  // .2 for committing, .5 for all bytes received.
  // Medium priority resource is ignored.
  Progress().CompleteProgress(1ul);
  EXPECT_EQ(0.7, LastProgress());

  Progress().FinishedParsing();
  Progress().DidFirstContentfulPaint();
  EXPECT_EQ(1.0, LastProgress());
}

TEST_F(ProgressTrackerTest, FinishParsingBeforeContentfulPaint) {
  EmulateMainResourceRequestAndResponse();

  // .2 for committing, .5 for all bytes received.
  Progress().CompleteProgress(1ul);
  EXPECT_EQ(0.7, LastProgress());

  Progress().FinishedParsing();
  EXPECT_EQ(0.8, LastProgress());

  Progress().DidFirstContentfulPaint();
  EXPECT_EQ(1.0, LastProgress());
}

TEST_F(ProgressTrackerTest, ContentfulPaintBeforeFinishParsing) {
  EmulateMainResourceRequestAndResponse();

  // .2 for committing, .5 for all bytes received.
  Progress().CompleteProgress(1ul);
  EXPECT_EQ(0.7, LastProgress());

  Progress().DidFirstContentfulPaint();
  EXPECT_EQ(0.8, LastProgress());

  Progress().FinishedParsing();
  EXPECT_EQ(1.0, LastProgress());
}

}  // namespace blink
