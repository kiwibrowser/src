// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_task_environment.h"
#include "content/child/child_process.h"
#include "content/renderer/media/mock_audio_device_factory.h"
#include "content/renderer/media/stream/media_stream_video_source.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/stream/mock_media_stream_video_source.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_heap.h"

using ::testing::_;

namespace content {

class WebRtcMediaStreamAdapterMapTest : public ::testing::Test {
 public:
  void SetUp() override {
    dependency_factory_.reset(new MockPeerConnectionDependencyFactory());
    main_thread_ = blink::scheduler::GetSingleThreadTaskRunnerForTesting();
    map_ = new WebRtcMediaStreamAdapterMap(
        dependency_factory_.get(), main_thread_,
        new WebRtcMediaStreamTrackAdapterMap(dependency_factory_.get(),
                                             main_thread_));
  }

  void TearDown() override { blink::WebHeap::CollectAllGarbageForTesting(); }

  blink::WebMediaStream CreateLocalStream(const std::string& id) {
    blink::WebVector<blink::WebMediaStreamTrack> web_video_tracks(
        static_cast<size_t>(1));
    blink::WebMediaStreamSource video_source;
    video_source.Initialize("video_source",
                            blink::WebMediaStreamSource::kTypeVideo,
                            "video_source", false /* remote */);
    MediaStreamVideoSource* native_source = new MockMediaStreamVideoSource();
    video_source.SetExtraData(native_source);
    web_video_tracks[0] = MediaStreamVideoTrack::CreateVideoTrack(
        native_source, MediaStreamVideoSource::ConstraintsCallback(), true);

    blink::WebMediaStream web_stream;
    web_stream.Initialize(blink::WebString::FromUTF8(id),
                          blink::WebVector<blink::WebMediaStreamTrack>(),
                          web_video_tracks);
    return web_stream;
  }

  scoped_refptr<webrtc::MediaStreamInterface> CreateRemoteStream(
      const std::string& id) {
    scoped_refptr<webrtc::MediaStreamInterface> stream(
        new rtc::RefCountedObject<MockMediaStream>(id));
    stream->AddTrack(MockWebRtcAudioTrack::Create("remote_audio_track").get());
    stream->AddTrack(MockWebRtcVideoTrack::Create("remote_video_track").get());
    return stream;
  }

  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>
  GetOrCreateRemoteStreamAdapter(webrtc::MediaStreamInterface* webrtc_stream) {
    std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref;
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&WebRtcMediaStreamAdapterMapTest::
                           GetOrCreateRemoteStreamAdapterOnSignalingThread,
                       base::Unretained(this), base::Unretained(webrtc_stream),
                       base::Unretained(&adapter_ref)));
    RunMessageLoopsUntilIdle();
    DCHECK(adapter_ref);
    return adapter_ref;
  }

 protected:
  void GetOrCreateRemoteStreamAdapterOnSignalingThread(
      webrtc::MediaStreamInterface* webrtc_stream,
      std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>* adapter_ref) {
    *adapter_ref = map_->GetOrCreateRemoteStreamAdapter(webrtc_stream);
    EXPECT_TRUE(*adapter_ref);
  }

  // Runs message loops on the webrtc signaling thread and the main thread until
  // idle.
  void RunMessageLoopsUntilIdle() {
    base::WaitableEvent waitable_event(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE, base::BindOnce(&WebRtcMediaStreamAdapterMapTest::SignalEvent,
                                  base::Unretained(this), &waitable_event));
    waitable_event.Wait();
    base::RunLoop().RunUntilIdle();
  }

  void SignalEvent(base::WaitableEvent* waitable_event) {
    waitable_event->Signal();
  }

  // Message loop and child processes is needed for task queues and threading to
  // work, as is necessary to create tracks and adapters.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ChildProcess child_process_;

  std::unique_ptr<MockPeerConnectionDependencyFactory> dependency_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<WebRtcMediaStreamAdapterMap> map_;
};

