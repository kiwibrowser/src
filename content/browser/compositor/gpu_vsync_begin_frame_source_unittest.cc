// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/gpu_vsync_begin_frame_source.h"

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

class TestBeginFrameObserver : public viz::BeginFrameObserverBase {
 public:
  TestBeginFrameObserver() = default;

  int begin_frame_count() { return begin_frame_count_; }

 private:
  bool OnBeginFrameDerivedImpl(const viz::BeginFrameArgs& args) override {
    begin_frame_count_++;
    return true;
  }

  void OnBeginFrameSourcePausedChanged(bool paused) override {}

  int begin_frame_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestBeginFrameObserver);
};

class TestGpuVSyncBeginFrameSource : public GpuVSyncBeginFrameSource {
 public:
  explicit TestGpuVSyncBeginFrameSource(GpuVSyncControl* vsync_control)
      : GpuVSyncBeginFrameSource(vsync_control) {}

  void SetNow(base::TimeTicks now) { now_ = now; }

 private:
  base::TimeTicks Now() const override { return now_; }

  base::TimeTicks now_;

  DISALLOW_COPY_AND_ASSIGN(TestGpuVSyncBeginFrameSource);
};

class GpuVSyncBeginFrameSourceTest : public testing::Test,
                                     public GpuVSyncControl {
 public:
  GpuVSyncBeginFrameSourceTest() = default;
  ~GpuVSyncBeginFrameSourceTest() override = default;

 protected:
  void SetNeedsVSync(bool needs_vsync) override { needs_vsync_ = needs_vsync; }

  bool needs_vsync_ = false;

  DISALLOW_COPY_AND_ASSIGN(GpuVSyncBeginFrameSourceTest);
};

// Test that an observer can be added to BFS and that it receives OnBeginFrame
// notification.
TEST_F(GpuVSyncBeginFrameSourceTest, BasicTest) {
  TestBeginFrameObserver observer;
  TestGpuVSyncBeginFrameSource begin_frame_source(this);

  base::TimeTicks now = base::TimeTicks() + base::TimeDelta::FromHours(2);
  begin_frame_source.SetNow(now);

  EXPECT_FALSE(needs_vsync_);

  begin_frame_source.AddObserver(&observer);

  EXPECT_TRUE(needs_vsync_);
  EXPECT_FALSE(observer.LastUsedBeginFrameArgs().IsValid());

  base::TimeTicks timestamp = now - base::TimeDelta::FromSeconds(1);
  base::TimeDelta interval = base::TimeDelta::FromSeconds(2);

  begin_frame_source.OnVSync(timestamp, interval);

  EXPECT_EQ(1, observer.begin_frame_count());

  viz::BeginFrameArgs args = observer.LastUsedBeginFrameArgs();
  EXPECT_TRUE(args.IsValid());
  EXPECT_EQ(timestamp, args.frame_time);
  EXPECT_EQ(interval, args.interval);
  EXPECT_EQ(timestamp + interval, args.deadline);
  EXPECT_EQ(viz::BeginFrameArgs::kStartingFrameNumber + 1,
            args.sequence_number);
  EXPECT_EQ(viz::BeginFrameArgs::NORMAL, args.type);
  EXPECT_EQ(begin_frame_source.source_id(), args.source_id);

  // Make sure that the deadline time is correctly advanced forward to be after
  // 'now' when frame time is way behind.
  now += base::TimeDelta::FromSeconds(10);
  begin_frame_source.SetNow(now);
  // v-sync timestamp is 5 seconds behind but frame time should be projected
  // forward to be 1 second before 'now' and the the deadline should be 1 second
  // after 'now' considering the 2 second interval.
  timestamp = now - base::TimeDelta::FromSeconds(5);

  begin_frame_source.OnVSync(timestamp, interval);

  EXPECT_EQ(2, observer.begin_frame_count());

  args = observer.LastUsedBeginFrameArgs();
  EXPECT_TRUE(args.IsValid());
  EXPECT_EQ(now - base::TimeDelta::FromSeconds(1), args.frame_time);
  EXPECT_EQ(interval, args.interval);
  EXPECT_EQ(now + base::TimeDelta::FromSeconds(1), args.deadline);
  EXPECT_EQ(viz::BeginFrameArgs::kStartingFrameNumber + 2,
            args.sequence_number);
  EXPECT_EQ(viz::BeginFrameArgs::NORMAL, args.type);
  EXPECT_EQ(begin_frame_source.source_id(), args.source_id);

  begin_frame_source.RemoveObserver(&observer);

  EXPECT_EQ(2, observer.begin_frame_count());
  EXPECT_FALSE(needs_vsync_);
}

