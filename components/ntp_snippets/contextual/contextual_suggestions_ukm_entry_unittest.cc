// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_ukm_entry.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_metrics_reporter.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gtest/include/gtest/gtest.h"

using ukm::TestUkmRecorder;
using ukm::builders::ContextualSuggestions;

namespace contextual_suggestions {

const int NO_ENTRY = -1;

class ContextualSuggestionsUkmEntryTest : public ::testing::Test {
 protected:
  ContextualSuggestionsUkmEntryTest() = default;

  void SetUp() override;

  // The entry under test.
  std::unique_ptr<ContextualSuggestionsUkmEntry> ukm_entry_;

  TestUkmRecorder* GetTestUkmRecorder() { return &test_ukm_recorder_; }

  const ukm::mojom::UkmEntry* FirstEntry();
  bool HasEntryMetric(const char* metric_name);
  int GetEntryMetric(const char* metric_name);

 private:
  // Sets up the task scheduling/task-runner environment for each test.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Sets itself as the global UkmRecorder on construction.
  ukm::TestAutoSetUkmRecorder test_ukm_recorder_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsUkmEntryTest);
};

void ContextualSuggestionsUkmEntryTest::SetUp() {
  ukm_entry_.reset(
      new ContextualSuggestionsUkmEntry(ukm::UkmRecorder::GetNewSourceID()));
}

const ukm::mojom::UkmEntry* ContextualSuggestionsUkmEntryTest::FirstEntry() {
  TestUkmRecorder* recorder = GetTestUkmRecorder();
  std::vector<const ukm::mojom::UkmEntry*> entry_vector =
      recorder->GetEntriesByName(ContextualSuggestions::kEntryName);
  EXPECT_EQ(1U, entry_vector.size());
  return entry_vector[0];
}

bool ContextualSuggestionsUkmEntryTest::HasEntryMetric(
    const char* metric_name) {
  TestUkmRecorder* recorder = GetTestUkmRecorder();
  return recorder->EntryHasMetric(FirstEntry(), metric_name);
}

int ContextualSuggestionsUkmEntryTest::GetEntryMetric(const char* metric_name) {
  TestUkmRecorder* recorder = GetTestUkmRecorder();
  const ukm::mojom::UkmEntry* first_entry = FirstEntry();
  if (!recorder->EntryHasMetric(first_entry, metric_name))
    return NO_ENTRY;

  return (int)*(recorder->GetEntryMetric(first_entry, metric_name));
}

TEST_F(ContextualSuggestionsUkmEntryTest, BaseTest) {
  ukm_entry_->Flush();
  // Deleting the entry should write default values for everything.
  EXPECT_EQ(0, GetEntryMetric(ContextualSuggestions::kAnyDownloadedName));
  EXPECT_EQ(0, GetEntryMetric(ContextualSuggestions::kAnySuggestionTakenName));
  EXPECT_EQ(0, GetEntryMetric(ContextualSuggestions::kClosedFromPeekName));
  EXPECT_EQ(0, GetEntryMetric(ContextualSuggestions::kEverOpenedName));
  EXPECT_EQ(0, GetEntryMetric(ContextualSuggestions::kFetchStateName));
  EXPECT_EQ(0,
            GetEntryMetric(ContextualSuggestions::kShowDurationBucketMinName));
  EXPECT_EQ(0, GetEntryMetric(ContextualSuggestions::kTriggerEventName));
}

TEST_F(ContextualSuggestionsUkmEntryTest, ExpectedOperationTest) {
  ukm_entry_->RecordEventMetrics(FETCH_DELAYED);
  ukm_entry_->RecordEventMetrics(FETCH_REQUESTED);
  ukm_entry_->RecordEventMetrics(FETCH_COMPLETED);
  ukm_entry_->RecordEventMetrics(UI_PEEK_REVERSE_SCROLL);
  ukm_entry_->RecordEventMetrics(UI_OPENED);
  ukm_entry_->RecordEventMetrics(SUGGESTION_DOWNLOADED);
  ukm_entry_->RecordEventMetrics(SUGGESTION_DOWNLOADED);
  ukm_entry_->RecordEventMetrics(SUGGESTION_CLICKED);
  ukm_entry_->Flush();
  EXPECT_EQ(1, GetEntryMetric(ContextualSuggestions::kAnyDownloadedName));
  EXPECT_EQ(1, GetEntryMetric(ContextualSuggestions::kAnySuggestionTakenName));
  EXPECT_EQ(0, GetEntryMetric(ContextualSuggestions::kClosedFromPeekName));
  EXPECT_EQ(1, GetEntryMetric(ContextualSuggestions::kEverOpenedName));
  EXPECT_EQ(static_cast<int64_t>(FetchState::COMPLETED),
            GetEntryMetric(ContextualSuggestions::kFetchStateName));
  EXPECT_LT(0,
            GetEntryMetric(ContextualSuggestions::kShowDurationBucketMinName));
  EXPECT_EQ(1, GetEntryMetric(ContextualSuggestions::kTriggerEventName));
}

}  // namespace contextual_suggestions