TEST_F(WebRtcMediaStreamAdapterMapTest, AddAndRemoveLocalStreamAdapter) {
  blink::WebMediaStream local_web_stream = CreateLocalStream("local_stream");
  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref =
      map_->GetOrCreateLocalStreamAdapter(local_web_stream);
  EXPECT_TRUE(adapter_ref);
  EXPECT_TRUE(adapter_ref->adapter().IsEqual(local_web_stream));
  EXPECT_EQ(1u, map_->GetLocalStreamCount());

  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref2 =
      map_->GetLocalStreamAdapter(local_web_stream);
  EXPECT_TRUE(adapter_ref2);
  EXPECT_EQ(&adapter_ref2->adapter(), &adapter_ref->adapter());
  EXPECT_EQ(1u, map_->GetLocalStreamCount());

  adapter_ref.reset();
  EXPECT_EQ(1u, map_->GetLocalStreamCount());
  adapter_ref2.reset();
  EXPECT_EQ(0u, map_->GetLocalStreamCount());
}

TEST_F(WebRtcMediaStreamAdapterMapTest, LookUpLocalAdapterByWebRtcStream) {
  blink::WebMediaStream local_web_stream = CreateLocalStream("local_stream");
  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref =
      map_->GetOrCreateLocalStreamAdapter(local_web_stream);
  EXPECT_TRUE(adapter_ref);
  EXPECT_TRUE(adapter_ref->adapter().IsEqual(local_web_stream));
  EXPECT_EQ(1u, map_->GetLocalStreamCount());

  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref2 =
      map_->GetLocalStreamAdapter(adapter_ref->adapter().web_stream());
  EXPECT_TRUE(adapter_ref2);
  EXPECT_EQ(&adapter_ref2->adapter(), &adapter_ref->adapter());
}

TEST_F(WebRtcMediaStreamAdapterMapTest, GetLocalStreamAdapterInvalidID) {
  blink::WebMediaStream local_web_stream = CreateLocalStream("missing");
  EXPECT_FALSE(map_->GetLocalStreamAdapter(local_web_stream));
}

TEST_F(WebRtcMediaStreamAdapterMapTest, AddAndRemoveRemoteStreamAdapter) {
  scoped_refptr<webrtc::MediaStreamInterface> remote_webrtc_stream =
      CreateRemoteStream("remote_stream");
  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref =
      GetOrCreateRemoteStreamAdapter(remote_webrtc_stream.get());
  EXPECT_EQ(remote_webrtc_stream, adapter_ref->adapter().webrtc_stream());
  EXPECT_EQ(1u, map_->GetRemoteStreamCount());

  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref2 =
      map_->GetRemoteStreamAdapter(remote_webrtc_stream.get());
  EXPECT_TRUE(adapter_ref2);
  EXPECT_EQ(&adapter_ref2->adapter(), &adapter_ref->adapter());
  EXPECT_EQ(1u, map_->GetRemoteStreamCount());

  adapter_ref.reset();
  EXPECT_EQ(1u, map_->GetRemoteStreamCount());
  adapter_ref2.reset();
  EXPECT_EQ(0u, map_->GetRemoteStreamCount());
}

TEST_F(WebRtcMediaStreamAdapterMapTest, LookUpRemoteAdapterByWebStream) {
  scoped_refptr<webrtc::MediaStreamInterface> remote_webrtc_stream =
      CreateRemoteStream("remote_stream");
  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref =
      GetOrCreateRemoteStreamAdapter(remote_webrtc_stream.get());
  EXPECT_EQ(remote_webrtc_stream, adapter_ref->adapter().webrtc_stream());
  EXPECT_EQ(1u, map_->GetRemoteStreamCount());

  std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> adapter_ref2 =
      map_->GetRemoteStreamAdapter(
          adapter_ref->adapter().webrtc_stream().get());
  EXPECT_TRUE(adapter_ref2);
  EXPECT_EQ(&adapter_ref2->adapter(), &adapter_ref->adapter());
}

TEST_F(WebRtcMediaStreamAdapterMapTest, GetRemoteStreamAdapterInvalidID) {
  scoped_refptr<webrtc::MediaStreamInterface> remote_webrtc_stream =
      CreateRemoteStream("missing");
  EXPECT_FALSE(map_->GetRemoteStreamAdapter(remote_webrtc_stream.get()));
}

}  // namespace content
