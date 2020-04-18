// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "content/renderer/media/webrtc/media_stream_track_metrics.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/api/mediastreaminterface.h"

using webrtc::AudioSourceInterface;
using webrtc::AudioTrackInterface;
using webrtc::AudioTrackSinkInterface;
using webrtc::MediaStreamInterface;
using webrtc::ObserverInterface;
using webrtc::PeerConnectionInterface;
using webrtc::VideoTrackSourceInterface;
using webrtc::VideoTrackInterface;

namespace content {

// A very simple mock that implements only the id() method.
class MockAudioTrackInterface : public AudioTrackInterface {
 public:
  explicit MockAudioTrackInterface(const std::string& id) : id_(id) {}
  ~MockAudioTrackInterface() override {}

  std::string id() const override { return id_; }

  MOCK_METHOD1(RegisterObserver, void(ObserverInterface*));
  MOCK_METHOD1(UnregisterObserver, void(ObserverInterface*));
  MOCK_CONST_METHOD0(kind, std::string());
  MOCK_CONST_METHOD0(enabled, bool());
  MOCK_CONST_METHOD0(state, TrackState());
  MOCK_METHOD1(set_enabled, bool(bool));
  MOCK_METHOD1(set_state, bool(TrackState));
  MOCK_CONST_METHOD0(GetSource, AudioSourceInterface*());
  MOCK_METHOD1(AddSink, void(AudioTrackSinkInterface*));
  MOCK_METHOD1(RemoveSink, void(AudioTrackSinkInterface*));

 private:
  std::string id_;
};

// A very simple mock that implements only the id() method.
class MockVideoTrackInterface : public VideoTrackInterface {
 public:
  explicit MockVideoTrackInterface(const std::string& id) : id_(id) {}
  ~MockVideoTrackInterface() override {}

  std::string id() const override { return id_; }

  MOCK_METHOD1(RegisterObserver, void(ObserverInterface*));
  MOCK_METHOD1(UnregisterObserver, void(ObserverInterface*));
  MOCK_CONST_METHOD0(kind, std::string());
  MOCK_CONST_METHOD0(enabled, bool());
  MOCK_CONST_METHOD0(state, TrackState());
  MOCK_METHOD1(set_enabled, bool(bool));
  MOCK_METHOD1(set_state, bool(TrackState));
  MOCK_METHOD2(AddOrUpdateSink,
               void(rtc::VideoSinkInterface<webrtc::VideoFrame>*,
                    const rtc::VideoSinkWants&));
  MOCK_METHOD1(RemoveSink, void(rtc::VideoSinkInterface<webrtc::VideoFrame>*));
  MOCK_CONST_METHOD0(GetSource, VideoTrackSourceInterface*());

 private:
  std::string id_;
};

class MockMediaStreamTrackMetrics : public MediaStreamTrackMetrics {
 public:
  virtual ~MockMediaStreamTrackMetrics() {}

  MOCK_METHOD4(SendLifetimeMessage,
               void(const std::string&, TrackType, LifetimeEvent, StreamType));

  using MediaStreamTrackMetrics::MakeUniqueIdImpl;
};

class MediaStreamTrackMetricsTest : public testing::Test {
 public:
  MediaStreamTrackMetricsTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        signaling_thread_("signaling_thread") {}

  void SetUp() override {
    metrics_.reset(new MockMediaStreamTrackMetrics());
    stream_ = new rtc::RefCountedObject<MockMediaStream>("stream");
    signaling_thread_.Start();
  }

  void TearDown() override {
    signaling_thread_.Stop();
    metrics_.reset();
    stream_ = nullptr;
  }

  // Adds an audio track to |stream_| on the signaling thread to simulate how
  // notifications will be fired in Chrome.
  template <typename TrackType>
  void AddTrack(TrackType* track) {
    // Explicitly casting to this type is necessary since the
    // MediaStreamInterface has two methods with the same name.
    typedef bool (MediaStreamInterface::*AddTrack)(TrackType*);
    base::RunLoop run_loop;
    signaling_thread_.task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(
            base::IgnoreResult<AddTrack>(&MediaStreamInterface::AddTrack),
            stream_, base::Unretained(track)),
        run_loop.QuitClosure());
    run_loop.Run();
  }

