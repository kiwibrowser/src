// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/test/scoped_task_environment.h"
#include "content/child/child_process.h"
#include "content/renderer/media/pepper/video_track_to_pepper_adapter.h"
#include "content/renderer/media/stream/mock_media_stream_registry.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_heap.h"

namespace content {

static const std::string kTestStreamUrl = "stream_url";
static const std::string kTestVideoTrackId = "video_track_id";
static const std::string kUnknownStreamUrl = "unknown_stream_url";

class VideoTrackToPepperAdapterTest : public ::testing::Test,
                                      public FrameReaderInterface {
 public:
  VideoTrackToPepperAdapterTest() : registry_(new MockMediaStreamRegistry()) {
    handler_.reset(new VideoTrackToPepperAdapter(registry_.get()));
    registry_->Init(kTestStreamUrl);
    registry_->AddVideoTrack(kTestVideoTrackId);
    EXPECT_FALSE(handler_->GetFirstVideoTrack(kTestStreamUrl).IsNull());
  }

  MOCK_METHOD1(GotFrame, void(const scoped_refptr<media::VideoFrame>&));

  void TearDown() override {
    registry_.reset();
    handler_.reset();
    blink::WebHeap::CollectAllGarbageForTesting();
  }

  void DeliverFrameForTesting(const scoped_refptr<media::VideoFrame>& frame) {
    handler_->DeliverFrameForTesting(this, frame);
  }

 protected:
  // A ChildProcess is needed to fool the Tracks and Sources below into
  // believing they are on the right threads. A ScopedTaskEnvironment must be
  // instantiated before ChildProcess to prevent it from leaking a
  // TaskScheduler.
  const base::test::ScopedTaskEnvironment scoped_task_environment_;
  const ChildProcess child_process_;
  std::unique_ptr<VideoTrackToPepperAdapter> handler_;
  std::unique_ptr<MockMediaStreamRegistry> registry_;
};

// Open |handler_| and send a VideoFrame to be received at the other side.
TEST_F(VideoTrackToPepperAdapterTest, OpenClose) {
  // Unknow url will return false.
  EXPECT_FALSE(handler_->Open(kUnknownStreamUrl, this));
  EXPECT_TRUE(handler_->Open(kTestStreamUrl, this));

  const base::TimeDelta ts = base::TimeDelta::FromMilliseconds(789012);
  const scoped_refptr<media::VideoFrame> captured_frame =
      media::VideoFrame::CreateBlackFrame(gfx::Size(640, 360));
  captured_frame->set_timestamp(ts);

  EXPECT_CALL(*this, GotFrame(captured_frame));
  DeliverFrameForTesting(captured_frame);

  EXPECT_FALSE(handler_->Close(nullptr));
  EXPECT_TRUE(handler_->Close(this));
}

TEST_F(VideoTrackToPepperAdapterTest, OpenWithoutClose) {
  EXPECT_TRUE(handler_->Open(kTestStreamUrl, this));
}

}  // namespace content
