// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_composite_reporter.h"

#include "components/ntp_snippets/contextual/contextual_suggestions_reporter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace contextual_suggestions {

static const std::string kTestUrl = "https://foo.com";
static const ukm::SourceId kSourceId = 1;

class TestReporter : public ContextualSuggestionsReporter {
 public:
  TestReporter() = default;
  ~TestReporter() override { reporter_destroy_count_++; }

  /* ContextualSuggestionsReporter */

  void SetupForPage(const std::string& url, ukm::SourceId source_id) override {
    this->url_ = url;
    this->source_id_ = source_id;
    called_setup_for_page_count_++;
  }

  void RecordEvent(ContextualSuggestionsEvent event) override {
    called_record_event_count_++;
  }

  void Flush() override { called_flush_count_++; }

  /* Accessors */
  std::string GetUrl() { return url_; }
  ukm::SourceId GetSourceId() { return source_id_; }
  int GetNSetupForPage() { return called_setup_for_page_count_; }
  int GetNRecordEvent() { return called_record_event_count_; }
  int GetNFlush() { return called_flush_count_; };

  /* Static variables*/
  static int GetReporterDestroyCount() { return reporter_destroy_count_; }
  static void ResetReporterDestroyCount() { reporter_destroy_count_ = 0; }

 private:
  static int reporter_destroy_count_;

  std::string url_;
  ukm::SourceId source_id_;
  int called_setup_for_page_count_ = 0;
  int called_record_event_count_ = 0;
  int called_flush_count_ = 0;
};

int TestReporter::reporter_destroy_count_ = 0;

TEST(ContextualSuggestionsCompositeReporterTest, AddAndReportUnique) {
  std::unique_ptr<ContextualSuggestionsCompositeReporter> composite_reporter =
      std::make_unique<ContextualSuggestionsCompositeReporter>();

  std::vector<TestReporter*> reporters;
  for (int i = 0; i < 10; i++) {
    auto reporter = std::make_unique<TestReporter>();
    reporters.push_back(reporter.get());
    composite_reporter->AddOwnedReporter(std::move(reporter));
  }

  composite_reporter->SetupForPage(kTestUrl, kSourceId);
  composite_reporter->RecordEvent(ContextualSuggestionsEvent::FETCH_REQUESTED);
  composite_reporter->Flush();

  for (TestReporter* reporter : reporters) {
    EXPECT_EQ(1, reporter->GetNSetupForPage());
    EXPECT_EQ(1, reporter->GetNRecordEvent());
    EXPECT_EQ(1, reporter->GetNFlush());
  }

  EXPECT_EQ(kTestUrl, reporters.front()->GetUrl());
  EXPECT_EQ(kSourceId, reporters.front()->GetSourceId());
}

TEST(ContextualSuggestionsCompositeReporterTest, AddAndReportRaw) {
  std::unique_ptr<ContextualSuggestionsCompositeReporter> composite_reporter =
      std::make_unique<ContextualSuggestionsCompositeReporter>();

  std::vector<std::unique_ptr<TestReporter>> reporters;
  for (int i = 0; i < 10; i++) {
    auto reporter = std::make_unique<TestReporter>();
    composite_reporter->AddRawReporter(reporter.get());
    reporters.push_back(std::move(reporter));
  }

  composite_reporter->SetupForPage(kTestUrl, kSourceId);
  composite_reporter->RecordEvent(ContextualSuggestionsEvent::FETCH_REQUESTED);
  composite_reporter->Flush();

  for (auto& reporter : reporters) {
    EXPECT_EQ(1, reporter->GetNSetupForPage());
    EXPECT_EQ(1, reporter->GetNRecordEvent());
    EXPECT_EQ(1, reporter->GetNFlush());
  }

  EXPECT_EQ(kTestUrl, reporters.front()->GetUrl());
  EXPECT_EQ(kSourceId, reporters.front()->GetSourceId());
}

TEST(ContextualSuggestionsCompositeReporterTest, CheckOwnedDeleted) {
  TestReporter::ResetReporterDestroyCount();
  std::unique_ptr<ContextualSuggestionsCompositeReporter> composite_reporter =
      std::make_unique<ContextualSuggestionsCompositeReporter>();

  auto reporter = std::make_unique<TestReporter>();
  composite_reporter->AddOwnedReporter(std::make_unique<TestReporter>());
  composite_reporter->AddOwnedReporter(std::make_unique<TestReporter>());
  composite_reporter->AddRawReporter(reporter.get());
  composite_reporter.reset(nullptr);

  EXPECT_EQ(2, TestReporter::GetReporterDestroyCount());
}

}  // namespace contextual_suggestions