  template <typename TrackType>
  void RemoveTrack(TrackType* track) {
    // Explicitly casting to this type is necessary since the
    // MediaStreamInterface has two methods with the same name.
    typedef bool (MediaStreamInterface::*RemoveTrack)(TrackType*);
    base::RunLoop run_loop;
    signaling_thread_.task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(
            base::IgnoreResult<RemoveTrack>(&MediaStreamInterface::RemoveTrack),
            stream_, base::Unretained(track)),
        run_loop.QuitClosure());
    run_loop.Run();
  }

  // Convenience methods to cast the mock track types into their webrtc
  // equivalents.
  void AddAudioTrack(AudioTrackInterface* track) { AddTrack(track); }
  void RemoveAudioTrack(AudioTrackInterface* track) { RemoveTrack(track); }
  void AddVideoTrack(VideoTrackInterface* track) { AddTrack(track); }
  void RemoveVideoTrack(VideoTrackInterface* track) { RemoveTrack(track); }

  scoped_refptr<MockAudioTrackInterface> MakeAudioTrack(const std::string& id) {
    return new rtc::RefCountedObject<MockAudioTrackInterface>(id);
  }

  scoped_refptr<MockVideoTrackInterface> MakeVideoTrack(const std::string& id) {
    return new rtc::RefCountedObject<MockVideoTrackInterface>(id);
  }

  std::unique_ptr<MockMediaStreamTrackMetrics> metrics_;
  scoped_refptr<MediaStreamInterface> stream_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::Thread signaling_thread_;
};

TEST_F(MediaStreamTrackMetricsTest, MakeUniqueId) {
  // The important testable properties of the unique ID are that it
  // should differ when any of the three constituents differ
  // (PeerConnection pointer, track ID, remote or not. Also, testing
  // that the implementation does not discard the upper 32 bits of the
  // PeerConnection pointer is important.
  //
  // The important hard-to-test property is that the ID be generated
  // using a hash function with virtually zero chance of
  // collisions. We don't test this, we rely on MD5 having this
  // property.

  // Lower 32 bits the same, upper 32 differ.
  EXPECT_NE(
      metrics_->MakeUniqueIdImpl(
          0x1000000000000001, "x", MediaStreamTrackMetrics::RECEIVED_STREAM),
      metrics_->MakeUniqueIdImpl(
          0x2000000000000001, "x", MediaStreamTrackMetrics::RECEIVED_STREAM));

  // Track ID differs.
  EXPECT_NE(metrics_->MakeUniqueIdImpl(
                42, "x", MediaStreamTrackMetrics::RECEIVED_STREAM),
            metrics_->MakeUniqueIdImpl(
                42, "y", MediaStreamTrackMetrics::RECEIVED_STREAM));

  // Remove vs. local track differs.
  EXPECT_NE(metrics_->MakeUniqueIdImpl(
                42, "x", MediaStreamTrackMetrics::RECEIVED_STREAM),
            metrics_->MakeUniqueIdImpl(
                42, "x", MediaStreamTrackMetrics::SENT_STREAM));
}

TEST_F(MediaStreamTrackMetricsTest, BasicRemoteStreams) {
  scoped_refptr<MockAudioTrackInterface> audio(MakeAudioTrack("audio"));
  scoped_refptr<MockVideoTrackInterface> video(MakeVideoTrack("video"));
  stream_->AddTrack(audio.get());
  stream_->AddTrack(video.get());
  metrics_->AddStream(MediaStreamTrackMetrics::RECEIVED_STREAM, stream_.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionDisconnected);
}

TEST_F(MediaStreamTrackMetricsTest, BasicLocalStreams) {
  scoped_refptr<MockAudioTrackInterface> audio(MakeAudioTrack("audio"));
  scoped_refptr<MockVideoTrackInterface> video(MakeVideoTrack("video"));
  stream_->AddTrack(audio.get());
  stream_->AddTrack(video.get());
  metrics_->AddStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamAddedAferIceConnect) {
  metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));

  scoped_refptr<MockAudioTrackInterface> audio(MakeAudioTrack("audio"));
  scoped_refptr<MockVideoTrackInterface> video(MakeVideoTrack("video"));
  stream_->AddTrack(audio.get());
  stream_->AddTrack(video.get());
  metrics_->AddStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());
}

TEST_F(MediaStreamTrackMetricsTest, RemoteStreamAddedAferIceConnect) {
  metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));

  scoped_refptr<MockAudioTrackInterface> audio(MakeAudioTrack("audio"));
  scoped_refptr<MockVideoTrackInterface> video(MakeVideoTrack("video"));
  stream_->AddTrack(audio.get());
  stream_->AddTrack(video.get());
  metrics_->AddStream(MediaStreamTrackMetrics::RECEIVED_STREAM, stream_.get());
}

