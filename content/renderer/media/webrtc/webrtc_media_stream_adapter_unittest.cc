// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_media_stream_adapter.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/child_process.h"
#include "content/renderer/media/mock_audio_device_factory.h"
#include "content/renderer/media/stream/media_stream_video_source.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/media/stream/mock_constraint_factory.h"
#include "content/renderer/media/stream/mock_media_stream_video_source.h"
#include "content/renderer/media/stream/processed_local_audio_source.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_heap.h"

using ::testing::_;

namespace content {

class WebRtcMediaStreamAdapterTest : public ::testing::Test {
 public:
  void SetUp() override {
    dependency_factory_.reset(new MockPeerConnectionDependencyFactory());
    track_adapter_map_ = new WebRtcMediaStreamTrackAdapterMap(
        dependency_factory_.get(),
        blink::scheduler::GetSingleThreadTaskRunnerForTesting());
  }

  void TearDown() override {
    track_adapter_map_ = nullptr;
    blink::WebHeap::CollectAllGarbageForTesting();
  }

 protected:
  // The ScopedTaskEnvironment prevents the ChildProcess from leaking a
  // TaskScheduler.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ChildProcess child_process_;
  std::unique_ptr<MockPeerConnectionDependencyFactory> dependency_factory_;
  scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map_;
};

class LocalWebRtcMediaStreamAdapterTest : public WebRtcMediaStreamAdapterTest {
 public:
  blink::WebMediaStream CreateWebMediaStream(
      const std::string& audio_track_id,
      const std::string& video_track_id) {
    blink::WebVector<blink::WebMediaStreamTrack> audio_tracks(
        static_cast<size_t>(1u));
    blink::WebMediaStreamSource audio_source;
    audio_source.Initialize(blink::WebString::FromUTF8(audio_track_id),
                            blink::WebMediaStreamSource::kTypeAudio,
                            blink::WebString::FromUTF8(audio_track_id),
                            false /* remote */);
    ProcessedLocalAudioSource* const source = new ProcessedLocalAudioSource(
        -1 /* consumer_render_frame_id is N/A for non-browser tests */,
        MediaStreamDevice(MEDIA_DEVICE_AUDIO_CAPTURE, "mock_audio_device_id",
                          "Mock audio device",
                          media::AudioParameters::kAudioCDSampleRate,
                          media::CHANNEL_LAYOUT_STEREO,
                          media::AudioParameters::kAudioCDSampleRate / 50),
        false /* hotword_enabled */, false /* disable_local_echo */,
        AudioProcessingProperties(),
        base::Bind(&LocalWebRtcMediaStreamAdapterTest::OnAudioSourceStarted),
        dependency_factory_.get());
    source->SetAllowInvalidRenderFrameIdForTesting(true);
    audio_source.SetExtraData(source);  // Takes ownership.
    audio_tracks[0].Initialize(blink::WebString::FromUTF8(audio_track_id),
                               audio_source);
    EXPECT_CALL(*mock_audio_device_factory_.mock_capturer_source(),
                Initialize(_, _));
    EXPECT_CALL(*mock_audio_device_factory_.mock_capturer_source(),
                SetAutomaticGainControl(true));
    EXPECT_CALL(*mock_audio_device_factory_.mock_capturer_source(), Start());
    EXPECT_CALL(*mock_audio_device_factory_.mock_capturer_source(), Stop());
    CHECK(source->ConnectToTrack(audio_tracks[0]));

    blink::WebVector<blink::WebMediaStreamTrack> video_tracks(
        static_cast<size_t>(1u));
    MediaStreamSource::SourceStoppedCallback dummy_callback;
    blink::WebMediaStreamSource video_source;
    video_source.Initialize(blink::WebString::FromUTF8(video_track_id),
                            blink::WebMediaStreamSource::kTypeVideo,
                            blink::WebString::FromUTF8(video_track_id),
                            false /* remote */);
    MediaStreamVideoSource* native_source = new MockMediaStreamVideoSource();
    video_source.SetExtraData(native_source);
    video_tracks[0] = MediaStreamVideoTrack::CreateVideoTrack(
        blink::WebString::FromUTF8(video_track_id), native_source,
        MediaStreamVideoSource::ConstraintsCallback(), true);

    blink::WebMediaStream stream_desc;
    stream_desc.Initialize("stream_id", audio_tracks, video_tracks);
    return stream_desc;
  }

 private:
  static void OnAudioSourceStarted(MediaStreamSource* source,
                                   MediaStreamRequestResult result,
                                   const blink::WebString& result_name) {}

