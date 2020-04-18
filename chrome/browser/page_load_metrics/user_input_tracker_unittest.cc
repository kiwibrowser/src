// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/user_input_tracker.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_input_event.h"

namespace page_load_metrics {

namespace {

// UserInputTracker allows events to be at most 2 seconds old. Thus we use
// 2100ms to make sure the age is greater than 2 seconds.
const int kTooOldMilliseconds = 2100;

class FakeInputEvent : public blink::WebInputEvent {
 public:
  FakeInputEvent(blink::WebInputEvent::Type type = blink::WebInputEvent::kChar,
                 int modifiers = blink::WebInputEvent::kNoModifiers)
      : WebInputEvent(sizeof(FakeInputEvent),
                      type,
                      modifiers,
                      base::TimeTicks::Now()) {}

  base::TimeTicks GetTimeStampRounded() {
    return UserInputTracker::RoundToRateLimitedOffset(TimeStamp());
  }
};

}  // namespace

class UserInputTrackerTest : public testing::Test {};

TEST_F(UserInputTrackerTest, Basic) {
  UserInputTracker tracker;
  EXPECT_EQ(base::TimeTicks(),
            tracker.FindMostRecentUserInputEventBefore(base::TimeTicks()));
  EXPECT_EQ(base::TimeTicks(),
            tracker.FindMostRecentUserInputEventBefore(base::TimeTicks::Now()));
}

TEST_F(UserInputTrackerTest, SingleEvent) {
  UserInputTracker tracker;
  FakeInputEvent e;
  tracker.OnInputEvent(e);

  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e.GetTimeStampRounded()));

  base::TimeTicks after =
      e.GetTimeStampRounded() + base::TimeDelta::FromMicroseconds(1);

  EXPECT_EQ(e.GetTimeStampRounded(),
            tracker.FindMostRecentUserInputEventBefore(after));

  EXPECT_TRUE(tracker.FindAndConsumeInputEventsBefore(after));

  EXPECT_EQ(base::TimeTicks(),
            tracker.FindMostRecentUserInputEventBefore(after));
}

TEST_F(UserInputTrackerTest, MultipleEvents) {
  FakeInputEvent e1;
  FakeInputEvent e2;

  // Make sure that the two events are monotonically increasing, and that both
  // are in the past.
  e1.SetTimeStamp(e2.TimeStamp() - base::TimeDelta::FromMilliseconds(100));

  base::TimeTicks after =
      e2.GetTimeStampRounded() + base::TimeDelta::FromMicroseconds(1);

  {
    UserInputTracker tracker;
    tracker.OnInputEvent(e1);
    tracker.OnInputEvent(e2);

    EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                     e1.GetTimeStampRounded()));
    EXPECT_EQ(
        e1.GetTimeStampRounded(),
        tracker.FindMostRecentUserInputEventBefore(e2.GetTimeStampRounded()));

    EXPECT_EQ(e2.GetTimeStampRounded(),
              tracker.FindMostRecentUserInputEventBefore(after));

    EXPECT_FALSE(
        tracker.FindAndConsumeInputEventsBefore(e1.GetTimeStampRounded()));
    EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                     e1.GetTimeStampRounded()));
    EXPECT_EQ(
        e1.GetTimeStampRounded(),
        tracker.FindMostRecentUserInputEventBefore(e2.GetTimeStampRounded()));
    EXPECT_EQ(e2.GetTimeStampRounded(),
              tracker.FindMostRecentUserInputEventBefore(after));

    EXPECT_TRUE(tracker.FindAndConsumeInputEventsBefore(after));
    EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                     e1.GetTimeStampRounded()));
    EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                     e2.GetTimeStampRounded()));
    EXPECT_EQ(base::TimeTicks(),
              tracker.FindMostRecentUserInputEventBefore(after));
  }

  {
    UserInputTracker tracker;
    tracker.OnInputEvent(e1);
    tracker.OnInputEvent(e2);
    EXPECT_EQ(e2.GetTimeStampRounded(),
              tracker.FindMostRecentUserInputEventBefore(after));
    EXPECT_TRUE(tracker.FindAndConsumeInputEventsBefore(after));
    EXPECT_EQ(base::TimeTicks(),
              tracker.FindMostRecentUserInputEventBefore(after));
  }
}