TEST_F(MediaStreamTrackMetricsTest, RemoteStreamTrackAdded) {
  scoped_refptr<MockAudioTrackInterface> initial(MakeAudioTrack("initial"));
  scoped_refptr<MockAudioTrackInterface> added(MakeAudioTrack("added"));
  stream_->AddTrack(initial.get());
  metrics_->AddStream(MediaStreamTrackMetrics::RECEIVED_STREAM, stream_.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("initial",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("added",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  AddAudioTrack(added.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("initial",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("added",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamTrackRemoved) {
  scoped_refptr<MockAudioTrackInterface> first(MakeAudioTrack("first"));
  scoped_refptr<MockAudioTrackInterface> second(MakeAudioTrack("second"));
  stream_->AddTrack(first.get());
  stream_->AddTrack(second.get());
  metrics_->AddStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("first",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("second",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("first",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  stream_->RemoveTrack(first.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("second",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamModificationsBeforeAndAfter) {
  scoped_refptr<MockAudioTrackInterface> first(MakeAudioTrack("first"));
  scoped_refptr<MockAudioTrackInterface> second(MakeAudioTrack("second"));
  stream_->AddTrack(first.get());
  metrics_->AddStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());

  // This gets added after we start observing, but no lifetime message
  // should be sent at this point since the call is not connected. It
  // should get sent only once it gets connected.
  AddAudioTrack(second.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("first",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("second",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("first",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("second",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);

  // This happens after the call is disconnected so no lifetime
  // message should be sent.
  RemoveAudioTrack(first.get());
}

TEST_F(MediaStreamTrackMetricsTest, RemoteStreamMultipleDisconnects) {
  scoped_refptr<MockAudioTrackInterface> audio(MakeAudioTrack("audio"));
  stream_->AddTrack(audio.get());
  metrics_->AddStream(MediaStreamTrackMetrics::RECEIVED_STREAM, stream_.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::RECEIVED_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionDisconnected);
  metrics_->IceConnectionChange(PeerConnectionInterface::kIceConnectionFailed);
  RemoveAudioTrack(audio.get());
}

TEST_F(MediaStreamTrackMetricsTest, RemoteStreamConnectDisconnectTwice) {
  scoped_refptr<MockAudioTrackInterface> audio(MakeAudioTrack("audio"));
  stream_->AddTrack(audio.get());
  metrics_->AddStream(MediaStreamTrackMetrics::RECEIVED_STREAM, stream_.get());

  for (size_t i = 0; i < 2; ++i) {
    EXPECT_CALL(*metrics_,
                SendLifetimeMessage("audio",
                                    MediaStreamTrackMetrics::AUDIO_TRACK,
                                    MediaStreamTrackMetrics::CONNECTED,
                                    MediaStreamTrackMetrics::RECEIVED_STREAM));
    metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionConnected);

    EXPECT_CALL(*metrics_,
                SendLifetimeMessage("audio",
                                    MediaStreamTrackMetrics::AUDIO_TRACK,
                                    MediaStreamTrackMetrics::DISCONNECTED,
                                    MediaStreamTrackMetrics::RECEIVED_STREAM));
    metrics_->IceConnectionChange(
        PeerConnectionInterface::kIceConnectionDisconnected);
  }

  RemoveAudioTrack(audio.get());
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamRemovedNoDisconnect) {
  scoped_refptr<MockAudioTrackInterface> audio(MakeAudioTrack("audio"));
  scoped_refptr<MockVideoTrackInterface> video(MakeVideoTrack("video"));
  stream_->AddTrack(audio.get());
  stream_->AddTrack(video.get());
  metrics_->AddStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->RemoveStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());
}

TEST_F(MediaStreamTrackMetricsTest, LocalStreamLargerTest) {
  scoped_refptr<MockAudioTrackInterface> audio1(MakeAudioTrack("audio1"));
  scoped_refptr<MockAudioTrackInterface> audio2(MakeAudioTrack("audio2"));
  scoped_refptr<MockAudioTrackInterface> audio3(MakeAudioTrack("audio3"));
  scoped_refptr<MockVideoTrackInterface> video1(MakeVideoTrack("video1"));
  scoped_refptr<MockVideoTrackInterface> video2(MakeVideoTrack("video2"));
  scoped_refptr<MockVideoTrackInterface> video3(MakeVideoTrack("video3"));
  stream_->AddTrack(audio1.get());
  stream_->AddTrack(video1.get());
  metrics_->AddStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio1",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video1",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->IceConnectionChange(
      PeerConnectionInterface::kIceConnectionConnected);

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio2",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  AddAudioTrack(audio2.get());
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video2",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  AddVideoTrack(video2.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio1",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  RemoveAudioTrack(audio1.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio3",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  AddAudioTrack(audio3.get());
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video3",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  AddVideoTrack(video3.get());

  // Add back audio1
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio1",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::CONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  AddAudioTrack(audio1.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio2",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  RemoveAudioTrack(audio2.get());
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video2",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  RemoveVideoTrack(video2.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio1",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  RemoveAudioTrack(audio1.get());
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video1",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  RemoveVideoTrack(video1.get());

  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("audio3",
                                  MediaStreamTrackMetrics::AUDIO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  EXPECT_CALL(*metrics_,
              SendLifetimeMessage("video3",
                                  MediaStreamTrackMetrics::VIDEO_TRACK,
                                  MediaStreamTrackMetrics::DISCONNECTED,
                                  MediaStreamTrackMetrics::SENT_STREAM));
  metrics_->RemoveStream(MediaStreamTrackMetrics::SENT_STREAM, stream_.get());
}

}  // namespace content
