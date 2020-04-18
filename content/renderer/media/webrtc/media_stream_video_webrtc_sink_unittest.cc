// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/media_stream_video_webrtc_sink.h"

#include "base/test/scoped_task_environment.h"
#include "content/child/child_process.h"
#include "content/renderer/media/stream/mock_constraint_factory.h"
#include "content/renderer/media/stream/mock_media_stream_registry.h"
#include "content/renderer/media/stream/video_track_adapter.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"

namespace content {
namespace {

class MediaStreamVideoWebRtcSinkTest : public ::testing::Test {
 public:
  MediaStreamVideoWebRtcSinkTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void SetVideoTrack() {
    registry_.Init("stream URL");
    registry_.AddVideoTrack("test video track");
    blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
    registry_.test_stream().VideoTracks(video_tracks);
    track_ = video_tracks[0];
    // TODO(hta): Verify that track_ is valid. When constraints produce
    // no valid format, using the track will cause a crash.
  }

  void SetVideoTrack(const base::Optional<bool>& noise_reduction) {
    registry_.Init("stream URL");
    registry_.AddVideoTrack("test video track", VideoTrackAdapterSettings(),
                            noise_reduction, false, 0.0);
    blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
    registry_.test_stream().VideoTracks(video_tracks);
    track_ = video_tracks[0];
    // TODO(hta): Verify that track_ is valid. When constraints produce
    // no valid format, using the track will cause a crash.
  }

 protected:
  blink::WebMediaStreamTrack track_;
  MockPeerConnectionDependencyFactory dependency_factory_;

 private:
  MockMediaStreamRegistry registry_;
  // A ChildProcess is needed to fool the Tracks and Sources into believing they
  // are on the right threads. A ScopedTaskEnvironment must be instantiated
  // before ChildProcess to prevent it from leaking a TaskScheduler.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  const ChildProcess child_process_;
};

TEST_F(MediaStreamVideoWebRtcSinkTest, NoiseReductionDefaultsToNotSet) {
  SetVideoTrack();
  MediaStreamVideoWebRtcSink my_sink(
      track_, &dependency_factory_,
      blink::scheduler::GetSingleThreadTaskRunnerForTesting());
  EXPECT_TRUE(my_sink.webrtc_video_track());
  EXPECT_FALSE(my_sink.SourceNeedsDenoisingForTesting());
}

}  // namespace
}  // namespace content
