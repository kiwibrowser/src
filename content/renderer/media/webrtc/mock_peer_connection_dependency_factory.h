// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_PEER_CONNECTION_DEPENDENCY_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_PEER_CONNECTION_DEPENDENCY_FACTORY_H_

#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "content/renderer/media/webrtc/peer_connection_dependency_factory.h"
#include "third_party/webrtc/api/mediaconstraintsinterface.h"
#include "third_party/webrtc/api/mediastreaminterface.h"

namespace content {

typedef std::set<webrtc::ObserverInterface*> ObserverSet;

class MockWebRtcAudioSource : public webrtc::AudioSourceInterface {
 public:
  MockWebRtcAudioSource(bool is_remote);
  void RegisterObserver(webrtc::ObserverInterface* observer) override;
  void UnregisterObserver(webrtc::ObserverInterface* observer) override;

  SourceState state() const override;
  bool remote() const override;

 private:
  const bool is_remote_;
};

class MockWebRtcAudioTrack : public webrtc::AudioTrackInterface {
 public:
  static scoped_refptr<MockWebRtcAudioTrack> Create(const std::string& id);

  void AddSink(webrtc::AudioTrackSinkInterface* sink) override {}
  void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override {}
  webrtc::AudioSourceInterface* GetSource() const override;

  std::string kind() const override;
  std::string id() const override;
  bool enabled() const override;
  webrtc::MediaStreamTrackInterface::TrackState state() const override;
  bool set_enabled(bool enable) override;

  void RegisterObserver(webrtc::ObserverInterface* observer) override;
  void UnregisterObserver(webrtc::ObserverInterface* observer) override;

  void SetEnded();

 protected:
  MockWebRtcAudioTrack(const std::string& id);
  ~MockWebRtcAudioTrack() override;

 private:
  std::string id_;
  scoped_refptr<webrtc::AudioSourceInterface> source_;
  bool enabled_;
  TrackState state_;
  ObserverSet observers_;
};

class MockWebRtcVideoTrack : public webrtc::VideoTrackInterface {
 public:
  static scoped_refptr<MockWebRtcVideoTrack> Create(const std::string& id);
  MockWebRtcVideoTrack(const std::string& id,
                       webrtc::VideoTrackSourceInterface* source);
  void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
  webrtc::VideoTrackSourceInterface* GetSource() const override;

  std::string kind() const override;
  std::string id() const override;
  bool enabled() const override;
  webrtc::MediaStreamTrackInterface::TrackState state() const override;
  bool set_enabled(bool enable) override;

  void RegisterObserver(webrtc::ObserverInterface* observer) override;
  void UnregisterObserver(webrtc::ObserverInterface* observer) override;

  void SetEnded();

 protected:
  ~MockWebRtcVideoTrack() override;

 private:
  std::string id_;
  scoped_refptr<webrtc::VideoTrackSourceInterface> source_;
  bool enabled_;
  TrackState state_;
  ObserverSet observers_;
  rtc::VideoSinkInterface<webrtc::VideoFrame>* sink_;
};

class MockMediaStream : public webrtc::MediaStreamInterface {
 public:
  explicit MockMediaStream(const std::string& id);

  bool AddTrack(webrtc::AudioTrackInterface* track) override;
  bool AddTrack(webrtc::VideoTrackInterface* track) override;
  bool RemoveTrack(webrtc::AudioTrackInterface* track) override;
  bool RemoveTrack(webrtc::VideoTrackInterface* track) override;
  std::string id() const override;
  webrtc::AudioTrackVector GetAudioTracks() override;
  webrtc::VideoTrackVector GetVideoTracks() override;
  rtc::scoped_refptr<webrtc::AudioTrackInterface> FindAudioTrack(
      const std::string& track_id) override;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> FindVideoTrack(
      const std::string& track_id) override;
  void RegisterObserver(webrtc::ObserverInterface* observer) override;
  void UnregisterObserver(webrtc::ObserverInterface* observer) override;

 protected:
  ~MockMediaStream() override;

 private:
  void NotifyObservers();

  std::string id_;
  webrtc::AudioTrackVector audio_track_vector_;
  webrtc::VideoTrackVector video_track_vector_;

  ObserverSet observers_;
};

// A mock factory for creating different objects for
// RTC PeerConnections.
class MockPeerConnectionDependencyFactory
     : public PeerConnectionDependencyFactory {
 public:
  MockPeerConnectionDependencyFactory();
  ~MockPeerConnectionDependencyFactory() override;

  scoped_refptr<webrtc::PeerConnectionInterface> CreatePeerConnection(
      const webrtc::PeerConnectionInterface::RTCConfiguration& config,
      blink::WebLocalFrame* frame,
      webrtc::PeerConnectionObserver* observer) override;
  scoped_refptr<webrtc::VideoTrackSourceInterface> CreateVideoTrackSourceProxy(
      webrtc::VideoTrackSourceInterface* source) override;
  scoped_refptr<webrtc::MediaStreamInterface> CreateLocalMediaStream(
      const std::string& label) override;
  scoped_refptr<webrtc::VideoTrackInterface> CreateLocalVideoTrack(
      const std::string& id,
      webrtc::VideoTrackSourceInterface* source) override;
  webrtc::SessionDescriptionInterface* CreateSessionDescription(
      const std::string& type,
      const std::string& sdp,
      webrtc::SdpParseError* error) override;
  webrtc::IceCandidateInterface* CreateIceCandidate(
      const std::string& sdp_mid,
      int sdp_mline_index,
      const std::string& sdp) override;

  scoped_refptr<base::SingleThreadTaskRunner> GetWebRtcSignalingThread()
      const override;

  // If |fail| is true, subsequent calls to CreateSessionDescription will
  // return nullptr. This can be used to fake a blob of SDP that fails to be
  // parsed.
  void SetFailToCreateSessionDescription(bool fail);

 private:
  base::Thread signaling_thread_;
  bool fail_to_create_session_description_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockPeerConnectionDependencyFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_PEER_CONNECTION_DEPENDENCY_FACTORY_H_
