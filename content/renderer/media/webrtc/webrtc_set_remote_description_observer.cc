// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_set_remote_description_observer.h"

#include "base/logging.h"

namespace content {

WebRtcReceiverState::WebRtcReceiverState(
    scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref,
    std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
        stream_refs)
    : receiver(std::move(receiver)),
      track_ref(std::move(track_ref)),
      stream_refs(std::move(stream_refs)) {}

WebRtcReceiverState::WebRtcReceiverState(WebRtcReceiverState&& other) = default;

WebRtcReceiverState& WebRtcReceiverState::operator=(
    WebRtcReceiverState&& other) = default;

WebRtcReceiverState::~WebRtcReceiverState() {}

WebRtcSetRemoteDescriptionObserver::States::States() {}

WebRtcSetRemoteDescriptionObserver::States::States(States&& other)
    : receiver_states(std::move(other.receiver_states)) {}

WebRtcSetRemoteDescriptionObserver::States::~States() {}

WebRtcSetRemoteDescriptionObserver::States&
WebRtcSetRemoteDescriptionObserver::States::operator=(States&& other) {
  receiver_states = std::move(other.receiver_states);
  return *this;
}

void WebRtcSetRemoteDescriptionObserver::States::CheckInvariants() const {
  // Invariants:
  // - All receiver states have a stream ref
  // - All receiver states refer to streams that are non-null.
  for (auto& receiver_state : receiver_states) {
    for (auto& stream_ref : receiver_state.stream_refs) {
      CHECK(stream_ref);
      CHECK(!stream_ref->adapter().web_stream().IsNull());
    }
  }
}

WebRtcSetRemoteDescriptionObserver::WebRtcSetRemoteDescriptionObserver() {}

WebRtcSetRemoteDescriptionObserver::~WebRtcSetRemoteDescriptionObserver() {}

scoped_refptr<WebRtcSetRemoteDescriptionObserverHandler>
WebRtcSetRemoteDescriptionObserverHandler::Create(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread,
    scoped_refptr<webrtc::PeerConnectionInterface> pc,
    scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map,
    scoped_refptr<WebRtcSetRemoteDescriptionObserver> observer) {
  return new rtc::RefCountedObject<WebRtcSetRemoteDescriptionObserverHandler>(
      std::move(main_thread), std::move(pc), std::move(stream_adapter_map),
      std::move(observer));
}

WebRtcSetRemoteDescriptionObserverHandler::
    WebRtcSetRemoteDescriptionObserverHandler(
        scoped_refptr<base::SingleThreadTaskRunner> main_thread,
        scoped_refptr<webrtc::PeerConnectionInterface> pc,
        scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map,
        scoped_refptr<WebRtcSetRemoteDescriptionObserver> observer)
    : main_thread_(std::move(main_thread)),
      pc_(std::move(pc)),
      stream_adapter_map_(std::move(stream_adapter_map)),
      observer_(std::move(observer)) {}

WebRtcSetRemoteDescriptionObserverHandler::
    ~WebRtcSetRemoteDescriptionObserverHandler() {}

void WebRtcSetRemoteDescriptionObserverHandler::OnSetRemoteDescriptionComplete(
    webrtc::RTCError error) {
  CHECK(!main_thread_->BelongsToCurrentThread());

  webrtc::RTCErrorOr<WebRtcSetRemoteDescriptionObserver::States>
      states_or_error;
  if (error.ok()) {
    WebRtcSetRemoteDescriptionObserver::States states;
    for (const auto& webrtc_receiver : pc_->GetReceivers()) {
      std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref =
          track_adapter_map()->GetOrCreateRemoteTrackAdapter(
              webrtc_receiver->track().get());
      std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
          stream_refs;
      for (const auto& stream : webrtc_receiver->streams()) {
        stream_refs.push_back(
            stream_adapter_map_->GetOrCreateRemoteStreamAdapter(stream.get()));
      }
      states.receiver_states.push_back(WebRtcReceiverState(
          webrtc_receiver.get(), std::move(track_ref), std::move(stream_refs)));
    }
    states_or_error = std::move(states);
  } else {
    states_or_error = std::move(error);
  }
  main_thread_->PostTask(
      FROM_HERE, base::BindOnce(&WebRtcSetRemoteDescriptionObserverHandler::
                                    OnSetRemoteDescriptionCompleteOnMainThread,
                                this, std::move(states_or_error)));
}

void WebRtcSetRemoteDescriptionObserverHandler::
    OnSetRemoteDescriptionCompleteOnMainThread(
        webrtc::RTCErrorOr<WebRtcSetRemoteDescriptionObserver::States>
            states_or_error) {
  CHECK(main_thread_->BelongsToCurrentThread());
  observer_->OnSetRemoteDescriptionComplete(std::move(states_or_error));
}

}  // namespace content
