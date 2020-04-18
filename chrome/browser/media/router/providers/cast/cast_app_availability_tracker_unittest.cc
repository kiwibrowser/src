// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_app_availability_tracker.h"

#include "base/test/simple_test_tick_clock.h"
#include "chrome/common/media_router/providers/cast/cast_media_source.h"
#include "components/cast_channel/cast_message_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using cast_channel::GetAppAvailabilityResult;

namespace media_router {

namespace {

MATCHER_P(CastMediaSourcesEqual, expected, "") {
  if (expected.size() != arg.size())
    return false;
  return std::equal(
      expected.begin(), expected.end(), arg.begin(),
      [](const CastMediaSource& source1, const CastMediaSource& source2) {
        return source1.source_id() == source2.source_id();
      });
}

}  // namespace

class CastAppAvailabilityTrackerTest : public testing::Test {
 public:
  CastAppAvailabilityTrackerTest() {}
  ~CastAppAvailabilityTrackerTest() override = default;

  base::TimeTicks Now() const { return clock_.NowTicks(); }

 protected:
  base::SimpleTestTickClock clock_;
  CastAppAvailabilityTracker tracker_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastAppAvailabilityTrackerTest);
};

TEST_F(CastAppAvailabilityTrackerTest, RegisterSource) {
  auto source1 = CastMediaSource::From("cast:AAAAAAAA?clientId=1");
  ASSERT_TRUE(source1);
  auto source2 = CastMediaSource::From("cast:AAAAAAAA?clientId=2");
  ASSERT_TRUE(source2);

  base::flat_set<std::string> expected_app_ids = {"AAAAAAAA"};
  EXPECT_EQ(expected_app_ids, tracker_.RegisterSource(*source1));

  expected_app_ids.clear();
  EXPECT_EQ(expected_app_ids, tracker_.RegisterSource(*source1));
  EXPECT_EQ(expected_app_ids, tracker_.RegisterSource(*source2));

  tracker_.UnregisterSource(*source1);
  tracker_.UnregisterSource(*source2);

  expected_app_ids = {"AAAAAAAA"};
  EXPECT_EQ(expected_app_ids, tracker_.RegisterSource(*source1));
  EXPECT_EQ(expected_app_ids, tracker_.GetRegisteredApps());
}

TEST_F(CastAppAvailabilityTrackerTest, RegisterSourceReturnsMultipleAppIds) {
  auto source1 = CastMediaSource::From("urn:x-org.chromium.media:source:tab:1");
  ASSERT_TRUE(source1);

  // Mirorring app ids.
  base::flat_set<std::string> expected_app_ids = {"0F5096E8", "85CDB22F"};
  EXPECT_EQ(expected_app_ids, tracker_.RegisterSource(*source1));
  EXPECT_EQ(expected_app_ids, tracker_.GetRegisteredApps());
}

TEST_F(CastAppAvailabilityTrackerTest, MultipleAppIdsAlreadyTrackingOne) {
  // One of the mirroring app IDs.
  auto source1 = CastMediaSource::From("cast:0F5096E8");
  ASSERT_TRUE(source1);

  base::flat_set<std::string> new_app_ids = {"0F5096E8"};
  base::flat_set<std::string> registered_app_ids = {"0F5096E8"};
  EXPECT_EQ(new_app_ids, tracker_.RegisterSource(*source1));
  EXPECT_EQ(registered_app_ids, tracker_.GetRegisteredApps());

  auto source2 = CastMediaSource::From("urn:x-org.chromium.media:source:tab:1");
  ASSERT_TRUE(source2);

  new_app_ids = {"85CDB22F"};
  registered_app_ids = {"0F5096E8", "85CDB22F"};

  EXPECT_EQ(new_app_ids, tracker_.RegisterSource(*source2));
  EXPECT_EQ(registered_app_ids, tracker_.GetRegisteredApps());
}

