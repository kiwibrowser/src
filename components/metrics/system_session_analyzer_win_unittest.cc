// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/system_session_analyzer_win.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {

namespace {

const uint16_t kIdSessionStart = 6005U;
const uint16_t kIdSessionEnd = 6006U;
const uint16_t kIdSessionEndUnclean = 6008U;

}  // namespace

// Ensure the fetcher retrieves events.
TEST(SystemSessionAnalyzerTest, FetchEvents) {
  SystemSessionAnalyzer analyzer(0);
  std::vector<SystemSessionAnalyzer::EventInfo> events;
  ASSERT_TRUE(analyzer.FetchEvents(1U, &events));
  EXPECT_EQ(1U, events.size());
}

// Ensure the fetcher's retrieved events conform to our expectations.
// Note: this test fails if the host system doesn't have at least 1 prior
// session.
TEST(SystemSessionAnalyzerTest, ValidateEvents) {
  SystemSessionAnalyzer analyzer(1U);
  EXPECT_EQ(SystemSessionAnalyzer::CLEAN,
            analyzer.IsSessionUnclean(base::Time::Now()));
}

// Stubs FetchEvents.
class StubSystemSessionAnalyzer : public SystemSessionAnalyzer {
 public:
  StubSystemSessionAnalyzer(uint32_t max_session_cnt)
      : SystemSessionAnalyzer(max_session_cnt) {}

  bool FetchEvents(size_t requested_events,
                   std::vector<EventInfo>* event_infos) override {
    DCHECK(event_infos);
    size_t num_to_copy = std::min(requested_events, events_.size());
    if (num_to_copy) {
      event_infos->clear();
      event_infos->insert(event_infos->begin(), events_.begin(),
                          events_.begin() + num_to_copy);
      events_.erase(events_.begin(), events_.begin() + num_to_copy);
    }

    return true;
  }

  void AddEvent(const EventInfo& info) { events_.push_back(info); }

 private:
  std::vector<EventInfo> events_;
};

TEST(SystemSessionAnalyzerTest, StandardCase) {
  StubSystemSessionAnalyzer analyzer(2U);

  base::Time time = base::Time::Now();
  analyzer.AddEvent({kIdSessionStart, time});
  analyzer.AddEvent(
      {kIdSessionEndUnclean, time - base::TimeDelta::FromSeconds(10)});
  analyzer.AddEvent({kIdSessionStart, time - base::TimeDelta::FromSeconds(20)});
  analyzer.AddEvent({kIdSessionEnd, time - base::TimeDelta::FromSeconds(22)});
  analyzer.AddEvent({kIdSessionStart, time - base::TimeDelta::FromSeconds(28)});

  EXPECT_EQ(SystemSessionAnalyzer::OUTSIDE_RANGE,
            analyzer.IsSessionUnclean(time - base::TimeDelta::FromSeconds(30)));
  EXPECT_EQ(SystemSessionAnalyzer::CLEAN,
            analyzer.IsSessionUnclean(time - base::TimeDelta::FromSeconds(25)));
  EXPECT_EQ(SystemSessionAnalyzer::UNCLEAN,
            analyzer.IsSessionUnclean(time - base::TimeDelta::FromSeconds(20)));
  EXPECT_EQ(SystemSessionAnalyzer::UNCLEAN,
            analyzer.IsSessionUnclean(time - base::TimeDelta::FromSeconds(15)));
  EXPECT_EQ(SystemSessionAnalyzer::UNCLEAN,
            analyzer.IsSessionUnclean(time - base::TimeDelta::FromSeconds(10)));
  EXPECT_EQ(SystemSessionAnalyzer::CLEAN,
            analyzer.IsSessionUnclean(time - base::TimeDelta::FromSeconds(5)));
  EXPECT_EQ(SystemSessionAnalyzer::CLEAN,
            analyzer.IsSessionUnclean(time + base::TimeDelta::FromSeconds(5)));
}

TEST(SystemSessionAnalyzerTest, NoEvent) {
  StubSystemSessionAnalyzer analyzer(0U);
  EXPECT_EQ(SystemSessionAnalyzer::FAILED,
            analyzer.IsSessionUnclean(base::Time::Now()));
}

TEST(SystemSessionAnalyzerTest, TimeInversion) {
  StubSystemSessionAnalyzer analyzer(1U);

  base::Time time = base::Time::Now();
  analyzer.AddEvent({kIdSessionStart, time});
  analyzer.AddEvent({kIdSessionEnd, time + base::TimeDelta::FromSeconds(1)});
  analyzer.AddEvent({kIdSessionStart, time - base::TimeDelta::FromSeconds(1)});

  EXPECT_EQ(SystemSessionAnalyzer::FAILED,
            analyzer.IsSessionUnclean(base::Time::Now()));
}

TEST(SystemSessionAnalyzerTest, IdInversion) {
  StubSystemSessionAnalyzer analyzer(1U);

  base::Time time = base::Time::Now();
  analyzer.AddEvent({kIdSessionStart, time});
  analyzer.AddEvent({kIdSessionStart, time - base::TimeDelta::FromSeconds(1)});
  analyzer.AddEvent({kIdSessionEnd, time - base::TimeDelta::FromSeconds(2)});

  EXPECT_EQ(SystemSessionAnalyzer::FAILED,
            analyzer.IsSessionUnclean(base::Time::Now()));
}

}  // namespace metrics
