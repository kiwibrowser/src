// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_metrics_reporter.h"
#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_task_environment.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_ukm_entry.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ukm::TestUkmRecorder;
using ukm::builders::ContextualSuggestions;

namespace contextual_suggestions {

namespace {
const char kEventsHistogramName[] = "ContextualSuggestions.Events";
const char kTestNavigationUrl[] = "https://foo.com";
const int kUninitialized = 0;
const int kFetchDelayed = 1;
const int kFetchRequested = 2;
const int kFetchError = 3;
const int kFetchServerBusy = 4;
const int kFetchBelowThreshold = 5;
const int kFetchEmpty = 6;
const int kFetchCompleted = 7;
const int kUiPeekReverseScroll = 8;
const int kUiOpened = 9;
const int kUiClosedObsolete = 10;
const int kSuggestionDownloaded = 11;
const int kSuggestionClicked = 12;
const int kUiDismissedWithoutOpen = 13;
const int kUiDismissedAfterOpen = 14;
}  // namespace

class ContextualSuggestionsMetricsReporterTest : public ::testing::Test {
 protected:
  ContextualSuggestionsMetricsReporterTest() = default;

  TestUkmRecorder* GetTestUkmRecorder() { return &test_ukm_recorder_; }

  ukm::SourceId GetSourceId();

  ContextualSuggestionsMetricsReporter& GetReporter() { return reporter_; }

 protected:
  void ExpectMultipleEventsCountOnce(ContextualSuggestionsEvent event);

 private:
  // The reporter under test.
  ContextualSuggestionsMetricsReporter reporter_;

  // Sets up the task scheduling/task-runner environment for each test.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Sets itself as the global UkmRecorder on construction.
  ukm::TestAutoSetUkmRecorder test_ukm_recorder_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsMetricsReporterTest);
};

ukm::SourceId ContextualSuggestionsMetricsReporterTest::GetSourceId() {
  ukm::SourceId source_id = ukm::UkmRecorder::GetNewSourceID();
  test_ukm_recorder_.UpdateSourceURL(source_id, GURL(kTestNavigationUrl));
  return source_id;
}

TEST_F(ContextualSuggestionsMetricsReporterTest, BaseTest) {
  base::HistogramTester histogram_tester;
  GetReporter().SetupForPage(kTestNavigationUrl, GetSourceId());
  GetReporter().RecordEvent(FETCH_REQUESTED);
  GetReporter().RecordEvent(FETCH_COMPLETED);
  GetReporter().RecordEvent(UI_PEEK_REVERSE_SCROLL);
  GetReporter().RecordEvent(UI_OPENED);
  GetReporter().RecordEvent(SUGGESTION_DOWNLOADED);
  GetReporter().RecordEvent(SUGGESTION_CLICKED);
  // Flush data to write to UKM.
  GetReporter().Flush();
  // Check that we wrote something to UKM.  Details of UKM reporting are tested
  // in a different test suite.
  TestUkmRecorder* test_ukm_recorder = GetTestUkmRecorder();
  std::vector<const ukm::mojom::UkmEntry*> entry_vector =
      test_ukm_recorder->GetEntriesByName(ContextualSuggestions::kEntryName);
  EXPECT_EQ(1U, entry_vector.size());
  const ukm::mojom::UkmEntry* first_entry = entry_vector[0];
  EXPECT_TRUE(test_ukm_recorder->EntryHasMetric(
      first_entry, ContextualSuggestions::kFetchStateName));
  EXPECT_EQ(static_cast<int64_t>(FetchState::COMPLETED),
            *(test_ukm_recorder->GetEntryMetric(
                first_entry, ContextualSuggestions::kFetchStateName)));
  // Test that the expected histogram events were written.
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kUninitialized, 0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kFetchDelayed, 0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kFetchRequested, 1);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kFetchError, 0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kFetchServerBusy, 0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kFetchBelowThreshold,
                                     0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kFetchEmpty, 0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kFetchCompleted, 1);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kUiPeekReverseScroll,
                                     1);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kUiOpened, 1);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kUiClosedObsolete,
                                     0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName,
                                     kSuggestionDownloaded, 1);
  histogram_tester.ExpectBucketCount(kEventsHistogramName, kSuggestionClicked,
                                     1);
  histogram_tester.ExpectBucketCount(kEventsHistogramName,
                                     kUiDismissedWithoutOpen, 0);
  histogram_tester.ExpectBucketCount(kEventsHistogramName,
                                     kUiDismissedAfterOpen, 0);
}