  MockAudioDeviceFactory mock_audio_device_factory_;
};

class RemoteWebRtcMediaStreamAdapterTest : public WebRtcMediaStreamAdapterTest {
 public:
  scoped_refptr<webrtc::MediaStreamInterface> CreateWebRtcMediaStream(
      const std::string& audio_track_id,
      const std::string& video_track_id) {
    scoped_refptr<webrtc::MediaStreamInterface> stream(
        new rtc::RefCountedObject<MockMediaStream>("remote_stream"));
    stream->AddTrack(MockWebRtcAudioTrack::Create(audio_track_id).get());
    stream->AddTrack(MockWebRtcVideoTrack::Create(video_track_id).get());
    return stream;
  }

  std::unique_ptr<WebRtcMediaStreamAdapter> CreateRemoteStreamAdapter(
      webrtc::MediaStreamInterface* webrtc_stream) {
    std::unique_ptr<WebRtcMediaStreamAdapter> adapter;
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &RemoteWebRtcMediaStreamAdapterTest::
                CreateRemoteStreamAdapterOnSignalingThread,
            base::Unretained(this),
            base::Unretained(
                blink::scheduler::GetSingleThreadTaskRunnerForTesting().get()),
            base::Unretained(webrtc_stream), base::Unretained(&adapter)));
    RunMessageLoopsUntilIdle();
    DCHECK(adapter);
    return adapter;
  }

  // Adds the track and gets the adapters for the stream's resulting tracks.
  template <typename TrackType>
  WebRtcMediaStreamAdapter::TrackAdapterRefs AddTrack(
      webrtc::MediaStreamInterface* webrtc_stream,
      TrackType* webrtc_track) {
    typedef bool (webrtc::MediaStreamInterface::*AddTrack)(TrackType*);
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE, base::BindOnce(base::IgnoreResult<AddTrack>(
                                      &webrtc::MediaStreamInterface::AddTrack),
                                  base::Unretained(webrtc_stream),
                                  base::Unretained(webrtc_track)));
    WebRtcMediaStreamAdapter::TrackAdapterRefs track_refs;
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&RemoteWebRtcMediaStreamAdapterTest::
                           GetStreamTrackRefsOnSignalingThread,
                       base::Unretained(this), base::Unretained(webrtc_stream),
                       base::Unretained(&track_refs)));
    RunMessageLoopsUntilIdle();
    return track_refs;
  }

  // Removes the track and gets the adapters for the stream's resulting tracks.
  template <typename TrackType>
  WebRtcMediaStreamAdapter::TrackAdapterRefs RemoveTrack(
      webrtc::MediaStreamInterface* webrtc_stream,
      TrackType* webrtc_track) {
    typedef bool (webrtc::MediaStreamInterface::*RemoveTrack)(TrackType*);
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(base::IgnoreResult<RemoveTrack>(
                           &webrtc::MediaStreamInterface::RemoveTrack),
                       base::Unretained(webrtc_stream),
                       base::Unretained(webrtc_track)));
    WebRtcMediaStreamAdapter::TrackAdapterRefs track_refs;
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&RemoteWebRtcMediaStreamAdapterTest::
                           GetStreamTrackRefsOnSignalingThread,
                       base::Unretained(this), base::Unretained(webrtc_stream),
                       base::Unretained(&track_refs)));
    RunMessageLoopsUntilIdle();
    return track_refs;
  }

 private:
  void CreateRemoteStreamAdapterOnSignalingThread(
      base::SingleThreadTaskRunner* main_thread,
      webrtc::MediaStreamInterface* webrtc_stream,
      std::unique_ptr<WebRtcMediaStreamAdapter>* adapter) {
    *adapter = WebRtcMediaStreamAdapter::CreateRemoteStreamAdapter(
        main_thread, track_adapter_map_, webrtc_stream);
    EXPECT_FALSE((*adapter)->is_initialized());
  }

  void GetStreamTrackRefsOnSignalingThread(
      webrtc::MediaStreamInterface* webrtc_stream,
      WebRtcMediaStreamAdapter::TrackAdapterRefs* out_track_refs) {
    WebRtcMediaStreamAdapter::TrackAdapterRefs track_refs;
    for (auto& audio_track : webrtc_stream->GetAudioTracks()) {
      track_refs.push_back(
          track_adapter_map_->GetOrCreateRemoteTrackAdapter(audio_track.get()));
    }
    for (auto& video_track : webrtc_stream->GetVideoTracks()) {
      track_refs.push_back(
          track_adapter_map_->GetOrCreateRemoteTrackAdapter(video_track.get()));
    }
    *out_track_refs = std::move(track_refs);
  }

  // Runs message loops on the webrtc signaling thread and the main thread until
  // idle.
  void RunMessageLoopsUntilIdle() {
    base::WaitableEvent waitable_event(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&RemoteWebRtcMediaStreamAdapterTest::SignalEvent,
                       base::Unretained(this), &waitable_event));
    // TODO(hbos): Use base::RunLoop instead of WaitableEvent.
    waitable_event.Wait();
    base::RunLoop().RunUntilIdle();
  }

  void SignalEvent(base::WaitableEvent* waitable_event) {
    waitable_event->Signal();
  }
};

