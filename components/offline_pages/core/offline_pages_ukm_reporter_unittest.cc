// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_pages_ukm_reporter.h"
#include "base/test/scoped_task_environment.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kVisitedUrl[] = "http://m.en.wikipedia.org/wiki/Glider_(sailplane)";
}  // namespace

namespace offline_pages {

class OfflinePagesUkmReporterTest : public testing::Test {
 public:
  ukm::TestUkmRecorder* test_recorder() { return &test_recorder_; }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ukm::TestAutoSetUkmRecorder test_recorder_;
};

TEST_F(OfflinePagesUkmReporterTest, RecordOfflinePageVisit) {
  OfflinePagesUkmReporter reporter;
  GURL gurl(kVisitedUrl);

  reporter.ReportUrlOfflineRequest(gurl, false);

  const auto& entries = test_recorder()->GetEntriesByName(
      ukm::builders::OfflinePages_SavePageRequested::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    // TODO(petewil): re-enable once crbug/792197 is addressed.
    // test_recorder()->ExpectEntrySourceHasUrl(entry, gurl);
    test_recorder()->ExpectEntryMetric(
        entry,
        ukm::builders::OfflinePages_SavePageRequested::
            kRequestedFromForegroundName,
        0);
  }
}

}  // namespace offline_pages