void ContextualSuggestionsMetricsReporterTest::ExpectMultipleEventsCountOnce(
    ContextualSuggestionsEvent event) {
  std::unique_ptr<base::HistogramTester> histogram_tester =
      std::make_unique<base::HistogramTester>();
  GetReporter().SetupForPage(kTestNavigationUrl, GetSourceId());
  // Always report a single FETCH_DELAYED event so we ensure there's a
  // histogram (otherwise the ExpectBucketCount may crash).
  GetReporter().RecordEvent(FETCH_DELAYED);
  int event_index = static_cast<int>(event);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, event_index, 0);
  // Peek the UI, which starts the timer, expected by all the other UI or
  // suggestion events.
  GetReporter().RecordEvent(UI_PEEK_REVERSE_SCROLL);
  // Report the event that we want to test multiple times.
  GetReporter().RecordEvent(event);
  GetReporter().RecordEvent(event);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, event_index, 1);
  GetReporter().Flush();
}

TEST_F(ContextualSuggestionsMetricsReporterTest, UiPeekReverseScrollTest) {
  ExpectMultipleEventsCountOnce(UI_PEEK_REVERSE_SCROLL);
}

TEST_F(ContextualSuggestionsMetricsReporterTest, UiOpenedTest) {
  ExpectMultipleEventsCountOnce(UI_OPENED);
}

TEST_F(ContextualSuggestionsMetricsReporterTest, UiDismissedWithoutOpenTest) {
  ExpectMultipleEventsCountOnce(UI_DISMISSED_WITHOUT_OPEN);
}

TEST_F(ContextualSuggestionsMetricsReporterTest, UiDismissedAfterOpenTest) {
  ExpectMultipleEventsCountOnce(UI_DISMISSED_AFTER_OPEN);
}

TEST_F(ContextualSuggestionsMetricsReporterTest, SuggestionDownloadedTest) {
  ExpectMultipleEventsCountOnce(SUGGESTION_DOWNLOADED);
}

TEST_F(ContextualSuggestionsMetricsReporterTest, SuggestionClickedTest) {
  ExpectMultipleEventsCountOnce(SUGGESTION_CLICKED);
}

TEST_F(ContextualSuggestionsMetricsReporterTest, MultipleEventsTest) {
  std::unique_ptr<base::HistogramTester> histogram_tester =
      std::make_unique<base::HistogramTester>();
  GetReporter().SetupForPage(kTestNavigationUrl, GetSourceId());
  // Test multiple cycles of FETCH_REQUESTED, FETCH_COMPLETED.
  GetReporter().RecordEvent(FETCH_REQUESTED);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchRequested, 1);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchCompleted, 0);
  GetReporter().RecordEvent(FETCH_COMPLETED);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchRequested, 1);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchCompleted, 1);
  GetReporter().RecordEvent(FETCH_REQUESTED);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchRequested, 2);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchCompleted, 1);
  GetReporter().RecordEvent(FETCH_COMPLETED);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchRequested, 2);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kFetchCompleted, 2);
  GetReporter().Flush();
}

// Test that might catch enum reordering that's not done right.
TEST_F(ContextualSuggestionsMetricsReporterTest, EnumNotReorderedTest) {
  std::unique_ptr<base::HistogramTester> histogram_tester =
      std::make_unique<base::HistogramTester>();
  GetReporter().SetupForPage(kTestNavigationUrl, GetSourceId());
  // Peek the UI, which starts the timer, expected by all the other UI or
  // suggestion events.
  GetReporter().RecordEvent(UI_PEEK_REVERSE_SCROLL);
  // The current last legal event.
  GetReporter().RecordEvent(SUGGESTION_CLICKED);
  histogram_tester->ExpectBucketCount(kEventsHistogramName, kSuggestionClicked,
                                      1);
  GetReporter().Flush();
}

}  // namespace contextual_suggestions
