// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_MEDIA_STREAM_ADAPTER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_MEDIA_STREAM_ADAPTER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/webrtc/api/mediastreaminterface.h"

namespace content {

class PeerConnectionDependencyFactory;

// An adapter is the glue between webrtc and blink media streams. Adapters of
// local media streams are created from a blink stream and get a webrtc stream
// correspondent. Adapters of remote media streams are created from a webrtc
// stream and get a blink stream correspondent. The adapter makes sure that when
// the stream it was created from is modified the correspondent is updated to
// reflect the new state.
// The adapters are thread safe but must be constructed on the correct thread
// (local adapters: main thread, remote adapters: webrtc signaling thread) and
// destroyed on the main thread.
class CONTENT_EXPORT WebRtcMediaStreamAdapter {
 public:
  using TrackAdapterRefs = std::vector<
      std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>>;

  // Creates an adapter for a local media stream. The adapter is already
  // initialized. Invoked on the main thread.
  static std::unique_ptr<WebRtcMediaStreamAdapter> CreateLocalStreamAdapter(
      PeerConnectionDependencyFactory* factory,
      scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
      const blink::WebMediaStream& web_stream);
  // Creates an adapter for a remote media stream. Invoked on the webrtc
  // signaling thread. The adapter is initialized in a post to the main thread.
  static std::unique_ptr<WebRtcMediaStreamAdapter> CreateRemoteStreamAdapter(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
      scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream);
  // Invoke on the main thread.
  virtual ~WebRtcMediaStreamAdapter();

  // Once initialized, |webrtc_stream| and |web_stream| are accessible. An
  // initialized adapter remains initialized until destroyed.
  virtual bool is_initialized() const = 0;
  virtual const scoped_refptr<webrtc::MediaStreamInterface>& webrtc_stream()
      const = 0;
  virtual const blink::WebMediaStream& web_stream() const = 0;
  bool IsEqual(const blink::WebMediaStream& stream) const {
    return (web_stream_.IsNull() && stream.IsNull()) ||
           (web_stream_.Id() == stream.Id());
  }

  virtual void SetTracks(TrackAdapterRefs track_refs) = 0;

 protected:
  WebRtcMediaStreamAdapter(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
      scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream,
      const blink::WebMediaStream& web_stream);

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  // The map and owner of all track adapters for the associated peer connection.
  // When a track is added or removed from this stream, the map provides us with
  // a reference to the corresponding track adapter, creating a new one if
  // necessary.
  scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map_;

  // If |is_initialized()| both of these need to be set and remain constant.
  scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream_;
  blink::WebMediaStream web_stream_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcMediaStreamAdapter);
};

// Adapter implementation for a local |blink::WebMediaStream|. Created and
// destroyed on the main thread.
class LocalWebRtcMediaStreamAdapter : public WebRtcMediaStreamAdapter,
                                      blink::WebMediaStreamObserver {
 public:
  LocalWebRtcMediaStreamAdapter(
      PeerConnectionDependencyFactory* factory,
      scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
      const blink::WebMediaStream& web_stream);
  ~LocalWebRtcMediaStreamAdapter() override;

  // |WebRtcMediaStreamAdapter| implementation.
  bool is_initialized() const override;
  const scoped_refptr<webrtc::MediaStreamInterface>& webrtc_stream()
      const override;
  const blink::WebMediaStream& web_stream() const override;
  void SetTracks(TrackAdapterRefs track_refs) override;

 private:
  // A map between web track UniqueIDs and references to track adapters.
  using LocalAdapterRefMap =
      std::map<int,  // blink::WebMediaStreamTrack::UniqueId()
               std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>>;

  // |MediaStreamObserver| implementation. Also used as a helper functions when
  // adding/removing all tracks in the constructor/destructor.
  void TrackAdded(const blink::WebMediaStreamTrack& web_track) override;
  void TrackRemoved(const blink::WebMediaStreamTrack& web_track) override;

  // Pointer to a |PeerConnectionDependencyFactory|, owned by the
  // |RenderThread|. It's valid for the lifetime of |RenderThread|.
  PeerConnectionDependencyFactory* const factory_;

  // Track adapters belonging to this stream. Keeping adapter references alive
  // ensures the adapters are not disposed by the |track_adapter_map_| as long
  // as the webrtc layer track is in use by the webrtc layer stream.
  LocalAdapterRefMap adapter_refs_;

  DISALLOW_COPY_AND_ASSIGN(LocalWebRtcMediaStreamAdapter);
};

// Adapter implementation for a remote |webrtc::MediaStreamInterface|. Created
// on the the webrtc signaling thread, initialized on the main thread where it
// must be destroyed.
class RemoteWebRtcMediaStreamAdapter : public WebRtcMediaStreamAdapter {
 public:
  RemoteWebRtcMediaStreamAdapter(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map,
      scoped_refptr<webrtc::MediaStreamInterface> webrtc_stream);
  ~RemoteWebRtcMediaStreamAdapter() override;

  // |WebRtcMediaStreamAdapter| implementation.
  bool is_initialized() const override;
  const scoped_refptr<webrtc::MediaStreamInterface>& webrtc_stream()
      const override;
  const blink::WebMediaStream& web_stream() const override;
  void SetTracks(TrackAdapterRefs track_refs) override;

 private:
  // Track adapters for the remote webrtc tracks of a stream.
  using RemoteAdapterRefs = WebRtcMediaStreamAdapter::TrackAdapterRefs;

  static bool RemoteAdapterRefsContainsTrack(
      const RemoteAdapterRefs& adapter_refs,
      webrtc::MediaStreamTrackInterface* track);

  // Gets the adapters for the tracks that are members of the webrtc stream.
  // Invoke on webrtc signaling thread. New adapters are initialized in a post
  // to the main thread after which their |web_track| becomes available.
  static RemoteAdapterRefs GetRemoteAdapterRefsFromWebRtcStream(
      const scoped_refptr<WebRtcMediaStreamTrackAdapterMap>& track_adapter_map,
      webrtc::MediaStreamInterface* webrtc_stream);

  void InitializeOnMainThread(const std::string& label,
                              RemoteAdapterRefs track_adapter_refs,
                              size_t audio_track_count,
                              size_t video_track_count);

  mutable base::Lock lock_;
  bool is_initialized_;
  // Track adapters belonging to this stream. Keeping adapter references alive
  // ensures the adapters are not disposed by the |track_adapter_map_| as long
  // as the webrtc layer track is in use by the webrtc layer stream.
  RemoteAdapterRefs adapter_refs_;
  base::WeakPtrFactory<RemoteWebRtcMediaStreamAdapter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RemoteWebRtcMediaStreamAdapter);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_MEDIA_STREAM_ADAPTER_H_