// Test that MISSED OnBeginFrame is produced as expected.
TEST_F(GpuVSyncBeginFrameSourceTest, MissedBeginFrameArgs) {
  TestBeginFrameObserver observer1;
  TestBeginFrameObserver observer2;
  TestGpuVSyncBeginFrameSource begin_frame_source(this);

  base::TimeTicks now = base::TimeTicks() + base::TimeDelta::FromHours(2);
  begin_frame_source.SetNow(now);

  begin_frame_source.AddObserver(&observer1);

  // The observer shouldn't be getting any BeginFrame at this point.
  EXPECT_EQ(0, observer1.begin_frame_count());

  base::TimeDelta interval = base::TimeDelta::FromSeconds(2);

  // Trigger first OnBeginFrame.
  begin_frame_source.OnVSync(now, interval);

  // The observer should get a NORMAL BeginFrame notification which is covered
  // by BasicTest above.
  EXPECT_EQ(1, observer1.begin_frame_count());
  EXPECT_EQ(viz::BeginFrameArgs::NORMAL,
            observer1.LastUsedBeginFrameArgs().type);

  // Remove the first observer and add it back after advancing the 'now' time.
  // It should get a more recent missed notification calculated from the
  // projection of the previous notification.
  begin_frame_source.RemoveObserver(&observer1);
  now = now + base::TimeDelta::FromSeconds(5);
  begin_frame_source.SetNow(now);
  begin_frame_source.AddObserver(&observer1);

  EXPECT_EQ(2, observer1.begin_frame_count());

  // The projected MISSED frame_time should be 1 second behind 'now'.
  base::TimeTicks timestamp1 = now - base::TimeDelta::FromSeconds(1);
  viz::BeginFrameArgs args1 = observer1.LastUsedBeginFrameArgs();
  EXPECT_TRUE(args1.IsValid());
  EXPECT_EQ(timestamp1, args1.frame_time);
  EXPECT_EQ(timestamp1 + interval, args1.deadline);
  EXPECT_EQ(viz::BeginFrameArgs::kStartingFrameNumber + 2,
            args1.sequence_number);
  EXPECT_EQ(viz::BeginFrameArgs::MISSED, args1.type);

  // Add second observer which should receive the same notification.
  begin_frame_source.AddObserver(&observer2);

  EXPECT_EQ(1, observer2.begin_frame_count());

  viz::BeginFrameArgs args2 = observer2.LastUsedBeginFrameArgs();
  EXPECT_TRUE(args2.IsValid());
  EXPECT_EQ(timestamp1, args2.frame_time);
  EXPECT_EQ(timestamp1 + interval, args2.deadline);
  EXPECT_EQ(viz::BeginFrameArgs::kStartingFrameNumber + 2,
            args2.sequence_number);
  EXPECT_EQ(viz::BeginFrameArgs::MISSED, args2.type);

  // Adding and removing the second observer shouldn't produce any
  // new notifications.
  begin_frame_source.RemoveObserver(&observer2);
  begin_frame_source.AddObserver(&observer2);

  EXPECT_EQ(1, observer2.begin_frame_count());
}

}  // namespace
}  // namespace content
