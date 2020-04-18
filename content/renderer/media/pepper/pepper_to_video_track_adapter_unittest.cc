// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/run_loop.h"
#include "content/child/child_process.h"
#include "content/public/test/mock_render_thread.h"
#include "content/renderer/media/pepper/pepper_to_video_track_adapter.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/stream/mock_media_stream_registry.h"
#include "content/renderer/media/stream/mock_media_stream_video_sink.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/ppb_image_data_impl.h"
#include "content/test/ppapi_unittest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_heap.h"

using ::testing::_;

namespace content {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

static const std::string kTestStreamUrl = "stream_url";
static const std::string kUnknownStreamUrl = "unknown_stream_url";

class PepperToVideoTrackAdapterTest : public PpapiUnittest {
 public:
  PepperToVideoTrackAdapterTest() : registry_(new MockMediaStreamRegistry()) {
    registry_->Init(kTestStreamUrl);
  }

  void TearDown() override {
    registry_.reset();
    blink::WebHeap::CollectAllGarbageForTesting();
    PpapiUnittest::TearDown();
  }

 protected:
  // A ChildProcess is needed to fool the Tracks and Sources into believing they
  // are on the right threads. The ScopedTaskEnvironment provided by
  // PpapiUnittest prevents the ChildProcess from leaking a TaskScheduler.
  const ChildProcess child_process_;
  const MockRenderThread render_thread_;
  std::unique_ptr<MockMediaStreamRegistry> registry_;
};

TEST_F(PepperToVideoTrackAdapterTest, Open) {
  // |frame_writer| is a proxy and is owned by whoever call Open.
  FrameWriterInterface* frame_writer = nullptr;
  // Unknow url will return false.
  EXPECT_FALSE(PepperToVideoTrackAdapter::Open(registry_.get(),
                                             kUnknownStreamUrl, &frame_writer));
  EXPECT_TRUE(PepperToVideoTrackAdapter::Open(registry_.get(),
                                            kTestStreamUrl, &frame_writer));
  delete frame_writer;
}

TEST_F(PepperToVideoTrackAdapterTest, PutFrame) {
  FrameWriterInterface* frame_writer = nullptr;
  EXPECT_TRUE(PepperToVideoTrackAdapter::Open(registry_.get(),
                                            kTestStreamUrl, &frame_writer));
  ASSERT_TRUE(frame_writer);

  // Verify the video track has been added.
  const blink::WebMediaStream test_stream = registry_->test_stream();
  blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
  test_stream.VideoTracks(video_tracks);
  ASSERT_EQ(1u, video_tracks.size());

  // Verify the native video track has been added.
  MediaStreamVideoTrack* native_track =
      MediaStreamVideoTrack::GetVideoTrack(video_tracks[0]);
  ASSERT_TRUE(native_track != nullptr);

  MockMediaStreamVideoSink sink;
  native_track->AddSink(&sink, sink.GetDeliverFrameCB(), false);
  scoped_refptr<PPB_ImageData_Impl> image(new PPB_ImageData_Impl(
      instance()->pp_instance(), PPB_ImageData_Impl::ForTest()));
  image->Init(PP_IMAGEDATAFORMAT_BGRA_PREMUL, 640, 360, true);
  {
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();

    EXPECT_CALL(sink, OnVideoFrame())
        .WillOnce(RunClosure(std::move(quit_closure)));
    frame_writer->PutFrame(image.get(), 10);
    run_loop.Run();
    // Run all pending tasks to let the the test clean up before the test ends.
    // This is due to that
    // FrameWriterDelegate::FrameWriterDelegate::DeliverFrame use
    // PostTaskAndReply to the IO thread and expects the reply to process
    // on the main render thread to clean up its resources. However, the
    // QuitClosure above ends before that.
    base::RunLoop().RunUntilIdle();
  }
  EXPECT_EQ(1, sink.number_of_frames());
  native_track->RemoveSink(&sink);

  // The |frame_writer| is a proxy and is owned by whoever call Open.
  delete frame_writer;
}

}  // namespace content
