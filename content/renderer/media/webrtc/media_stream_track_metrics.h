// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_MEDIA_STREAM_TRACK_METRICS_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_MEDIA_STREAM_TRACK_METRICS_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/sequence_checker.h"
#include "content/common/content_export.h"
#include "content/common/media/media_stream.mojom.h"
#include "third_party/webrtc/api/peerconnectioninterface.h"

namespace webrtc {
class MediaStreamInterface;
}

namespace content {

class MediaStreamTrackMetricsObserver;
class RTCPeerConnectionHandler;

// Responsible for observing the connected lifetimes of tracks going
// over a PeerConnection, and sending messages to the browser process
// about lifetime events.
//
// There should be exactly one of these objects owned by each
// RTCPeerConnectionHandler, and its lifetime should match the
// lifetime of its owner.
class CONTENT_EXPORT MediaStreamTrackMetrics {
 public:
  explicit MediaStreamTrackMetrics();
  ~MediaStreamTrackMetrics();

  enum StreamType { SENT_STREAM, RECEIVED_STREAM };

  enum TrackType { AUDIO_TRACK, VIDEO_TRACK };

  enum LifetimeEvent { CONNECTED, DISCONNECTED };

  // Starts tracking lifetimes of all the tracks in |stream| and any
  // tracks added or removed to/from the stream until |RemoveStream|
  // is called or this object's lifetime ends.
  void AddStream(StreamType type, webrtc::MediaStreamInterface* stream);

  // Stops tracking lifetimes of tracks in |stream|.
  void RemoveStream(StreamType type, webrtc::MediaStreamInterface* stream);

  // Called to indicate changes in the ICE connection state for the
  // PeerConnection this object is associated with. Used to generate
  // the connected/disconnected lifetime events for these tracks.
  void IceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state);

  // Send a lifetime message to the browser process. Virtual so that
  // it can be overridden in unit tests.
  //
  // |track_id| is the ID of the track that just got connected or
  // disconnected.
  //
  // |is_audio| is true for an audio track, false for a video track.
  //
  // |start_lifetime| is true to indicate that it just got connected,
  // false to indicate it is no longer connected.
  //
  // |is_remote| is true for remote streams (received over a
  // PeerConnection), false for local streams (sent over a
  // PeerConnection).
  virtual void SendLifetimeMessage(const std::string& track_id,
                                   TrackType track_type,
                                   LifetimeEvent lifetime_event,
                                   StreamType stream_type);

 protected:
  // Calls SendLifetimeMessage for |observer| depending on |ice_state_|.
  void SendLifeTimeMessageDependingOnIceState(
      MediaStreamTrackMetricsObserver* observer);

  // Implements MakeUniqueId. |pc_id| is a cast of this object's
  // |this| pointer to a 64-bit integer, which is usable as a unique
  // ID for the PeerConnection this object is attached to (since there
  // is a one-to-one relationship).
  uint64_t MakeUniqueIdImpl(uint64_t pc_id,
                            const std::string& track,
                            StreamType stream_type);

 private:
  // Make a unique ID for the given track, that is valid while the
  // track object and the PeerConnection it is attached to both exist.
  uint64_t MakeUniqueId(const std::string& track, StreamType stream_type);

  mojom::MediaStreamTrackMetricsHostPtr& GetMediaStreamTrackMetricsHost();

  mojom::MediaStreamTrackMetricsHostPtr track_metrics_host_;

  typedef std::vector<std::unique_ptr<MediaStreamTrackMetricsObserver>>
      ObserverVector;
  ObserverVector observers_;

  webrtc::PeerConnectionInterface::IceConnectionState ice_state_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(MediaStreamTrackMetrics);
};

}  // namespace

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_MEDIA_STREAM_TRACK_METRICS_H_