TEST_F(LocalWebRtcMediaStreamAdapterTest, CreateStreamAdapter) {
  blink::WebMediaStream web_stream =
      CreateWebMediaStream("audio_track_id", "video_track_id");
  std::unique_ptr<WebRtcMediaStreamAdapter> adapter =
      WebRtcMediaStreamAdapter::CreateLocalStreamAdapter(
          dependency_factory_.get(), track_adapter_map_, web_stream);
  EXPECT_TRUE(adapter->IsEqual(web_stream));
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetAudioTracks().size());
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetVideoTracks().size());
  EXPECT_EQ(web_stream.Id().Utf8(), adapter->webrtc_stream()->id());
}

TEST_F(LocalWebRtcMediaStreamAdapterTest,
       CreateStreamAdapterWithSharedTrackIds) {
  blink::WebMediaStream web_stream =
      CreateWebMediaStream("shared_track_id", "shared_track_id");
  std::unique_ptr<WebRtcMediaStreamAdapter> adapter =
      WebRtcMediaStreamAdapter::CreateLocalStreamAdapter(
          dependency_factory_.get(), track_adapter_map_, web_stream);
  EXPECT_TRUE(adapter->IsEqual(web_stream));
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetAudioTracks().size());
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetVideoTracks().size());
  EXPECT_EQ(web_stream.Id().Utf8(), adapter->webrtc_stream()->id());
}

// It should not crash if |MediaStream| is created in blink with an unknown
// audio source. This can happen if a |MediaStream| is created with remote audio
// track.
TEST_F(LocalWebRtcMediaStreamAdapterTest,
       AdapterForWebStreamWithoutAudioSource) {
  // Create a blink MediaStream description.
  blink::WebMediaStreamSource audio_source;
  audio_source.Initialize("audio source",
                          blink::WebMediaStreamSource::kTypeAudio, "something",
                          false /* remote */);
  // Without |audio_source.SetExtraData()| it is possible to initialize tracks
  // and streams but the source is a dummy.

  blink::WebVector<blink::WebMediaStreamTrack> audio_tracks(
      static_cast<size_t>(1));
  audio_tracks[0].Initialize(audio_source.Id(), audio_source);
  blink::WebVector<blink::WebMediaStreamTrack> video_tracks(
      static_cast<size_t>(0));

  blink::WebMediaStream web_stream;
  web_stream.Initialize("stream_id", audio_tracks, video_tracks);

  std::unique_ptr<WebRtcMediaStreamAdapter> adapter =
      WebRtcMediaStreamAdapter::CreateLocalStreamAdapter(
          dependency_factory_.get(), track_adapter_map_, web_stream);
  EXPECT_TRUE(adapter->IsEqual(web_stream));
  EXPECT_EQ(0u, adapter->webrtc_stream()->GetAudioTracks().size());
  EXPECT_EQ(0u, adapter->webrtc_stream()->GetVideoTracks().size());
  EXPECT_EQ(web_stream.Id().Utf8(), adapter->webrtc_stream()->id());
}

TEST_F(LocalWebRtcMediaStreamAdapterTest, RemoveAndAddTrack) {
  blink::WebMediaStream web_stream =
      CreateWebMediaStream("audio_track_id", "video_track_id");
  std::unique_ptr<WebRtcMediaStreamAdapter> adapter =
      WebRtcMediaStreamAdapter::CreateLocalStreamAdapter(
          dependency_factory_.get(), track_adapter_map_, web_stream);
  EXPECT_TRUE(adapter->IsEqual(web_stream));
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetAudioTracks().size());
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetVideoTracks().size());
  EXPECT_EQ(web_stream.Id().Utf8(), adapter->webrtc_stream()->id());

  // Modify the web layer stream, make sure the webrtc layer stream is updated.
  blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
  web_stream.AudioTracks(audio_tracks);

  web_stream.RemoveTrack(audio_tracks[0]);
  EXPECT_TRUE(adapter->webrtc_stream()->GetAudioTracks().empty());

  blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
  web_stream.VideoTracks(video_tracks);

  web_stream.RemoveTrack(video_tracks[0]);
  EXPECT_TRUE(adapter->webrtc_stream()->GetVideoTracks().empty());

  web_stream.AddTrack(audio_tracks[0]);
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetAudioTracks().size());

  web_stream.AddTrack(video_tracks[0]);
  EXPECT_EQ(1u, adapter->webrtc_stream()->GetVideoTracks().size());
}