TEST_F(CastAppAvailabilityTrackerTest, UpdateAppAvailability) {
  auto source1 = CastMediaSource::From("cast:AAAAAAAA?clientId=1");
  ASSERT_TRUE(source1);
  auto source2 = CastMediaSource::From("cast:AAAAAAAA?clientId=2");
  ASSERT_TRUE(source2);
  auto source3 = CastMediaSource::From("cast:BBBBBBBB?clientId=3");
  ASSERT_TRUE(source3);

  tracker_.RegisterSource(*source3);

  // |source3| not affected.
  EXPECT_THAT(
      tracker_.UpdateAppAvailability(
          "sinkId1", "AAAAAAAA", {GetAppAvailabilityResult::kAvailable, Now()}),
      CastMediaSourcesEqual(std::vector<CastMediaSource>()));

  base::flat_set<MediaSink::Id> sinks_1 = {"sinkId1"};
  base::flat_set<MediaSink::Id> sinks_1_2 = {"sinkId1", "sinkId2"};
  std::vector<CastMediaSource> sources_1 = {*source1};
  std::vector<CastMediaSource> sources_1_2 = {*source1, *source2};

  // Tracker returns available sinks even though sources aren't registered.
  EXPECT_EQ(sinks_1, tracker_.GetAvailableSinks(*source1));
  EXPECT_EQ(sinks_1, tracker_.GetAvailableSinks(*source2));
  EXPECT_TRUE(tracker_.GetAvailableSinks(*source3).empty());

  tracker_.RegisterSource(*source1);
  // Only |source1| is registered for this app.
  EXPECT_THAT(
      tracker_.UpdateAppAvailability(
          "sinkId2", "AAAAAAAA", {GetAppAvailabilityResult::kAvailable, Now()}),
      CastMediaSourcesEqual(sources_1));
  EXPECT_EQ(sinks_1_2, tracker_.GetAvailableSinks(*source1));
  EXPECT_EQ(sinks_1_2, tracker_.GetAvailableSinks(*source2));
  EXPECT_TRUE(tracker_.GetAvailableSinks(*source3).empty());

  tracker_.RegisterSource(*source2);
  EXPECT_THAT(tracker_.UpdateAppAvailability(
                  "sinkId2", "AAAAAAAA",
                  {GetAppAvailabilityResult::kUnavailable, Now()}),
              CastMediaSourcesEqual(sources_1_2));
  EXPECT_EQ(sinks_1, tracker_.GetAvailableSinks(*source1));
  EXPECT_EQ(sinks_1, tracker_.GetAvailableSinks(*source2));
  EXPECT_TRUE(tracker_.GetAvailableSinks(*source3).empty());
}

TEST_F(CastAppAvailabilityTrackerTest, RemoveResultsForSink) {
  auto source1 = CastMediaSource::From("cast:AAAAAAAA?clientId=1");
  ASSERT_TRUE(source1);

  tracker_.UpdateAppAvailability("sinkId1", "AAAAAAAA",
                                 {GetAppAvailabilityResult::kAvailable, Now()});
  EXPECT_EQ(GetAppAvailabilityResult::kAvailable,
            tracker_.GetAvailability("sinkId1", "AAAAAAAA").first);

  base::flat_set<MediaSink::Id> expected_sink_ids = {"sinkId1"};
  EXPECT_EQ(expected_sink_ids, tracker_.GetAvailableSinks(*source1));

  // Unrelated sink ID.
  tracker_.RemoveResultsForSink("sinkId2");
  EXPECT_EQ(GetAppAvailabilityResult::kAvailable,
            tracker_.GetAvailability("sinkId1", "AAAAAAAA").first);
  EXPECT_EQ(expected_sink_ids, tracker_.GetAvailableSinks(*source1));

  expected_sink_ids.clear();
  tracker_.RemoveResultsForSink("sinkId1");
  EXPECT_EQ(GetAppAvailabilityResult::kUnknown,
            tracker_.GetAvailability("sinkId1", "AAAAAAAA").first);
  EXPECT_EQ(expected_sink_ids, tracker_.GetAvailableSinks(*source1));
}

}  // namespace media_router
