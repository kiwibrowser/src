// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_rtc_peer_connection_handler.h"
#include "third_party/blink/public/platform/web_rtc_rtp_receiver.h"
#include "third_party/blink/public/platform/web_rtc_rtp_sender.h"
#include "third_party/blink/public/platform/web_rtc_session_description.h"
#include "third_party/blink/public/web/web_heap.h"
#include "third_party/blink/renderer/bindings/core/v8/dictionary.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_track.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_configuration.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_ice_server.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_web_rtc.h"

namespace blink {

class RTCPeerConnectionTest : public testing::Test {
 public:
  RTCPeerConnection* CreatePC(V8TestingScope& scope) {
    RTCConfiguration config;
    RTCIceServer ice_server;
    ice_server.setURL("stun:fake.stun.url");
    HeapVector<RTCIceServer> ice_servers;
    ice_servers.push_back(ice_server);
    config.setIceServers(ice_servers);
    return RTCPeerConnection::Create(scope.GetExecutionContext(), config,
                                     Dictionary(), scope.GetExceptionState());
  }

  MediaStreamTrack* CreateTrack(V8TestingScope& scope,
                                MediaStreamSource::StreamType type,
                                String id) {
    MediaStreamSource* source =
        MediaStreamSource::Create("sourceId", type, "sourceName", false);
    MediaStreamComponent* component = MediaStreamComponent::Create(id, source);
    return MediaStreamTrack::Create(scope.GetExecutionContext(), component);
  }

  std::string GetExceptionMessage(V8TestingScope& scope) {
    ExceptionState& exception_state = scope.GetExceptionState();
    return exception_state.HadException()
               ? exception_state.Message().Utf8().data()
               : "";
  }

  void AddStream(V8TestingScope& scope,
                 RTCPeerConnection* pc,
                 MediaStream* stream) {
    pc->addStream(scope.GetScriptState(), stream, Dictionary(),
                  scope.GetExceptionState());
    EXPECT_EQ("", GetExceptionMessage(scope));
  }

  void RemoveStream(V8TestingScope& scope,
                    RTCPeerConnection* pc,
                    MediaStream* stream) {
    pc->removeStream(stream, scope.GetExceptionState());
    EXPECT_EQ("", GetExceptionMessage(scope));
  }