TEST_F(UserInputTrackerTest, IgnoreEventsOlderThanConsumed) {
  FakeInputEvent e1;
  FakeInputEvent e2;

  // Make sure that the two events are monotonically increasing, and that both
  // are in the past.
  e1.SetTimeStamp(e2.TimeStamp() - base::TimeDelta::FromMilliseconds(100));

  base::TimeTicks after =
      e2.GetTimeStampRounded() + base::TimeDelta::FromMicroseconds(1);

  UserInputTracker tracker;
  tracker.OnInputEvent(e2);

  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e1.GetTimeStampRounded()));
  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e2.GetTimeStampRounded()));
  EXPECT_EQ(e2.GetTimeStampRounded(),
            tracker.FindMostRecentUserInputEventBefore(after));

  EXPECT_TRUE(tracker.FindAndConsumeInputEventsBefore(after));
  EXPECT_EQ(base::TimeTicks(),
            tracker.FindMostRecentUserInputEventBefore(after));

  tracker.OnInputEvent(e1);
  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e1.GetTimeStampRounded()));
  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e1.GetTimeStampRounded() +
                                   base::TimeDelta::FromMicroseconds(1)));
}

TEST_F(UserInputTrackerTest, ExcludeOldEvents) {
  UserInputTracker tracker;
  FakeInputEvent e1;
  FakeInputEvent e2;
  // make sure e1 is too old to be considered.
  e1.SetTimeStamp(e2.TimeStamp() -
                  base::TimeDelta::FromMilliseconds(kTooOldMilliseconds));

  tracker.OnInputEvent(e1);
  tracker.OnInputEvent(e2);

  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e1.GetTimeStampRounded() +
                                   base::TimeDelta::FromMilliseconds(1)));
  EXPECT_EQ(base::TimeTicks(),
            tracker.FindMostRecentUserInputEventBefore(
                e2.GetTimeStampRounded() +
                base::TimeDelta::FromMilliseconds(kTooOldMilliseconds)));
  EXPECT_EQ(
      e2.GetTimeStampRounded(),
      tracker.FindMostRecentUserInputEventBefore(
          e2.GetTimeStampRounded() + base::TimeDelta::FromMilliseconds(1)));

  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e2.GetTimeStampRounded()));
}

TEST_F(UserInputTrackerTest, RateLimit) {
  const size_t kTooManyEntries = UserInputTracker::kMaxTrackedEvents * 5;

  UserInputTracker tracker;
  FakeInputEvent e;

  // UserInputTracker DCHECKs that event timestamps aren't after the current
  // time, so to be safe, we use a starting timestamp that is twice
  // kTooManyEntries milliseconds in the past, and then synthesize one event for
  // each of kTooManyEntries after this start point. This guarantees that all
  // events are in the past.
  e.SetTimeStamp(e.TimeStamp() -
                 base::TimeDelta::FromMilliseconds(kTooManyEntries * 2));

  // Insert more than kMaxEntries entries. The rate limiting logic should
  // prevent more than kMaxEntries entries from actually being inserted. A
  // DCHECK in OnInputEvent verifies that we don't exceed the expected capacity.
  for (size_t i = 0; i < kTooManyEntries; ++i) {
    tracker.OnInputEvent(e);
    e.SetTimeStamp(e.TimeStamp() + base::TimeDelta::FromMilliseconds(1));
  }

  // Do a basic sanity check to make sure we can find events in the tracker.
  EXPECT_NE(base::TimeTicks(),
            tracker.FindMostRecentUserInputEventBefore(base::TimeTicks::Now()));
}

TEST_F(UserInputTrackerTest, IgnoredEventType) {
  UserInputTracker tracker;
  FakeInputEvent e(blink::WebInputEvent::kMouseMove);
  tracker.OnInputEvent(e);
  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e.GetTimeStampRounded() +
                                   base::TimeDelta::FromMilliseconds(1)));
}

TEST_F(UserInputTrackerTest, IgnoreRepeatEvents) {
  UserInputTracker tracker;
  FakeInputEvent e(blink::WebInputEvent::kChar,
                   blink::WebInputEvent::kIsAutoRepeat);
  tracker.OnInputEvent(e);
  EXPECT_EQ(base::TimeTicks(), tracker.FindMostRecentUserInputEventBefore(
                                   e.GetTimeStampRounded() +
                                   base::TimeDelta::FromMilliseconds(1)));
}

}  // namespace page_load_metrics
