// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/common/content_export.h"
#include "content/renderer/media/webrtc/rtc_peer_connection_handler.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter.h"
#include "third_party/webrtc/api/rtcerror.h"
#include "third_party/webrtc/api/rtpreceiverinterface.h"
#include "third_party/webrtc/api/setremotedescriptionobserverinterface.h"
#include "third_party/webrtc/rtc_base/refcount.h"
#include "third_party/webrtc/rtc_base/refcountedobject.h"
#include "third_party/webrtc/rtc_base/scoped_ref_ptr.h"

namespace content {

// Describes an instance of a receiver at the time the SRD call was completed.
// Because webrtc and content operate on different threads, webrtc objects may
// have been modified by the time we synchronize the receivers on the main
// thread, and members of this class should be inspected rather than members of
// |receiver|.
struct CONTENT_EXPORT WebRtcReceiverState {
  WebRtcReceiverState(
      scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref,
      std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
          stream_refs);
  WebRtcReceiverState(WebRtcReceiverState&& other);
  ~WebRtcReceiverState();

  WebRtcReceiverState& operator=(WebRtcReceiverState&& other);

  scoped_refptr<webrtc::RtpReceiverInterface> receiver;
  // The receiver's track when the SRD occurred.
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref;
  // The receiver's associated set of streams when the SRD occurred.
  std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
      stream_refs;

  DISALLOW_COPY_AND_ASSIGN(WebRtcReceiverState);
};

// The content layer correspondent of
// webrtc::SetRemoteDescriptionObserverInterface. It's an interface with
// callbacks for handling the result of SetRemoteDescription on the main thread.
// The implementation should process the state changes of the
// SetRemoteDescription by inspecting the updated States.
class CONTENT_EXPORT WebRtcSetRemoteDescriptionObserver
    : public base::RefCountedThreadSafe<WebRtcSetRemoteDescriptionObserver> {
 public:
  // The relevant peer connection states as they were when the
  // SetRemoteDescription call completed. This is used instead of inspecting the
  // PeerConnection and other webrtc objects directly because they may have been
  // modified before we reach the main thread.
  struct CONTENT_EXPORT States {
    States();
    States(States&& other);
    ~States();

    States& operator=(States&& other);

    // The receivers at the time of the event.
    std::vector<WebRtcReceiverState> receiver_states;
    // Check that the invariants for this structure hold.
    void CheckInvariants() const;

    DISALLOW_COPY_AND_ASSIGN(States);
  };

  WebRtcSetRemoteDescriptionObserver();

  // Invoked asynchronously on the main thread after the SetRemoteDescription
  // completed on the webrtc signaling thread.
  virtual void OnSetRemoteDescriptionComplete(
      webrtc::RTCErrorOr<States> states_or_error) = 0;

 protected:
  friend class base::RefCountedThreadSafe<WebRtcSetRemoteDescriptionObserver>;
  virtual ~WebRtcSetRemoteDescriptionObserver();

  DISALLOW_COPY_AND_ASSIGN(WebRtcSetRemoteDescriptionObserver);
};

// The glue between webrtc and content layer observers listening to
// SetRemoteDescription. This observer listens on the webrtc signaling thread
// for the result of SetRemoteDescription, copies any relevant webrtc peer
// connection states such that they can be processed on the main thread, and
// invokes the WebRtcSetRemoteDescriptionObserver on the main thread with the
// state changes.
class CONTENT_EXPORT WebRtcSetRemoteDescriptionObserverHandler
    : public webrtc::SetRemoteDescriptionObserverInterface {
 public:
  static scoped_refptr<WebRtcSetRemoteDescriptionObserverHandler> Create(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<webrtc::PeerConnectionInterface> pc,
      scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map,
      scoped_refptr<WebRtcSetRemoteDescriptionObserver> observer);

  // webrtc::SetRemoteDescriptionObserverInterface implementation.
  void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

 protected:
  WebRtcSetRemoteDescriptionObserverHandler(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<webrtc::PeerConnectionInterface> pc,
      scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map,
      scoped_refptr<WebRtcSetRemoteDescriptionObserver> observer);
  ~WebRtcSetRemoteDescriptionObserverHandler() override;

 private:
  void OnSetRemoteDescriptionCompleteOnMainThread(
      webrtc::RTCErrorOr<WebRtcSetRemoteDescriptionObserver::States>
          states_or_error);

  scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map() const {
    return stream_adapter_map_->track_adapter_map();
  }

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<webrtc::PeerConnectionInterface> pc_;
  scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map_;
  scoped_refptr<WebRtcSetRemoteDescriptionObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcSetRemoteDescriptionObserverHandler);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_H_