 private:
  ScopedTestingPlatformSupport<TestingPlatformSupportWithWebRTC> platform;
};

TEST_F(RTCPeerConnectionTest, GetAudioTrack) {
  V8TestingScope scope;
  RTCPeerConnection* pc = CreatePC(scope);
  EXPECT_EQ("", GetExceptionMessage(scope));
  ASSERT_TRUE(pc);

  MediaStreamTrack* track =
      CreateTrack(scope, MediaStreamSource::kTypeAudio, "audioTrack");
  HeapVector<Member<MediaStreamTrack>> tracks;
  tracks.push_back(track);
  MediaStream* stream =
      MediaStream::Create(scope.GetExecutionContext(), tracks);
  ASSERT_TRUE(stream);

  EXPECT_FALSE(pc->GetTrack(track->Component()));
  AddStream(scope, pc, stream);
  EXPECT_TRUE(pc->GetTrack(track->Component()));
}

TEST_F(RTCPeerConnectionTest, GetVideoTrack) {
  V8TestingScope scope;
  RTCPeerConnection* pc = CreatePC(scope);
  EXPECT_EQ("", GetExceptionMessage(scope));
  ASSERT_TRUE(pc);

  MediaStreamTrack* track =
      CreateTrack(scope, MediaStreamSource::kTypeVideo, "videoTrack");
  HeapVector<Member<MediaStreamTrack>> tracks;
  tracks.push_back(track);
  MediaStream* stream =
      MediaStream::Create(scope.GetExecutionContext(), tracks);
  ASSERT_TRUE(stream);

  EXPECT_FALSE(pc->GetTrack(track->Component()));
  AddStream(scope, pc, stream);
  EXPECT_TRUE(pc->GetTrack(track->Component()));
}

TEST_F(RTCPeerConnectionTest, GetAudioAndVideoTrack) {
  V8TestingScope scope;
  RTCPeerConnection* pc = CreatePC(scope);
  EXPECT_EQ("", GetExceptionMessage(scope));
  ASSERT_TRUE(pc);

  HeapVector<Member<MediaStreamTrack>> tracks;
  MediaStreamTrack* audio_track =
      CreateTrack(scope, MediaStreamSource::kTypeAudio, "audioTrack");
  tracks.push_back(audio_track);
  MediaStreamTrack* video_track =
      CreateTrack(scope, MediaStreamSource::kTypeVideo, "videoTrack");
  tracks.push_back(video_track);

  MediaStream* stream =
      MediaStream::Create(scope.GetExecutionContext(), tracks);
  ASSERT_TRUE(stream);

  EXPECT_FALSE(pc->GetTrack(audio_track->Component()));
  EXPECT_FALSE(pc->GetTrack(video_track->Component()));
  AddStream(scope, pc, stream);
  EXPECT_TRUE(pc->GetTrack(audio_track->Component()));
  EXPECT_TRUE(pc->GetTrack(video_track->Component()));
}

TEST_F(RTCPeerConnectionTest, GetTrackRemoveStreamAndGCAll) {
  V8TestingScope scope;
  Persistent<RTCPeerConnection> pc = CreatePC(scope);
  EXPECT_EQ("", GetExceptionMessage(scope));
  ASSERT_TRUE(pc);

  MediaStreamTrack* track =
      CreateTrack(scope, MediaStreamSource::kTypeAudio, "audioTrack");
  HeapVector<Member<MediaStreamTrack>> tracks;
  tracks.push_back(track);
  MediaStream* stream =
      MediaStream::Create(scope.GetExecutionContext(), tracks);
  ASSERT_TRUE(stream);

  MediaStreamComponent* track_component = track->Component();

  EXPECT_FALSE(pc->GetTrack(track_component));
  AddStream(scope, pc, stream);
  EXPECT_TRUE(pc->GetTrack(track_component));

  RemoveStream(scope, pc, stream);
  // This will destroy |MediaStream|, |MediaStreamTrack| and its
  // |MediaStreamComponent|, which will remove its mapping from the peer
  // connection.
  WebHeap::CollectAllGarbageForTesting();
  EXPECT_FALSE(pc->GetTrack(track_component));
}

TEST_F(RTCPeerConnectionTest,
       GetTrackRemoveStreamAndGCWithPersistentComponent) {
  V8TestingScope scope;
  Persistent<RTCPeerConnection> pc = CreatePC(scope);
  EXPECT_EQ("", GetExceptionMessage(scope));
  ASSERT_TRUE(pc);

  MediaStreamTrack* track =
      CreateTrack(scope, MediaStreamSource::kTypeAudio, "audioTrack");
  HeapVector<Member<MediaStreamTrack>> tracks;
  tracks.push_back(track);
  MediaStream* stream =
      MediaStream::Create(scope.GetExecutionContext(), tracks);
  ASSERT_TRUE(stream);

  Persistent<MediaStreamComponent> track_component = track->Component();

  EXPECT_FALSE(pc->GetTrack(track_component.Get()));
  AddStream(scope, pc, stream);
  EXPECT_TRUE(pc->GetTrack(track_component.Get()));

  RemoveStream(scope, pc, stream);
  // This will destroy |MediaStream| and |MediaStreamTrack| (but not
  // |MediaStreamComponent|), which will remove its mapping from the peer
  // connection.
  WebHeap::CollectAllGarbageForTesting();
  EXPECT_FALSE(pc->GetTrack(track_component.Get()));
}

TEST_F(RTCPeerConnectionTest, GetTrackRemoveStreamAndGCWithPersistentStream) {
  V8TestingScope scope;
  Persistent<RTCPeerConnection> pc = CreatePC(scope);
  EXPECT_EQ("", GetExceptionMessage(scope));
  ASSERT_TRUE(pc);

  MediaStreamTrack* track =
      CreateTrack(scope, MediaStreamSource::kTypeAudio, "audioTrack");
  HeapVector<Member<MediaStreamTrack>> tracks;
  tracks.push_back(track);
  Persistent<MediaStream> stream =
      MediaStream::Create(scope.GetExecutionContext(), tracks);
  ASSERT_TRUE(stream);

  MediaStreamComponent* track_component = track->Component();

  EXPECT_FALSE(pc->GetTrack(track_component));
  AddStream(scope, pc, stream);
  EXPECT_TRUE(pc->GetTrack(track_component));

  RemoveStream(scope, pc, stream);
  // With a persistent |MediaStream|, the |MediaStreamTrack| and
  // |MediaStreamComponent| will not be destroyed and continue to be mapped by
  // peer connection.
  WebHeap::CollectAllGarbageForTesting();
  EXPECT_TRUE(pc->GetTrack(track_component));

  stream = nullptr;
  // Now |MediaStream|, |MediaStreamTrack| and |MediaStreamComponent| will be
  // destroyed and the mapping removed from the peer connection.
  WebHeap::CollectAllGarbageForTesting();
  EXPECT_FALSE(pc->GetTrack(track_component));
}

}  // namespace blink