TEST_F(RemoteWebRtcMediaStreamAdapterTest, CreateStreamAdapter) {
  scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream =
      CreateWebRtcMediaStream("audio_track_id", "video_track_id");
  std::unique_ptr<WebRtcMediaStreamAdapter> adapter =
      CreateRemoteStreamAdapter(webrtc_stream.get());
  EXPECT_TRUE(adapter->is_initialized());
  EXPECT_EQ(webrtc_stream, adapter->webrtc_stream());
  blink::WebVector<blink::WebMediaStreamTrack> web_audio_tracks;
  adapter->web_stream().AudioTracks(web_audio_tracks);
  EXPECT_EQ(1u, web_audio_tracks.size());
  blink::WebVector<blink::WebMediaStreamTrack> web_video_tracks;
  adapter->web_stream().VideoTracks(web_video_tracks);
  EXPECT_EQ(1u, web_video_tracks.size());
}

TEST_F(RemoteWebRtcMediaStreamAdapterTest,
       CreateStreamAdapterWithSharedTrackIds) {
  scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream =
      CreateWebRtcMediaStream("shared_track_id", "shared_track_id");
  std::unique_ptr<WebRtcMediaStreamAdapter> adapter =
      CreateRemoteStreamAdapter(webrtc_stream.get());
  EXPECT_TRUE(adapter->is_initialized());
  EXPECT_EQ(webrtc_stream, adapter->webrtc_stream());
  blink::WebVector<blink::WebMediaStreamTrack> web_audio_tracks;
  adapter->web_stream().AudioTracks(web_audio_tracks);
  EXPECT_EQ(1u, web_audio_tracks.size());
  blink::WebVector<blink::WebMediaStreamTrack> web_video_tracks;
  adapter->web_stream().VideoTracks(web_video_tracks);
  EXPECT_EQ(1u, web_video_tracks.size());
}

TEST_F(RemoteWebRtcMediaStreamAdapterTest, RemoveAndAddTrack) {
  scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream =
      CreateWebRtcMediaStream("audio_track_id", "video_track_id");
  std::unique_ptr<WebRtcMediaStreamAdapter> adapter =
      CreateRemoteStreamAdapter(webrtc_stream.get());
  EXPECT_TRUE(adapter->is_initialized());
  EXPECT_EQ(webrtc_stream, adapter->webrtc_stream());
  blink::WebVector<blink::WebMediaStreamTrack> web_audio_tracks;
  adapter->web_stream().AudioTracks(web_audio_tracks);
  EXPECT_EQ(1u, web_audio_tracks.size());
  blink::WebVector<blink::WebMediaStreamTrack> web_video_tracks;
  adapter->web_stream().VideoTracks(web_video_tracks);
  EXPECT_EQ(1u, web_video_tracks.size());

  // Modify the webrtc layer stream, make sure the web layer stream is updated.
  rtc::scoped_refptr<webrtc::AudioTrackInterface> webrtc_audio_track =
      webrtc_stream->GetAudioTracks()[0];
  adapter->SetTracks(
      RemoveTrack(webrtc_stream.get(), webrtc_audio_track.get()));
  adapter->web_stream().AudioTracks(web_audio_tracks);
  EXPECT_EQ(0u, web_audio_tracks.size());

  rtc::scoped_refptr<webrtc::VideoTrackInterface> webrtc_video_track =
      webrtc_stream->GetVideoTracks()[0];
  adapter->SetTracks(
      RemoveTrack(webrtc_stream.get(), webrtc_video_track.get()));
  adapter->web_stream().VideoTracks(web_video_tracks);
  EXPECT_EQ(0u, web_video_tracks.size());

  adapter->SetTracks(AddTrack(webrtc_stream.get(), webrtc_audio_track.get()));
  adapter->web_stream().AudioTracks(web_audio_tracks);
  EXPECT_EQ(1u, web_audio_tracks.size());

  adapter->SetTracks(AddTrack(webrtc_stream.get(), webrtc_video_track.get()));
  adapter->web_stream().VideoTracks(web_video_tracks);
  EXPECT_EQ(1u, web_video_tracks.size());
}

}  // namespace content
