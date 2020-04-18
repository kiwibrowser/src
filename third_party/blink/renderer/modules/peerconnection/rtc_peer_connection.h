/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_PEER_CONNECTION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_PEER_CONNECTION_H_

#include <memory>

#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_rtc_configuration.h"
#include "third_party/blink/public/platform/web_rtc_peer_connection_handler.h"
#include "third_party/blink/public/platform/web_rtc_peer_connection_handler_client.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/dom/pausable_object.h"
#include "third_party/blink/renderer/modules/crypto/normalize_algorithm.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_ice_candidate.h"
#include "third_party/blink/renderer/platform/async_method_runner.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"

namespace blink {

class ExceptionState;
class MediaStreamTrack;
class RTCAnswerOptions;
class RTCConfiguration;
class RTCDTMFSender;
class RTCDataChannel;
class RTCDataChannelInit;
class RTCIceCandidateInitOrRTCIceCandidate;
class RTCOfferOptions;
class RTCPeerConnectionTest;
class RTCRtpReceiver;
class RTCRtpSender;
class RTCSessionDescription;
class RTCSessionDescriptionInit;
class ScriptState;
class V8RTCPeerConnectionErrorCallback;
class V8RTCSessionDescriptionCallback;
class V8RTCStatsCallback;
class V8VoidFunction;
struct WebRTCConfiguration;

class MODULES_EXPORT RTCPeerConnection final
    : public EventTargetWithInlineData,
      public WebRTCPeerConnectionHandlerClient,
      public ActiveScriptWrappable<RTCPeerConnection>,
      public PausableObject,
      public MediaStreamObserver {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(RTCPeerConnection);
  USING_PRE_FINALIZER(RTCPeerConnection, Dispose);

 public:
  static RTCPeerConnection* Create(ExecutionContext*,
                                   const RTCConfiguration&,
                                   const Dictionary&,
                                   ExceptionState&);
  ~RTCPeerConnection() override;

  ScriptPromise createOffer(ScriptState*, const RTCOfferOptions&);
  ScriptPromise createOffer(ScriptState*,
                            V8RTCSessionDescriptionCallback*,
                            V8RTCPeerConnectionErrorCallback*,
                            const Dictionary&,
                            ExceptionState&);

  ScriptPromise createAnswer(ScriptState*, const RTCAnswerOptions&);
  ScriptPromise createAnswer(ScriptState*,
                             V8RTCSessionDescriptionCallback*,
                             V8RTCPeerConnectionErrorCallback*,
                             const Dictionary&);

  ScriptPromise setLocalDescription(ScriptState*,
                                    const RTCSessionDescriptionInit&);
  ScriptPromise setLocalDescription(
      ScriptState*,
      const RTCSessionDescriptionInit&,
      V8VoidFunction*,
      V8RTCPeerConnectionErrorCallback* = nullptr);
  RTCSessionDescription* localDescription();

  ScriptPromise setRemoteDescription(ScriptState*,
                                     const RTCSessionDescriptionInit&);
  ScriptPromise setRemoteDescription(
      ScriptState*,
      const RTCSessionDescriptionInit&,
      V8VoidFunction*,
      V8RTCPeerConnectionErrorCallback* = nullptr);
  RTCSessionDescription* remoteDescription();

  String signalingState() const;

  void setConfiguration(ScriptState*, const RTCConfiguration&, ExceptionState&);

  // Certificate management
  // http://w3c.github.io/webrtc-pc/#sec.cert-mgmt
  static ScriptPromise generateCertificate(
      ScriptState*,
      const AlgorithmIdentifier& keygen_algorithm,
      ExceptionState&);

  ScriptPromise addIceCandidate(ScriptState*,
                                const RTCIceCandidateInitOrRTCIceCandidate&);
  ScriptPromise addIceCandidate(ScriptState*,
                                const RTCIceCandidateInitOrRTCIceCandidate&,
                                V8VoidFunction*,
                                V8RTCPeerConnectionErrorCallback*);

  String iceGatheringState() const;

  String iceConnectionState() const;

  MediaStreamVector getLocalStreams() const;

  MediaStreamVector getRemoteStreams() const;
  // Returns the remote stream for the descriptor, if one exists.
  MediaStream* getRemoteStream(MediaStreamDescriptor*) const;
  // Counts the number of receivers that have a remote stream for the descriptor
  // in its set of associated remote streams.
  size_t getRemoteStreamUsageCount(MediaStreamDescriptor*) const;

  void addStream(ScriptState*,
                 MediaStream*,
                 const Dictionary& media_constraints,
                 ExceptionState&);

  void removeStream(MediaStream*, ExceptionState&);

  String id(ScriptState*) const;

  // Calls one of the below versions (or rejects with an exception) depending on
  // type, see RTCPeerConnection.idl.
  ScriptPromise getStats(ScriptState*, blink::ScriptValue callback_or_selector);
  // Calls LegacyCallbackBasedGetStats().
  ScriptPromise getStats(ScriptState*,
                         V8RTCStatsCallback* success_callback,
                         MediaStreamTrack* selector = nullptr);
  // Calls PromiseBasedGetStats().
  ScriptPromise getStats(ScriptState*, MediaStreamTrack* selector = nullptr);
  ScriptPromise LegacyCallbackBasedGetStats(
      ScriptState*,
      V8RTCStatsCallback* success_callback,
      MediaStreamTrack* selector);
  ScriptPromise PromiseBasedGetStats(ScriptState*, MediaStreamTrack* selector);

  const HeapVector<Member<RTCRtpSender>>& getSenders() const;
  const HeapVector<Member<RTCRtpReceiver>>& getReceivers() const;
  RTCRtpSender* addTrack(MediaStreamTrack*, MediaStreamVector, ExceptionState&);
  void removeTrack(RTCRtpSender*, ExceptionState&);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(track);

  RTCDataChannel* createDataChannel(ScriptState*,
                                    String label,
                                    const RTCDataChannelInit&,
                                    ExceptionState&);

  RTCDTMFSender* createDTMFSender(MediaStreamTrack*, ExceptionState&);

  bool IsClosed() { return closed_; }
  void close();

  // We allow getStats after close, but not other calls or callbacks.
  bool ShouldFireDefaultCallbacks() { return !closed_ && !stopped_; }
  bool ShouldFireGetStatsCallback() { return !stopped_; }

  DEFINE_ATTRIBUTE_EVENT_LISTENER(negotiationneeded);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(icecandidate);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(signalingstatechange);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(addstream);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(removestream);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(iceconnectionstatechange);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(icegatheringstatechange);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(datachannel);

  // Utility to note result of CreateOffer / CreateAnswer
  void NoteSdpCreated(const RTCSessionDescription&);

  // MediaStreamObserver
  void OnStreamAddTrack(MediaStream*, MediaStreamTrack*) override;
  void OnStreamRemoveTrack(MediaStream*, MediaStreamTrack*) override;

  // WebRTCPeerConnectionHandlerClient
  void NegotiationNeeded() override;
  void DidGenerateICECandidate(scoped_refptr<WebRTCICECandidate>) override;
  void DidChangeSignalingState(SignalingState) override;
  void DidChangeICEGatheringState(ICEGatheringState) override;
  void DidChangeICEConnectionState(ICEConnectionState) override;
  void DidAddRemoteTrack(std::unique_ptr<WebRTCRtpReceiver>) override;
  void DidRemoveRemoteTrack(std::unique_ptr<WebRTCRtpReceiver>) override;
  void DidAddRemoteDataChannel(WebRTCDataChannelHandler*) override;
  void ReleasePeerConnectionHandler() override;
  void ClosePeerConnection() override;
  WebRTCOriginTrials GetOriginTrials() override;

  // EventTarget
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;

  // PausableObject
  void Pause() override;
  void Unpause() override;
  void ContextDestroyed(ExecutionContext*) override;

  // ScriptWrappable
  // We keep the this object alive until either stopped or closed.
  bool HasPendingActivity() const final { return !closed_ && !stopped_; }

  // For testing; exported to testing/InternalWebRTCPeerConnection
  static int PeerConnectionCount();
  static int PeerConnectionCountLimit();

  void Trace(blink::Visitor*) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(RTCPeerConnectionTest, GetAudioTrack);
  FRIEND_TEST_ALL_PREFIXES(RTCPeerConnectionTest, GetVideoTrack);
  FRIEND_TEST_ALL_PREFIXES(RTCPeerConnectionTest, GetAudioAndVideoTrack);
  FRIEND_TEST_ALL_PREFIXES(RTCPeerConnectionTest, GetTrackRemoveStreamAndGCAll);
  FRIEND_TEST_ALL_PREFIXES(RTCPeerConnectionTest,
                           GetTrackRemoveStreamAndGCWithPersistentComponent);
  FRIEND_TEST_ALL_PREFIXES(RTCPeerConnectionTest,
                           GetTrackRemoveStreamAndGCWithPersistentStream);

  typedef base::OnceCallback<bool()> BoolFunction;
  class EventWrapper : public GarbageCollectedFinalized<EventWrapper> {
   public:
    EventWrapper(Event*, BoolFunction);
    // Returns true if |m_setupFunction| returns true or it is null.
    // |m_event| will only be fired if setup() returns true;
    bool Setup();

    void Trace(blink::Visitor*);

    Member<Event> event_;

   private:
    BoolFunction setup_function_;
  };

  RTCPeerConnection(ExecutionContext*,
                    const WebRTCConfiguration&,
                    WebMediaConstraints,
                    ExceptionState&);
  void Dispose();

  void ScheduleDispatchEvent(Event*);
  void ScheduleDispatchEvent(Event*, BoolFunction);
  void DispatchScheduledEvent();
  void MaybeFireNegotiationNeeded();
  MediaStreamTrack* GetTrack(const WebMediaStreamTrack&) const;
  RTCRtpSender* FindSenderForTrackAndStream(MediaStreamTrack*, MediaStream*);
  HeapVector<Member<RTCRtpSender>>::iterator FindSender(
      const WebRTCRtpSender& web_sender);
  HeapVector<Member<RTCRtpReceiver>>::iterator FindReceiver(
      const WebRTCRtpReceiver& web_receiver);

  // The "Change" methods set the state asynchronously and fire the
  // corresponding event immediately after changing the state (if it was really
  // changed).
  //
  // The "Set" methods are called asynchronously by the "Change" methods, and
  // set the corresponding state without firing an event, returning true if the
  // state was really changed.
  //
  // This is done because the standard guarantees that state changes and the
  // corresponding events will happen in the same task; it shouldn't be
  // possible to, for example, end up with two "icegatheringstatechange" events
  // that are delayed somehow and cause the application to read a "complete"
  // gathering state twice, missing the "gathering" state in the middle.
  //
  // TODO(deadbeef): This wasn't done for the signaling state because it
  // resulted in a change to the order of the signaling state being updated
  // relative to the SetLocalDescription or SetRemoteDescription promise being
  // resolved. Some additional refactoring would be necessary to fix this; for
  // example, passing the new signaling state along with the SRD/SLD callbacks
  // as opposed to relying on a separate event.
  void ChangeSignalingState(WebRTCPeerConnectionHandlerClient::SignalingState);

  void ChangeIceGatheringState(
      WebRTCPeerConnectionHandlerClient::ICEGatheringState);
  bool SetIceGatheringState(
      WebRTCPeerConnectionHandlerClient::ICEGatheringState);

  void ChangeIceConnectionState(
      WebRTCPeerConnectionHandlerClient::ICEConnectionState);
  bool SetIceConnectionState(
      WebRTCPeerConnectionHandlerClient::ICEConnectionState);

  void CloseInternal();

  void RecordRapporMetrics();

  DOMException* checkSdpForStateErrors(ExecutionContext*,
                                       const RTCSessionDescriptionInit&,
                                       String* sdp);

  SignalingState signaling_state_;
  ICEGatheringState ice_gathering_state_;
  ICEConnectionState ice_connection_state_;

  // A map containing any track that is in use by the peer connection. This
  // includes tracks of |rtp_senders_| and |rtp_receivers_|.
  HeapHashMap<WeakMember<MediaStreamComponent>, WeakMember<MediaStreamTrack>>
      tracks_;
  HeapVector<Member<RTCRtpSender>> rtp_senders_;
  HeapVector<Member<RTCRtpReceiver>> rtp_receivers_;

  std::unique_ptr<WebRTCPeerConnectionHandler> peer_handler_;

  Member<AsyncMethodRunner<RTCPeerConnection>> dispatch_scheduled_event_runner_;
  HeapVector<Member<EventWrapper>> scheduled_events_;

  // This handle notifies scheduler about an active connection associated
  // with a frame. Handle should be destroyed when connection is closed.
  std::unique_ptr<FrameScheduler::ActiveConnectionHandle>
      connection_handle_for_scheduler_;

  bool negotiation_needed_;
  bool stopped_;
  bool closed_;

  // Internal state [[LastOffer]] and [[LastAnswer]]
  String last_offer_;
  String last_answer_;

  bool has_data_channels_;  // For RAPPOR metrics
  WebRTCSdpSemantics sdp_semantics_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_PEER_CONNECTION_H_
