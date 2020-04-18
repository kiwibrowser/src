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

#include "third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.h"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_crypto_algorithm_params.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_rtc_answer_options.h"
#include "third_party/blink/public/platform/web_rtc_certificate.h"
#include "third_party/blink/public/platform/web_rtc_certificate_generator.h"
#include "third_party/blink/public/platform/web_rtc_configuration.h"
#include "third_party/blink/public/platform/web_rtc_data_channel_handler.h"
#include "third_party/blink/public/platform/web_rtc_data_channel_init.h"
#include "third_party/blink/public/platform/web_rtc_ice_candidate.h"
#include "third_party/blink/public/platform/web_rtc_key_params.h"
#include "third_party/blink/public/platform/web_rtc_offer_options.h"
#include "third_party/blink/public/platform/web_rtc_session_description.h"
#include "third_party/blink/public/platform/web_rtc_session_description_request.h"
#include "third_party/blink/public/platform/web_rtc_stats_request.h"
#include "third_party/blink/public/platform/web_rtc_void_request.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_void_function.h"
#include "third_party/blink/renderer/bindings/modules/v8/rtc_ice_candidate_init_or_rtc_ice_candidate.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_media_stream_track.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_rtc_certificate.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_rtc_peer_connection_error_callback.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_rtc_session_description_callback.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_rtc_stats_callback.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/dom_time_stamp.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/hosts_using_features.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trials.h"
#include "third_party/blink/renderer/modules/crypto/crypto_result_impl.h"
#include "third_party/blink/renderer/modules/mediastream/media_constraints_impl.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_event.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_answer_options.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_configuration.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_data_channel.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_data_channel_event.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_data_channel_init.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_dtmf_sender.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_ice_server.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_offer_options.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_peer_connection_ice_event.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_rtp_receiver.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_rtp_sender.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_session_description.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_session_description_init.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_session_description_request_impl.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_session_description_request_promise_impl.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_stats_report.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_stats_request_impl.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_track_event.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_void_request_impl.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_void_request_promise_impl.h"
#include "third_party/blink/renderer/modules/peerconnection/testing/internals_rtc_peer_connection.h"
#include "third_party/blink/renderer/modules/peerconnection/web_rtc_stats_report_callback_resolver.h"
#include "third_party/blink/renderer/platform/bindings/microtask.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/instance_counters.h"
#include "third_party/blink/renderer/platform/peerconnection/rtc_answer_options_platform.h"
#include "third_party/blink/renderer/platform/peerconnection/rtc_offer_options_platform.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

const char kSignalingStateClosedMessage[] =
    "The RTCPeerConnection's signalingState is 'closed'.";
const char kModifiedSdpMessage[] =
    "The SDP does not match the previously generated SDP for this type";

// The maximum number of PeerConnections that can exist simultaneously.
const long kMaxPeerConnections = 500;

bool ThrowExceptionIfSignalingStateClosed(
    RTCPeerConnection::SignalingState state,
    ExceptionState& exception_state) {
  if (state == RTCPeerConnection::kSignalingStateClosed) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      kSignalingStateClosedMessage);
    return true;
  }

  return false;
}

void AsyncCallErrorCallback(V8RTCPeerConnectionErrorCallback* error_callback,
                            DOMException* exception) {
  DCHECK(error_callback);
  Microtask::EnqueueMicrotask(
      WTF::Bind(&V8PersistentCallbackFunction<
                    V8RTCPeerConnectionErrorCallback>::InvokeAndReportException,
                WrapPersistent(ToV8PersistentCallbackFunction(error_callback)),
                nullptr, WrapPersistent(exception)));
}

bool CallErrorCallbackIfSignalingStateClosed(
    RTCPeerConnection::SignalingState state,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  if (state == RTCPeerConnection::kSignalingStateClosed) {
    if (error_callback)
      AsyncCallErrorCallback(
          error_callback, DOMException::Create(kInvalidStateError,
                                               kSignalingStateClosedMessage));

    return true;
  }

  return false;
}

bool IsIceCandidateMissingSdp(
    const RTCIceCandidateInitOrRTCIceCandidate& candidate) {
  if (candidate.IsRTCIceCandidateInit()) {
    const RTCIceCandidateInit& ice_candidate_init =
        candidate.GetAsRTCIceCandidateInit();
    return !ice_candidate_init.hasSdpMid() &&
           !ice_candidate_init.hasSdpMLineIndex();
  }

  DCHECK(candidate.IsRTCIceCandidate());
  return false;
}

WebRTCOfferOptions ConvertToWebRTCOfferOptions(const RTCOfferOptions& options) {
  return WebRTCOfferOptions(RTCOfferOptionsPlatform::Create(
      options.hasOfferToReceiveVideo()
          ? std::max(options.offerToReceiveVideo(), 0)
          : -1,
      options.hasOfferToReceiveAudio()
          ? std::max(options.offerToReceiveAudio(), 0)
          : -1,
      options.hasVoiceActivityDetection() ? options.voiceActivityDetection()
                                          : true,
      options.hasIceRestart() ? options.iceRestart() : false));
}

WebRTCAnswerOptions ConvertToWebRTCAnswerOptions(
    const RTCAnswerOptions& options) {
  return WebRTCAnswerOptions(RTCAnswerOptionsPlatform::Create(
      options.hasVoiceActivityDetection() ? options.voiceActivityDetection()
                                          : true));
}

scoped_refptr<WebRTCICECandidate> ConvertToWebRTCIceCandidate(
    ExecutionContext* context,
    const RTCIceCandidateInitOrRTCIceCandidate& candidate) {
  DCHECK(!candidate.IsNull());
  if (candidate.IsRTCIceCandidateInit()) {
    const RTCIceCandidateInit& ice_candidate_init =
        candidate.GetAsRTCIceCandidateInit();
    // TODO(guidou): Change default value to -1. crbug.com/614958.
    unsigned short sdp_m_line_index = 0;
    if (ice_candidate_init.hasSdpMLineIndex()) {
      sdp_m_line_index = ice_candidate_init.sdpMLineIndex();
    } else {
      UseCounter::Count(context,
                        WebFeature::kRTCIceCandidateDefaultSdpMLineIndex);
    }
    return WebRTCICECandidate::Create(ice_candidate_init.candidate(),
                                      ice_candidate_init.sdpMid(),
                                      sdp_m_line_index);
  }

  DCHECK(candidate.IsRTCIceCandidate());
  return candidate.GetAsRTCIceCandidate()->WebCandidate();
}

// Helper class for RTCPeerConnection::generateCertificate.
class WebRTCCertificateObserver : public WebRTCCertificateCallback {
 public:
  // Takes ownership of |resolver|.
  static WebRTCCertificateObserver* Create(ScriptPromiseResolver* resolver) {
    return new WebRTCCertificateObserver(resolver);
  }

  ~WebRTCCertificateObserver() override = default;

 private:
  explicit WebRTCCertificateObserver(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}

  void OnSuccess(std::unique_ptr<WebRTCCertificate> certificate) override {
    resolver_->Resolve(new RTCCertificate(std::move(certificate)));
  }

  void OnError() override { resolver_->Reject(); }

  Persistent<ScriptPromiseResolver> resolver_;
};

WebRTCIceTransportPolicy IceTransportPolicyFromString(const String& policy) {
  if (policy == "relay")
    return WebRTCIceTransportPolicy::kRelay;
  DCHECK_EQ(policy, "all");
  return WebRTCIceTransportPolicy::kAll;
}

WebRTCConfiguration ParseConfiguration(ExecutionContext* context,
                                       const RTCConfiguration& configuration,
                                       ExceptionState& exception_state) {
  DCHECK(context);

  WebRTCIceTransportPolicy ice_transport_policy =
      WebRTCIceTransportPolicy::kAll;
  if (configuration.hasIceTransportPolicy()) {
    UseCounter::Count(context, WebFeature::kRTCConfigurationIceTransportPolicy);
    ice_transport_policy =
        IceTransportPolicyFromString(configuration.iceTransportPolicy());
  } else if (configuration.hasIceTransports()) {
    UseCounter::Count(context, WebFeature::kRTCConfigurationIceTransports);
    ice_transport_policy =
        IceTransportPolicyFromString(configuration.iceTransports());
  }

  WebRTCBundlePolicy bundle_policy = WebRTCBundlePolicy::kBalanced;
  String bundle_policy_string = configuration.bundlePolicy();
  if (bundle_policy_string == "max-compat") {
    bundle_policy = WebRTCBundlePolicy::kMaxCompat;
  } else if (bundle_policy_string == "max-bundle") {
    bundle_policy = WebRTCBundlePolicy::kMaxBundle;
  } else {
    DCHECK_EQ(bundle_policy_string, "balanced");
  }

  WebRTCRtcpMuxPolicy rtcp_mux_policy = WebRTCRtcpMuxPolicy::kRequire;
  String rtcp_mux_policy_string = configuration.rtcpMuxPolicy();
  if (rtcp_mux_policy_string == "negotiate") {
    rtcp_mux_policy = WebRTCRtcpMuxPolicy::kNegotiate;
    Deprecation::CountDeprecation(context, WebFeature::kRtcpMuxPolicyNegotiate);
  } else {
    DCHECK_EQ(rtcp_mux_policy_string, "require");
  }

  WebRTCSdpSemantics sdp_semantics = WebRTCSdpSemantics::kDefault;
  if (configuration.hasSdpSemantics()) {
    String sdp_semantics_string = configuration.sdpSemantics();
    if (sdp_semantics_string == "plan-b") {
      sdp_semantics = WebRTCSdpSemantics::kPlanB;
    } else {
      DCHECK_EQ(sdp_semantics_string, "unified-plan");
      sdp_semantics = WebRTCSdpSemantics::kUnifiedPlan;
    }
  }

  WebRTCConfiguration web_configuration;
  web_configuration.ice_transport_policy = ice_transport_policy;
  web_configuration.bundle_policy = bundle_policy;
  web_configuration.rtcp_mux_policy = rtcp_mux_policy;
  web_configuration.sdp_semantics = sdp_semantics;

  if (configuration.hasIceServers()) {
    Vector<WebRTCIceServer> ice_servers;
    for (const RTCIceServer& ice_server : configuration.iceServers()) {
      Vector<String> url_strings;
      if (ice_server.hasURLs()) {
        UseCounter::Count(context, WebFeature::kRTCIceServerURLs);
        const StringOrStringSequence& urls = ice_server.urls();
        if (urls.IsString()) {
          url_strings.push_back(urls.GetAsString());
        } else {
          DCHECK(urls.IsStringSequence());
          url_strings = urls.GetAsStringSequence();
        }
      } else if (ice_server.hasURL()) {
        UseCounter::Count(context, WebFeature::kRTCIceServerURL);
        url_strings.push_back(ice_server.url());
      } else {
        exception_state.ThrowTypeError("Malformed RTCIceServer");
        return WebRTCConfiguration();
      }

      String username = ice_server.username();
      String credential = ice_server.credential();

      for (const String& url_string : url_strings) {
        KURL url(NullURL(), url_string);
        if (!url.IsValid()) {
          exception_state.ThrowDOMException(
              kSyntaxError, "'" + url_string + "' is not a valid URL.");
          return WebRTCConfiguration();
        }
        if (!(url.ProtocolIs("turn") || url.ProtocolIs("turns") ||
              url.ProtocolIs("stun"))) {
          exception_state.ThrowDOMException(
              kSyntaxError, "'" + url.Protocol() +
                                "' is not one of the supported URL schemes "
                                "'stun', 'turn' or 'turns'.");
          return WebRTCConfiguration();
        }
        if ((url.ProtocolIs("turn") || url.ProtocolIs("turns")) &&
            (username.IsNull() || credential.IsNull())) {
          exception_state.ThrowDOMException(kInvalidAccessError,
                                            "Both username and credential are "
                                            "required when the URL scheme is "
                                            "\"turn\" or \"turns\".");
        }
        ice_servers.push_back(WebRTCIceServer{url, username, credential});
      }
    }
    web_configuration.ice_servers = ice_servers;
  }

  if (configuration.hasCertificates()) {
    const HeapVector<Member<RTCCertificate>>& certificates =
        configuration.certificates();
    WebVector<std::unique_ptr<WebRTCCertificate>> certificates_copy(
        certificates.size());
    for (size_t i = 0; i < certificates.size(); ++i) {
      certificates_copy[i] = certificates[i]->CertificateShallowCopy();
    }
    web_configuration.certificates = std::move(certificates_copy);
  }

  web_configuration.ice_candidate_pool_size =
      configuration.iceCandidatePoolSize();
  return web_configuration;
}

RTCOfferOptionsPlatform* ParseOfferOptions(const Dictionary& options,
                                           ExceptionState& exception_state) {
  if (options.IsUndefinedOrNull())
    return nullptr;

  const Vector<String>& property_names =
      options.GetPropertyNames(exception_state);
  if (exception_state.HadException())
    return nullptr;

  // Treat |options| as MediaConstraints if it is empty or has "optional" or
  // "mandatory" properties for compatibility.
  // TODO(jiayl): remove constraints when RTCOfferOptions reaches Stable and
  // client code is ready.
  if (property_names.IsEmpty() || property_names.Contains("optional") ||
      property_names.Contains("mandatory"))
    return nullptr;

  int32_t offer_to_receive_video = -1;
  int32_t offer_to_receive_audio = -1;
  bool voice_activity_detection = true;
  bool ice_restart = false;

  if (DictionaryHelper::Get(options, "offerToReceiveVideo",
                            offer_to_receive_video) &&
      offer_to_receive_video < 0)
    offer_to_receive_video = 0;
  if (DictionaryHelper::Get(options, "offerToReceiveAudio",
                            offer_to_receive_audio) &&
      offer_to_receive_audio < 0)
    offer_to_receive_audio = 0;
  DictionaryHelper::Get(options, "voiceActivityDetection",
                        voice_activity_detection);
  DictionaryHelper::Get(options, "iceRestart", ice_restart);

  RTCOfferOptionsPlatform* rtc_offer_options = RTCOfferOptionsPlatform::Create(
      offer_to_receive_video, offer_to_receive_audio, voice_activity_detection,
      ice_restart);
  return rtc_offer_options;
}

bool FingerprintMismatch(String old_sdp, String new_sdp) {
  // Check special case of externally generated SDP without fingerprints.
  // It's impossible to generate a valid fingerprint without createOffer
  // or createAnswer, so this only applies when there are no fingerprints.
  // This is allowed.
  const size_t new_fingerprint_pos = new_sdp.Find("\na=fingerprint:");
  if (new_fingerprint_pos == kNotFound) {
    return false;
  }
  // Look for fingerprint having been added. Not allowed.
  const size_t old_fingerprint_pos = old_sdp.Find("\na=fingerprint:");
  if (old_fingerprint_pos == kNotFound) {
    return true;
  }
  // Look for fingerprint being modified. Not allowed.  Handle differences in
  // line endings ('\r\n' vs, '\n' when looking for the end of the fingerprint).
  size_t old_fingerprint_end = old_sdp.Find("\r\n", old_fingerprint_pos + 1);
  if (old_fingerprint_end == WTF::kNotFound) {
    old_fingerprint_end = old_sdp.Find("\n", old_fingerprint_pos + 1);
  }
  size_t new_fingerprint_end = new_sdp.Find("\r\n", new_fingerprint_pos + 1);
  if (new_fingerprint_end == WTF::kNotFound) {
    new_fingerprint_end = new_sdp.Find("\n", new_fingerprint_pos + 1);
  }
  return old_sdp.Substring(old_fingerprint_pos,
                           old_fingerprint_end - old_fingerprint_pos) !=
         new_sdp.Substring(new_fingerprint_pos,
                           new_fingerprint_end - new_fingerprint_pos);
}

}  // namespace

RTCPeerConnection::EventWrapper::EventWrapper(Event* event,
                                              BoolFunction function)
    : event_(event), setup_function_(std::move(function)) {}

bool RTCPeerConnection::EventWrapper::Setup() {
  if (setup_function_) {
    return std::move(setup_function_).Run();
  }
  return true;
}

void RTCPeerConnection::EventWrapper::Trace(blink::Visitor* visitor) {
  visitor->Trace(event_);
}

RTCPeerConnection* RTCPeerConnection::Create(
    ExecutionContext* context,
    const RTCConfiguration& rtc_configuration,
    const Dictionary& media_constraints,
    ExceptionState& exception_state) {
  // Count number of PeerConnections that could potentially be impacted by CSP
  if (context) {
    auto& security_context = context->GetSecurityContext();
    auto* content_security_policy = security_context.GetContentSecurityPolicy();
    if (content_security_policy &&
        content_security_policy->IsActiveForConnections()) {
      UseCounter::Count(context, WebFeature::kRTCPeerConnectionWithActiveCsp);
    }
  }

  if (media_constraints.IsObject()) {
    UseCounter::Count(context,
                      WebFeature::kRTCPeerConnectionConstructorConstraints);
  } else {
    UseCounter::Count(context,
                      WebFeature::kRTCPeerConnectionConstructorCompliant);
  }

  WebRTCConfiguration configuration =
      ParseConfiguration(context, rtc_configuration, exception_state);
  if (exception_state.HadException())
    return nullptr;
  // Override default SDP semantics if RuntimeEnabled=RTCUnifiedPlanByDefault.
  if (!rtc_configuration.hasSdpSemantics() &&
      RuntimeEnabledFeatures::RTCUnifiedPlanByDefaultEnabled()) {
    configuration.sdp_semantics = WebRTCSdpSemantics::kUnifiedPlan;
  }

  // Make sure no certificates have expired.
  if (configuration.certificates.size() > 0) {
    DOMTimeStamp now = ConvertSecondsToDOMTimeStamp(CurrentTime());
    for (const std::unique_ptr<WebRTCCertificate>& certificate :
         configuration.certificates) {
      DOMTimeStamp expires = certificate->Expires();
      if (expires <= now) {
        exception_state.ThrowDOMException(kInvalidAccessError,
                                          "Expired certificate(s).");
        return nullptr;
      }
    }
  }

  MediaErrorState media_error_state;
  WebMediaConstraints constraints = MediaConstraintsImpl::Create(
      context, media_constraints, media_error_state);
  if (media_error_state.HadException()) {
    media_error_state.RaiseException(exception_state);
    return nullptr;
  }

  RTCPeerConnection* peer_connection = new RTCPeerConnection(
      context, configuration, constraints, exception_state);
  peer_connection->PauseIfNeeded();
  if (exception_state.HadException())
    return nullptr;

  return peer_connection;
}

RTCPeerConnection::RTCPeerConnection(ExecutionContext* context,
                                     const WebRTCConfiguration& configuration,
                                     WebMediaConstraints constraints,
                                     ExceptionState& exception_state)
    : PausableObject(context),
      signaling_state_(kSignalingStateStable),
      ice_gathering_state_(kICEGatheringStateNew),
      ice_connection_state_(kICEConnectionStateNew),
      // WebRTC spec specifies kNetworking as task source.
      // https://www.w3.org/TR/webrtc/#operation
      dispatch_scheduled_event_runner_(
          AsyncMethodRunner<RTCPeerConnection>::Create(
              this,
              &RTCPeerConnection::DispatchScheduledEvent,
              context->GetTaskRunner(TaskType::kNetworking))),
      negotiation_needed_(false),
      stopped_(false),
      closed_(false),
      has_data_channels_(false),
      sdp_semantics_(configuration.sdp_semantics) {
  Document* document = ToDocument(GetExecutionContext());

  InstanceCounters::IncrementCounter(
      InstanceCounters::kRTCPeerConnectionCounter);
  // If we fail, set |m_closed| and |m_stopped| to true, to avoid hitting the
  // assert in the destructor.
  if (InstanceCounters::CounterValue(
          InstanceCounters::kRTCPeerConnectionCounter) > kMaxPeerConnections) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(kUnknownError,
                                      "Cannot create so many PeerConnections");
    return;
  }
  if (!document->GetFrame()) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "PeerConnections may not be created in detached documents.");
    return;
  }

  peer_handler_ = Platform::Current()->CreateRTCPeerConnectionHandler(
      this, document->GetTaskRunner(TaskType::kInternalMedia));
  if (!peer_handler_) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "No PeerConnection handler can be "
                                      "created, perhaps WebRTC is disabled?");
    return;
  }

  document->GetFrame()->Client()->DispatchWillStartUsingPeerConnectionHandler(
      peer_handler_.get());

  if (!peer_handler_->Initialize(configuration, constraints)) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(
        kNotSupportedError, "Failed to initialize native PeerConnection.");
    return;
  }

  connection_handle_for_scheduler_ =
      document->GetFrame()->GetFrameScheduler()->OnActiveConnectionCreated();
}

RTCPeerConnection::~RTCPeerConnection() {
  // This checks that close() or stop() is called before the destructor.
  // We are assuming that a wrapper is always created when RTCPeerConnection is
  // created.
  DCHECK(closed_ || stopped_);
  InstanceCounters::DecrementCounter(
      InstanceCounters::kRTCPeerConnectionCounter);
  DCHECK(InstanceCounters::CounterValue(
             InstanceCounters::kRTCPeerConnectionCounter) >= 0);
}

void RTCPeerConnection::Dispose() {
  // Promptly clears a raw reference from content/ to an on-heap object
  // so that content/ doesn't access it in a lazy sweeping phase.
  peer_handler_.reset();
}

ScriptPromise RTCPeerConnection::createOffer(ScriptState* script_state,
                                             const RTCOfferOptions& options) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestPromiseImpl::Create(this, resolver);
  if (options.hasOfferToReceiveAudio() || options.hasOfferToReceiveVideo()) {
    ExecutionContext* context = ExecutionContext::From(script_state);
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionCreateOfferOptionsOfferToReceive);
  }
  peer_handler_->CreateOffer(request, ConvertToWebRTCOfferOptions(options));
  return promise;
}

ScriptPromise RTCPeerConnection::createOffer(
    ScriptState* script_state,
    V8RTCSessionDescriptionCallback* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback,
    const Dictionary& rtc_offer_options,
    ExceptionState& exception_state) {
  DCHECK(success_callback);
  DCHECK(error_callback);
  ExecutionContext* context = ExecutionContext::From(script_state);
  UseCounter::Count(
      context, WebFeature::kRTCPeerConnectionCreateOfferLegacyFailureCallback);
  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  RTCOfferOptionsPlatform* offer_options =
      ParseOfferOptions(rtc_offer_options, exception_state);
  if (exception_state.HadException())
    return ScriptPromise();
  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestImpl::Create(
          GetExecutionContext(), this, success_callback, error_callback);

  if (offer_options) {
    if (offer_options->OfferToReceiveAudio() != -1 ||
        offer_options->OfferToReceiveVideo() != -1) {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyOfferOptions);
    } else {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyCompliant);
    }

    peer_handler_->CreateOffer(request, WebRTCOfferOptions(offer_options));
  } else {
    MediaErrorState media_error_state;
    WebMediaConstraints constraints = MediaConstraintsImpl::Create(
        context, rtc_offer_options, media_error_state);
    // Report constraints parsing errors via the callback, but ignore
    // unknown/unsupported constraints as they would be silently discarded by
    // WebIDL.
    if (media_error_state.CanGenerateException()) {
      String error_msg = media_error_state.GetErrorMessage();
      AsyncCallErrorCallback(error_callback,
                             DOMException::Create(kOperationError, error_msg));
      return ScriptPromise::CastUndefined(script_state);
    }

    if (!constraints.IsEmpty()) {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyConstraints);
    } else {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyCompliant);
    }

    peer_handler_->CreateOffer(request, constraints);
  }

  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise RTCPeerConnection::createAnswer(ScriptState* script_state,
                                              const RTCAnswerOptions& options) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestPromiseImpl::Create(this, resolver);
  peer_handler_->CreateAnswer(request, ConvertToWebRTCAnswerOptions(options));
  return promise;
}

ScriptPromise RTCPeerConnection::createAnswer(
    ScriptState* script_state,
    V8RTCSessionDescriptionCallback* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback,
    const Dictionary& media_constraints) {
  DCHECK(success_callback);
  DCHECK(error_callback);
  ExecutionContext* context = ExecutionContext::From(script_state);
  UseCounter::Count(
      context, WebFeature::kRTCPeerConnectionCreateAnswerLegacyFailureCallback);
  if (media_constraints.IsObject()) {
    UseCounter::Count(
        context, WebFeature::kRTCPeerConnectionCreateAnswerLegacyConstraints);
  } else {
    UseCounter::Count(
        context, WebFeature::kRTCPeerConnectionCreateAnswerLegacyCompliant);
  }

  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  MediaErrorState media_error_state;
  WebMediaConstraints constraints = MediaConstraintsImpl::Create(
      context, media_constraints, media_error_state);
  // Report constraints parsing errors via the callback, but ignore
  // unknown/unsupported constraints as they would be silently discarded by
  // WebIDL.
  if (media_error_state.CanGenerateException()) {
    String error_msg = media_error_state.GetErrorMessage();
    AsyncCallErrorCallback(error_callback,
                           DOMException::Create(kOperationError, error_msg));
    return ScriptPromise::CastUndefined(script_state);
  }

  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestImpl::Create(
          GetExecutionContext(), this, success_callback, error_callback);
  peer_handler_->CreateAnswer(request, constraints);
  return ScriptPromise::CastUndefined(script_state);
}

DOMException* RTCPeerConnection::checkSdpForStateErrors(
    ExecutionContext* context,
    const RTCSessionDescriptionInit& session_description_init,
    String* sdp) {
  if (signaling_state_ == kSignalingStateClosed) {
    return DOMException::Create(kInvalidStateError,
                                kSignalingStateClosedMessage);
  }

  *sdp = session_description_init.sdp();
  if (session_description_init.type() == "offer") {
    if (sdp->IsNull() || sdp->IsEmpty()) {
      *sdp = last_offer_;
    } else if (session_description_init.sdp() != last_offer_) {
      if (FingerprintMismatch(last_offer_, *sdp)) {
        return DOMException::Create(kInvalidModificationError,
                                    kModifiedSdpMessage);
      } else {
        UseCounter::Count(context, WebFeature::kRTCLocalSdpModification);
        return nullptr;
        // TODO(https://crbug.com/823036): Return failure for all modification.
      }
    }
  } else if (session_description_init.type() == "answer" ||
             session_description_init.type() == "pranswer") {
    if (sdp->IsNull() || sdp->IsEmpty()) {
      *sdp = last_answer_;
    } else if (session_description_init.sdp() != last_answer_) {
      if (FingerprintMismatch(last_answer_, *sdp)) {
        return DOMException::Create(kInvalidModificationError,
                                    kModifiedSdpMessage);
      } else {
        UseCounter::Count(context, WebFeature::kRTCLocalSdpModification);
        return nullptr;
        // TODO(https://crbug.com/823036): Return failure for all modification.
      }
    }
  }
  return nullptr;
}

ScriptPromise RTCPeerConnection::setLocalDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init) {
  String sdp;
  DOMException* exception = checkSdpForStateErrors(
      ExecutionContext::From(script_state), session_description_init, &sdp);
  if (exception) {
    return ScriptPromise::RejectWithDOMException(script_state, exception);
  }
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCVoidRequest* request = RTCVoidRequestPromiseImpl::Create(this, resolver);
  peer_handler_->SetLocalDescription(
      request, WebRTCSessionDescription(session_description_init.type(), sdp));
  return promise;
}

ScriptPromise RTCPeerConnection::setLocalDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (success_callback && error_callback) {
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionSetLocalDescriptionLegacyCompliant);
  } else {
    if (!success_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetLocalDescriptionLegacyNoSuccessCallback);
    if (!error_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetLocalDescriptionLegacyNoFailureCallback);
  }

  String sdp;
  DOMException* exception =
      checkSdpForStateErrors(context, session_description_init, &sdp);
  if (exception) {
    if (error_callback)
      AsyncCallErrorCallback(error_callback, exception);
    return ScriptPromise::CastUndefined(script_state);
  }

  RTCVoidRequest* request = RTCVoidRequestImpl::Create(
      GetExecutionContext(), this, success_callback, error_callback);
  peer_handler_->SetLocalDescription(
      request, WebRTCSessionDescription(session_description_init.type(),
                                        session_description_init.sdp()));
  return ScriptPromise::CastUndefined(script_state);
}

RTCSessionDescription* RTCPeerConnection::localDescription() {
  WebRTCSessionDescription web_session_description =
      peer_handler_->LocalDescription();
  if (web_session_description.IsNull())
    return nullptr;

  return RTCSessionDescription::Create(web_session_description);
}

ScriptPromise RTCPeerConnection::setRemoteDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCVoidRequest* request = RTCVoidRequestPromiseImpl::Create(this, resolver);
  peer_handler_->SetRemoteDescription(
      request, WebRTCSessionDescription(session_description_init.type(),
                                        session_description_init.sdp()));
  return promise;
}

ScriptPromise RTCPeerConnection::setRemoteDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (success_callback && error_callback) {
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionSetRemoteDescriptionLegacyCompliant);
  } else {
    if (!success_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetRemoteDescriptionLegacyNoSuccessCallback);
    if (!error_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetRemoteDescriptionLegacyNoFailureCallback);
  }

  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  RTCVoidRequest* request = RTCVoidRequestImpl::Create(
      GetExecutionContext(), this, success_callback, error_callback);
  peer_handler_->SetRemoteDescription(
      request, WebRTCSessionDescription(session_description_init.type(),
                                        session_description_init.sdp()));
  return ScriptPromise::CastUndefined(script_state);
}

RTCSessionDescription* RTCPeerConnection::remoteDescription() {
  WebRTCSessionDescription web_session_description =
      peer_handler_->RemoteDescription();
  if (web_session_description.IsNull())
    return nullptr;

  return RTCSessionDescription::Create(web_session_description);
}

void RTCPeerConnection::setConfiguration(
    ScriptState* script_state,
    const RTCConfiguration& rtc_configuration,
    ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;

  WebRTCConfiguration configuration = ParseConfiguration(
      ExecutionContext::From(script_state), rtc_configuration, exception_state);

  if (exception_state.HadException())
    return;

  MediaErrorState media_error_state;
  if (media_error_state.HadException()) {
    media_error_state.RaiseException(exception_state);
    return;
  }

  webrtc::RTCErrorType error = peer_handler_->SetConfiguration(configuration);
  if (error != webrtc::RTCErrorType::NONE) {
    // All errors besides InvalidModification should have been detected above.
    if (error == webrtc::RTCErrorType::INVALID_MODIFICATION) {
      exception_state.ThrowDOMException(
          kInvalidModificationError,
          "Attempted to modify the PeerConnection's "
          "configuration in an unsupported way.");
    } else {
      exception_state.ThrowDOMException(
          kOperationError,
          "Could not update the PeerConnection with the given configuration.");
    }
  }
}

ScriptPromise RTCPeerConnection::generateCertificate(
    ScriptState* script_state,
    const AlgorithmIdentifier& keygen_algorithm,
    ExceptionState& exception_state) {
  // Normalize |keygenAlgorithm| with WebCrypto, making sure it is a recognized
  // AlgorithmIdentifier.
  WebCryptoAlgorithm crypto_algorithm;
  AlgorithmError error;
  if (!NormalizeAlgorithm(keygen_algorithm, kWebCryptoOperationGenerateKey,
                          crypto_algorithm, &error)) {
    // Reject generateCertificate with the same error as was produced by
    // WebCrypto. |result| is garbage collected, no need to delete.
    CryptoResultImpl* result = CryptoResultImpl::Create(script_state);
    ScriptPromise promise = result->Promise();
    result->CompleteWithError(error.error_type, error.error_details);
    return promise;
  }

  // Check if |keygenAlgorithm| contains the optional DOMTimeStamp |expires|
  // attribute.
  base::Optional<DOMTimeStamp> expires;
  if (keygen_algorithm.IsDictionary()) {
    Dictionary keygen_algorithm_dict = keygen_algorithm.GetAsDictionary();
    if (keygen_algorithm_dict.HasProperty("expires", exception_state)) {
      v8::Local<v8::Value> expires_value;
      keygen_algorithm_dict.Get("expires", expires_value);
      if (expires_value->IsNumber()) {
        double expires_double =
            expires_value
                ->ToNumber(script_state->GetIsolate()->GetCurrentContext())
                .ToLocalChecked()
                ->Value();
        if (expires_double >= 0) {
          expires = static_cast<DOMTimeStamp>(expires_double);
        }
      }
    }
  }
  if (exception_state.HadException()) {
    return ScriptPromise();
  }

  // Convert from WebCrypto representation to recognized WebRTCKeyParams. WebRTC
  // supports a small subset of what are valid AlgorithmIdentifiers.
  const char* unsupported_params_string =
      "The 1st argument provided is an AlgorithmIdentifier with a supported "
      "algorithm name, but the parameters are not supported.";
  base::Optional<WebRTCKeyParams> key_params;
  switch (crypto_algorithm.Id()) {
    case kWebCryptoAlgorithmIdRsaSsaPkcs1v1_5:
      // name: "RSASSA-PKCS1-v1_5"
      unsigned public_exponent;
      // "publicExponent" must fit in an unsigned int. The only recognized
      // "hash" is "SHA-256".
      if (crypto_algorithm.RsaHashedKeyGenParams()
              ->ConvertPublicExponentToUnsigned(public_exponent) &&
          crypto_algorithm.RsaHashedKeyGenParams()->GetHash().Id() ==
              kWebCryptoAlgorithmIdSha256) {
        unsigned modulus_length =
            crypto_algorithm.RsaHashedKeyGenParams()->ModulusLengthBits();
        key_params =
            WebRTCKeyParams::CreateRSA(modulus_length, public_exponent);
      } else {
        return ScriptPromise::RejectWithDOMException(
            script_state, DOMException::Create(kNotSupportedError,
                                               unsupported_params_string));
      }
      break;
    case kWebCryptoAlgorithmIdEcdsa:
      // name: "ECDSA"
      // The only recognized "namedCurve" is "P-256".
      if (crypto_algorithm.EcKeyGenParams()->NamedCurve() ==
          kWebCryptoNamedCurveP256) {
        key_params = WebRTCKeyParams::CreateECDSA(kWebRTCECCurveNistP256);
      } else {
        return ScriptPromise::RejectWithDOMException(
            script_state, DOMException::Create(kNotSupportedError,
                                               unsupported_params_string));
      }
      break;
    default:
      return ScriptPromise::RejectWithDOMException(
          script_state, DOMException::Create(kNotSupportedError,
                                             "The 1st argument provided is an "
                                             "AlgorithmIdentifier, but the "
                                             "algorithm is not supported."));
      break;
  }
  DCHECK(key_params.has_value());

  std::unique_ptr<WebRTCCertificateGenerator> certificate_generator =
      Platform::Current()->CreateRTCCertificateGenerator();

  // |keyParams| was successfully constructed, but does the certificate
  // generator support these parameters?
  if (!certificate_generator->IsSupportedKeyParams(key_params.value())) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kNotSupportedError, unsupported_params_string));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  std::unique_ptr<WebRTCCertificateObserver> certificate_observer(
      WebRTCCertificateObserver::Create(resolver));

  // Generate certificate. The |certificateObserver| will resolve the promise
  // asynchronously upon completion. The observer will manage its own
  // destruction as well as the resolver's destruction.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      ExecutionContext::From(script_state)
          ->GetTaskRunner(blink::TaskType::kInternalMedia);
  if (!expires) {
    certificate_generator->GenerateCertificate(
        key_params.value(), std::move(certificate_observer), task_runner);
  } else {
    certificate_generator->GenerateCertificateWithExpiration(
        key_params.value(), expires.value(), std::move(certificate_observer),
        task_runner);
  }

  return promise;
}

ScriptPromise RTCPeerConnection::addIceCandidate(
    ScriptState* script_state,
    const RTCIceCandidateInitOrRTCIceCandidate& candidate) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  if (IsIceCandidateMissingSdp(candidate))
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(
            script_state->GetIsolate(),
            "Candidate missing values for both sdpMid and sdpMLineIndex"));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCVoidRequest* request = RTCVoidRequestPromiseImpl::Create(this, resolver);
  scoped_refptr<WebRTCICECandidate> web_candidate = ConvertToWebRTCIceCandidate(
      ExecutionContext::From(script_state), candidate);
  bool implemented =
      peer_handler_->AddICECandidate(request, std::move(web_candidate));
  if (!implemented)
    resolver->Reject(DOMException::Create(
        kOperationError, "This operation could not be completed."));

  return promise;
}

ScriptPromise RTCPeerConnection::addIceCandidate(
    ScriptState* script_state,
    const RTCIceCandidateInitOrRTCIceCandidate& candidate,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  DCHECK(success_callback);
  DCHECK(error_callback);

  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  if (IsIceCandidateMissingSdp(candidate))
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(
            script_state->GetIsolate(),
            "Candidate missing values for both sdpMid and sdpMLineIndex"));

  RTCVoidRequest* request = RTCVoidRequestImpl::Create(
      GetExecutionContext(), this, success_callback, error_callback);
  scoped_refptr<WebRTCICECandidate> web_candidate = ConvertToWebRTCIceCandidate(
      ExecutionContext::From(script_state), candidate);
  bool implemented =
      peer_handler_->AddICECandidate(request, std::move(web_candidate));
  if (!implemented)
    AsyncCallErrorCallback(
        error_callback,
        DOMException::Create(kOperationError,
                             "This operation could not be completed."));

  return ScriptPromise::CastUndefined(script_state);
}

String RTCPeerConnection::signalingState() const {
  switch (signaling_state_) {
    case kSignalingStateStable:
      return "stable";
    case kSignalingStateHaveLocalOffer:
      return "have-local-offer";
    case kSignalingStateHaveRemoteOffer:
      return "have-remote-offer";
    case kSignalingStateHaveLocalPrAnswer:
      return "have-local-pranswer";
    case kSignalingStateHaveRemotePrAnswer:
      return "have-remote-pranswer";
    case kSignalingStateClosed:
      return "closed";
  }

  NOTREACHED();
  return String();
}

String RTCPeerConnection::iceGatheringState() const {
  switch (ice_gathering_state_) {
    case kICEGatheringStateNew:
      return "new";
    case kICEGatheringStateGathering:
      return "gathering";
    case kICEGatheringStateComplete:
      return "complete";
  }

  NOTREACHED();
  return String();
}

String RTCPeerConnection::iceConnectionState() const {
  switch (ice_connection_state_) {
    case kICEConnectionStateNew:
      return "new";
    case kICEConnectionStateChecking:
      return "checking";
    case kICEConnectionStateConnected:
      return "connected";
    case kICEConnectionStateCompleted:
      return "completed";
    case kICEConnectionStateFailed:
      return "failed";
    case kICEConnectionStateDisconnected:
      return "disconnected";
    case kICEConnectionStateClosed:
      return "closed";
  }

  NOTREACHED();
  return String();
}

void RTCPeerConnection::addStream(ScriptState* script_state,
                                  MediaStream* stream,
                                  const Dictionary& media_constraints,
                                  ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;
  if (!media_constraints.IsUndefinedOrNull()) {
    MediaErrorState media_error_state;
    WebMediaConstraints constraints =
        MediaConstraintsImpl::Create(ExecutionContext::From(script_state),
                                     media_constraints, media_error_state);
    if (media_error_state.HadException()) {
      media_error_state.RaiseException(exception_state);
      return;
    }
    LOG(WARNING)
        << "mediaConstraints is not a supported argument to addStream.";
    LOG(WARNING) << "mediaConstraints was " << constraints.ToString().Utf8();
  }

  MediaStreamVector streams;
  streams.push_back(stream);
  for (const auto& track : stream->getTracks()) {
    addTrack(track, streams, exception_state);
    exception_state.ClearException();
  }

  stream->RegisterObserver(this);
}

void RTCPeerConnection::removeStream(MediaStream* stream,
                                     ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;
  for (const auto& track : stream->getTracks()) {
    auto* sender = FindSenderForTrackAndStream(track, stream);
    if (!sender)
      continue;
    removeTrack(sender, exception_state);
    exception_state.ClearException();
  }
  stream->UnregisterObserver(this);
}

String RTCPeerConnection::id(ScriptState* script_state) const {
  DCHECK(OriginTrials::rtcPeerConnectionIdEnabled(
      ExecutionContext::From(script_state)));
  return peer_handler_->Id();
}

MediaStreamVector RTCPeerConnection::getLocalStreams() const {
  MediaStreamVector local_streams;
  for (const auto& rtp_sender : getSenders()) {
    for (const auto& stream : rtp_sender->streams()) {
      if (!local_streams.Contains(stream))
        local_streams.push_back(stream);
    }
  }
  return local_streams;
}

MediaStreamVector RTCPeerConnection::getRemoteStreams() const {
  MediaStreamVector remote_streams;
  for (const auto& rtp_receiver : rtp_receivers_) {
    for (const auto& stream : rtp_receiver->streams()) {
      if (!remote_streams.Contains(stream))
        remote_streams.push_back(stream);
    }
  }
  return remote_streams;
}

MediaStream* RTCPeerConnection::getRemoteStream(
    MediaStreamDescriptor* descriptor) const {
  for (const auto& rtp_receiver : rtp_receivers_) {
    for (const auto& stream : rtp_receiver->streams()) {
      if (stream->Descriptor() == descriptor)
        return stream;
    }
  }
  return nullptr;
}

size_t RTCPeerConnection::getRemoteStreamUsageCount(
    MediaStreamDescriptor* descriptor) const {
  size_t usage_count = 0;
  for (const auto& receiver : rtp_receivers_) {
    WebVector<WebMediaStream> streams = receiver->web_receiver().Streams();
    for (const WebMediaStream& stream : streams) {
      if (stream == descriptor)
        ++usage_count;
    }
  }
  return usage_count;
}

ScriptPromise RTCPeerConnection::getStats(
    ScriptState* script_state,
    blink::ScriptValue callback_or_selector) {
  auto argument = callback_or_selector.V8Value();
  // Custom binding for legacy "getStats(RTCStatsCallback callback)".
  if (argument->IsFunction()) {
    V8RTCStatsCallback* success_callback =
        V8RTCStatsCallback::Create(argument.As<v8::Function>());
    return LegacyCallbackBasedGetStats(script_state, success_callback, nullptr);
  }
  // Custom binding for spec-compliant "getStats()" and "getStats(undefined)".
  if (argument->IsUndefined())
    return PromiseBasedGetStats(script_state, nullptr);
  auto* isolate = callback_or_selector.GetIsolate();
  // Custom binding for spec-compliant "getStats(MediaStreamTrack? selector)".
  // null is a valid selector value, but value of wrong type isn't. |selector|
  // set to no value means type error.
  base::Optional<MediaStreamTrack*> selector;
  if (argument->IsNull()) {
    selector = base::Optional<MediaStreamTrack*>(nullptr);
  } else {
    MediaStreamTrack* track =
        V8MediaStreamTrack::ToImplWithTypeCheck(isolate, argument);
    if (track)
      selector = base::Optional<MediaStreamTrack*>(track);
  }
  if (selector.has_value())
    return PromiseBasedGetStats(script_state, *selector);
  ExceptionState exception_state(isolate, ExceptionState::kExecutionContext,
                                 "RTCPeerConnection", "getStats");
  exception_state.ThrowTypeError(
      "The argument provided as parameter 1 is neither a callback (function) "
      "or selector (MediaStreamTrack or null).");
  return exception_state.Reject(script_state);
}

ScriptPromise RTCPeerConnection::getStats(ScriptState* script_state,
                                          V8RTCStatsCallback* success_callback,
                                          MediaStreamTrack* selector) {
  return LegacyCallbackBasedGetStats(script_state, success_callback, selector);
}

ScriptPromise RTCPeerConnection::getStats(ScriptState* script_state,
                                          MediaStreamTrack* selector) {
  return PromiseBasedGetStats(script_state, selector);
}

ScriptPromise RTCPeerConnection::LegacyCallbackBasedGetStats(
    ScriptState* script_state,
    V8RTCStatsCallback* success_callback,
    MediaStreamTrack* selector) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  UseCounter::Count(context,
                    WebFeature::kRTCPeerConnectionGetStatsLegacyNonCompliant);
  RTCStatsRequest* stats_request = RTCStatsRequestImpl::Create(
      GetExecutionContext(), this, success_callback, selector);
  // FIXME: Add passing selector as part of the statsRequest.
  peer_handler_->GetStats(stats_request);

  resolver->Resolve();
  return promise;
}

ScriptPromise RTCPeerConnection::PromiseBasedGetStats(
    ScriptState* script_state,
    MediaStreamTrack* selector) {
  if (!selector) {
    ExecutionContext* context = ExecutionContext::From(script_state);
    UseCounter::Count(context, WebFeature::kRTCPeerConnectionGetStats);

    ScriptPromiseResolver* resolver =
        ScriptPromiseResolver::Create(script_state);
    ScriptPromise promise = resolver->Promise();
    peer_handler_->GetStats(
        WebRTCStatsReportCallbackResolver::Create(resolver));

    return promise;
  }

  // Find the sender or receiver that represent the selector.
  size_t track_uses = 0u;
  RTCRtpSender* track_sender = nullptr;
  for (const auto& sender : rtp_senders_) {
    if (sender->track() == selector) {
      ++track_uses;
      track_sender = sender;
    }
  }
  RTCRtpReceiver* track_receiver = nullptr;
  for (const auto& receiver : rtp_receivers_) {
    if (receiver->track() == selector) {
      ++track_uses;
      track_receiver = receiver;
    }
  }
  if (track_uses == 0u) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidAccessError,
                             "There is no sender or receiver for the track."));
  }
  if (track_uses > 1u) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(
            kInvalidAccessError,
            "There are more than one sender or receiver for the track."));
  }
  // There is just one use of the track, a sender or receiver.
  if (track_sender) {
    DCHECK(!track_receiver);
    return track_sender->getStats(script_state);
  }
  DCHECK(track_receiver);
  return track_receiver->getStats(script_state);
}

const HeapVector<Member<RTCRtpSender>>& RTCPeerConnection::getSenders() const {
  return rtp_senders_;
}

const HeapVector<Member<RTCRtpReceiver>>& RTCPeerConnection::getReceivers()
    const {
  return rtp_receivers_;
}

RTCRtpSender* RTCPeerConnection::addTrack(MediaStreamTrack* track,
                                          MediaStreamVector streams,
                                          ExceptionState& exception_state) {
  DCHECK(track);
  DCHECK(track->Component());
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return nullptr;
  // TODO(bugs.webrtc.org/8530): Take out WebRTCSdpSemantics::kDefault check
  // once default is no longer interpreted as Plan B lower down.
  if ((sdp_semantics_ == WebRTCSdpSemantics::kPlanB ||
       sdp_semantics_ == WebRTCSdpSemantics::kDefault) &&
      streams.size() >= 2) {
    // TODO(hbos): Update peer_handler_ to call the AddTrack() that returns the
    // appropriate errors, and let the lower layers handle it.
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "Adding a track to multiple streams is not supported.");
    return nullptr;
  }
  for (const auto& sender : rtp_senders_) {
    if (sender->track() == track) {
      exception_state.ThrowDOMException(
          kInvalidAccessError, "A sender already exists for the track.");
      return nullptr;
    }
  }

  WebVector<WebMediaStream> web_streams(streams.size());
  for (size_t i = 0; i < streams.size(); ++i) {
    web_streams[i] = streams[i]->Descriptor();
  }
  std::unique_ptr<WebRTCRtpSender> web_rtp_sender =
      peer_handler_->AddTrack(track->Component(), web_streams);
  if (!web_rtp_sender) {
    exception_state.ThrowDOMException(
        kNotSupportedError, "A sender could not be created for this track.");
    return nullptr;
  }

  DCHECK(FindSender(*web_rtp_sender) == rtp_senders_.end());
  RTCRtpSender* rtp_sender =
      new RTCRtpSender(this, std::move(web_rtp_sender), track, streams);
  tracks_.insert(track->Component(), track);
  rtp_senders_.push_back(rtp_sender);
  return rtp_sender;
}

void RTCPeerConnection::removeTrack(RTCRtpSender* sender,
                                    ExceptionState& exception_state) {
  DCHECK(sender);
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;
  auto* it = FindSender(*sender->web_sender());
  if (it == rtp_senders_.end()) {
    exception_state.ThrowDOMException(
        kInvalidAccessError,
        "The sender was not created by this peer connection.");
    return;
  }

  if (!peer_handler_->RemoveTrack(sender->web_sender())) {
    // Operation aborted. This indicates that the sender is no longer used by
    // the peer connection, i.e. that it was removed due to setting a remote
    // description of type "rollback".
    // Note: Until the WebRTC library supports re-using senders, a sender will
    // also stop being used as a result of being removed.
    return;
  }
  // Successfully removing the track results in the sender's track property
  // being nulled.
  DCHECK(!sender->web_sender()->Track());
  sender->SetTrack(nullptr);
  rtp_senders_.erase(it);
}

RTCDataChannel* RTCPeerConnection::createDataChannel(
    ScriptState* script_state,
    String label,
    const RTCDataChannelInit& data_channel_dict,
    ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return nullptr;

  WebRTCDataChannelInit init;
  init.ordered = data_channel_dict.ordered();
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (data_channel_dict.hasMaxRetransmitTime()) {
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionCreateDataChannelMaxRetransmitTime);
    init.max_retransmit_time = data_channel_dict.maxRetransmitTime();
  }
  if (data_channel_dict.hasMaxRetransmits()) {
    UseCounter::Count(
        context, WebFeature::kRTCPeerConnectionCreateDataChannelMaxRetransmits);
    init.max_retransmits = data_channel_dict.maxRetransmits();
  }
  init.protocol = data_channel_dict.protocol();
  init.negotiated = data_channel_dict.negotiated();
  if (data_channel_dict.hasId())
    init.id = data_channel_dict.id();

  RTCDataChannel* channel = RTCDataChannel::Create(
      GetExecutionContext(), peer_handler_.get(), label, init, exception_state);
  if (exception_state.HadException())
    return nullptr;
  RTCDataChannel::ReadyState handler_state = channel->GetHandlerState();
  if (handler_state != RTCDataChannel::kReadyStateConnecting) {
    // There was an early state transition.  Don't miss it!
    channel->DidChangeReadyState(handler_state);
  }
  has_data_channels_ = true;

  return channel;
}

MediaStreamTrack* RTCPeerConnection::GetTrack(
    const WebMediaStreamTrack& web_track) const {
  return tracks_.at(static_cast<MediaStreamComponent*>(web_track));
}

RTCRtpSender* RTCPeerConnection::FindSenderForTrackAndStream(
    MediaStreamTrack* track,
    MediaStream* stream) {
  for (const auto& rtp_sender : rtp_senders_) {
    if (rtp_sender->track() == track) {
      auto streams = rtp_sender->streams();
      if (streams.size() == 1u && streams[0] == stream)
        return rtp_sender;
    }
  }
  return nullptr;
}

HeapVector<Member<RTCRtpSender>>::iterator RTCPeerConnection::FindSender(
    const WebRTCRtpSender& web_sender) {
  for (auto* it = rtp_senders_.begin(); it != rtp_senders_.end(); ++it) {
    if ((*it)->web_sender()->Id() == web_sender.Id())
      return it;
  }
  return rtp_senders_.end();
}

HeapVector<Member<RTCRtpReceiver>>::iterator RTCPeerConnection::FindReceiver(
    const WebRTCRtpReceiver& web_receiver) {
  for (auto* it = rtp_receivers_.begin(); it != rtp_receivers_.end(); ++it) {
    if ((*it)->web_receiver().Id() == web_receiver.Id())
      return it;
  }
  return rtp_receivers_.end();
}

RTCDTMFSender* RTCPeerConnection::createDTMFSender(
    MediaStreamTrack* track,
    ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return nullptr;
  if (track->kind() != "audio") {
    exception_state.ThrowDOMException(kSyntaxError,
                                      "track.kind is not 'audio'.");
    return nullptr;
  }
  RTCRtpSender* found_rtp_sender = nullptr;
  for (const auto& rtp_sender : rtp_senders_) {
    if (rtp_sender->track() == track) {
      found_rtp_sender = rtp_sender;
      break;
    }
  }
  if (!found_rtp_sender) {
    exception_state.ThrowDOMException(
        kSyntaxError, "No RTCRtpSender is available for the track provided.");
    return nullptr;
  }
  RTCDTMFSender* dtmf_sender = found_rtp_sender->dtmf();
  if (!dtmf_sender) {
    exception_state.ThrowDOMException(kSyntaxError,
                                      "Unable to create DTMF sender for track");
    return nullptr;
  }
  dtmf_sender->SetTrack(track);
  return dtmf_sender;
}

void RTCPeerConnection::close() {
  if (signaling_state_ == RTCPeerConnection::kSignalingStateClosed)
    return;

  CloseInternal();
}

void RTCPeerConnection::NoteSdpCreated(const RTCSessionDescription& desc) {
  if (desc.type() == "offer") {
    last_offer_ = desc.sdp();
  } else if (desc.type() == "answer") {
    last_answer_ = desc.sdp();
  }
}

void RTCPeerConnection::OnStreamAddTrack(MediaStream* stream,
                                         MediaStreamTrack* track) {
  ExceptionState exception_state(v8::Isolate::GetCurrent(),
                                 ExceptionState::kExecutionContext, nullptr,
                                 nullptr);
  MediaStreamVector streams;
  streams.push_back(stream);
  addTrack(track, streams, exception_state);
  // If addTrack() failed most likely the track already has a sender and this is
  // a NO-OP or the connection is closed. The exception can be suppressed, there
  // is nothing to do.
  exception_state.ClearException();
}

void RTCPeerConnection::OnStreamRemoveTrack(MediaStream* stream,
                                            MediaStreamTrack* track) {
  auto* sender = FindSenderForTrackAndStream(track, stream);
  if (sender) {
    ExceptionState exception_state(v8::Isolate::GetCurrent(),
                                   ExceptionState::kExecutionContext, nullptr,
                                   nullptr);
    removeTrack(sender, exception_state);
    // If removeTracl() failed most likely the connection is closed. The
    // exception can be suppressed, there is nothing to do.
    exception_state.ClearException();
  }
}

void RTCPeerConnection::NegotiationNeeded() {
  DCHECK(!closed_);
  negotiation_needed_ = true;
  Microtask::EnqueueMicrotask(
      WTF::Bind(&RTCPeerConnection::MaybeFireNegotiationNeeded,
                WrapWeakPersistent(this)));
}

void RTCPeerConnection::MaybeFireNegotiationNeeded() {
  if (!negotiation_needed_ || closed_)
    return;
  negotiation_needed_ = false;
  DispatchEvent(Event::Create(EventTypeNames::negotiationneeded));
}

void RTCPeerConnection::DidGenerateICECandidate(
    scoped_refptr<WebRTCICECandidate> web_candidate) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  DCHECK(web_candidate);
  RTCIceCandidate* ice_candidate =
      RTCIceCandidate::Create(std::move(web_candidate));
  ScheduleDispatchEvent(RTCPeerConnectionIceEvent::Create(ice_candidate));
}

void RTCPeerConnection::DidChangeSignalingState(SignalingState new_state) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  ChangeSignalingState(new_state);
}

void RTCPeerConnection::DidChangeICEGatheringState(
    ICEGatheringState new_state) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  ChangeIceGatheringState(new_state);
}

void RTCPeerConnection::DidChangeICEConnectionState(
    ICEConnectionState new_state) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  ChangeIceConnectionState(new_state);
}

void RTCPeerConnection::DidAddRemoteTrack(
    std::unique_ptr<WebRTCRtpReceiver> web_rtp_receiver) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  if (signaling_state_ == kSignalingStateClosed)
    return;
  HeapVector<Member<MediaStream>> streams;
  WebVector<WebMediaStream> web_streams = web_rtp_receiver->Streams();
  streams.ReserveCapacity(web_streams.size());
  for (const WebMediaStream& web_stream : web_streams) {
    MediaStream* stream = getRemoteStream(web_stream);
    if (!stream) {
      // This is a new stream that we need to create.
      // Get or create audio tracks.
      WebVector<WebMediaStreamTrack> audio_web_tracks;
      web_stream.AudioTracks(audio_web_tracks);
      MediaStreamTrackVector audio_tracks;
      audio_tracks.ReserveCapacity(audio_web_tracks.size());
      for (const WebMediaStreamTrack& audio_web_track : audio_web_tracks) {
        MediaStreamTrack* audio_track = tracks_.at(audio_web_track);
        if (!audio_track) {
          audio_track =
              MediaStreamTrack::Create(GetExecutionContext(), audio_web_track);
          tracks_.insert(audio_track->Component(), audio_track);
        }
        audio_tracks.push_back(audio_track);
      }
      // Get or create video tracks.
      WebVector<WebMediaStreamTrack> video_web_tracks;
      web_stream.VideoTracks(video_web_tracks);
      MediaStreamTrackVector video_tracks;
      video_tracks.ReserveCapacity(video_web_tracks.size());
      for (const WebMediaStreamTrack& video_web_track : video_web_tracks) {
        MediaStreamTrack* video_track = tracks_.at(video_web_track);
        if (!video_track) {
          video_track =
              MediaStreamTrack::Create(GetExecutionContext(), video_web_track);
          tracks_.insert(video_track->Component(), video_track);
        }
        video_tracks.push_back(video_track);
      }
      // Create stream with tracks.
      stream = MediaStream::Create(GetExecutionContext(), web_stream,
                                   audio_tracks, video_tracks);
      stream->RegisterObserver(this);
      ScheduleDispatchEvent(
          MediaStreamEvent::Create(EventTypeNames::addstream, stream));
    } else {
      // The stream already exists. Because the blink stream is wired up to
      // reflect when web tracks are added to the corresponding web stream, the
      // receiver's track will already have a blink track created for it and
      // added to the blink stream. Find it and add it to |tracks_| so that the
      // RTCPeerConnection knows of its existence.
      // TODO(hbos): This wiring is problematic since it assumes the blink track
      // should always be created. If the track already exists (on some other
      // stream or receiver) we will end up with multiple blink tracks for the
      // same component. When a web track is added to the web stream, we need to
      // check if a blink track already exists for it by querying the
      // RTCPeerConnection. https://crbug.com/769743
      MediaStreamTrack* receiver_track = nullptr;
      for (const auto& track : stream->getTracks()) {
        if (track->Component() == web_rtp_receiver->Track()) {
          receiver_track = track;
          break;
        }
      }
      DCHECK(receiver_track);
      tracks_.insert(receiver_track->Component(), receiver_track);
    }
    streams.push_back(stream);
  }
  DCHECK(FindReceiver(*web_rtp_receiver) == rtp_receivers_.end());
  MediaStreamTrack* track = GetTrack(web_rtp_receiver->Track());
  if (!track) {
    // Receiver with track, without a stream. May be created by Unified Plan.
    track = MediaStreamTrack::Create(GetExecutionContext(),
                                     web_rtp_receiver->Track());
  }
  RTCRtpReceiver* rtp_receiver =
      new RTCRtpReceiver(std::move(web_rtp_receiver), track, streams);
  rtp_receivers_.push_back(rtp_receiver);
  ScheduleDispatchEvent(
      new RTCTrackEvent(rtp_receiver, rtp_receiver->track(), streams));
}

void RTCPeerConnection::DidRemoveRemoteTrack(
    std::unique_ptr<WebRTCRtpReceiver> web_rtp_receiver) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());

  WebVector<WebMediaStream> web_streams = web_rtp_receiver->Streams();
  auto* it = FindReceiver(*web_rtp_receiver);
  DCHECK(it != rtp_receivers_.end());
  RTCRtpReceiver* rtp_receiver = *it;
  MediaStreamTrack* track = rtp_receiver->track();
  rtp_receivers_.erase(it);

  // End streams no longer in use and fire "removestream" events. This behavior
  // is no longer in the spec.
  for (const WebMediaStream& web_stream : web_streams) {
    MediaStreamDescriptor* stream_descriptor = web_stream;
    DCHECK(stream_descriptor->Client());
    MediaStream* stream =
        static_cast<MediaStream*>(stream_descriptor->Client());

    // The track should already have been removed from the stream thanks to
    // wiring listening to the webrtc layer stream. This should make sure the
    // "removetrack" event fires.
    DCHECK(!stream->getTracks().Contains(track));

    // Was this the last usage of the stream? Remove from remote streams.
    if (!getRemoteStreamUsageCount(web_stream)) {
      // TODO(hbos): The stream should already have ended by being empty, no
      // need for |StreamEnded|.
      stream->StreamEnded();
      stream->UnregisterObserver(this);
      ScheduleDispatchEvent(
          MediaStreamEvent::Create(EventTypeNames::removestream, stream));
    }
  }

  // Mute track and fire "onmute" if not already muted.
  track->Component()->Source()->SetReadyState(
      MediaStreamSource::kReadyStateMuted);
}

void RTCPeerConnection::DidAddRemoteDataChannel(
    WebRTCDataChannelHandler* handler) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());

  if (signaling_state_ == kSignalingStateClosed)
    return;

  RTCDataChannel* channel =
      RTCDataChannel::Create(GetExecutionContext(), base::WrapUnique(handler));
  ScheduleDispatchEvent(
      RTCDataChannelEvent::Create(EventTypeNames::datachannel, channel));
  has_data_channels_ = true;
}

void RTCPeerConnection::ReleasePeerConnectionHandler() {
  if (stopped_)
    return;

  stopped_ = true;
  ice_connection_state_ = kICEConnectionStateClosed;
  signaling_state_ = kSignalingStateClosed;

  dispatch_scheduled_event_runner_->Stop();

  peer_handler_.reset();

  connection_handle_for_scheduler_.reset();
}

void RTCPeerConnection::ClosePeerConnection() {
  DCHECK(signaling_state_ != RTCPeerConnection::kSignalingStateClosed);
  CloseInternal();
}

RTCPeerConnection::WebRTCOriginTrials RTCPeerConnection::GetOriginTrials() {
  RTCPeerConnection::WebRTCOriginTrials trials;
  trials.vaapi_hwvp8_encoding_enabled =
      OriginTrials::webRtcVaapiHWVP8EncodingEnabled(GetExecutionContext());
  return trials;
}

const AtomicString& RTCPeerConnection::InterfaceName() const {
  return EventTargetNames::RTCPeerConnection;
}

ExecutionContext* RTCPeerConnection::GetExecutionContext() const {
  return PausableObject::GetExecutionContext();
}

void RTCPeerConnection::Pause() {
  dispatch_scheduled_event_runner_->Pause();
}

void RTCPeerConnection::Unpause() {
  dispatch_scheduled_event_runner_->Unpause();
}

void RTCPeerConnection::ContextDestroyed(ExecutionContext*) {
  ReleasePeerConnectionHandler();
}

void RTCPeerConnection::ChangeSignalingState(SignalingState signaling_state) {
  if (signaling_state_ != kSignalingStateClosed) {
    signaling_state_ = signaling_state;
    ScheduleDispatchEvent(Event::Create(EventTypeNames::signalingstatechange));
  }
}

void RTCPeerConnection::ChangeIceGatheringState(
    ICEGatheringState ice_gathering_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed) {
    ScheduleDispatchEvent(
        Event::Create(EventTypeNames::icegatheringstatechange),
        WTF::Bind(&RTCPeerConnection::SetIceGatheringState,
                  WrapPersistent(this), ice_gathering_state));
    if (ice_gathering_state == kICEGatheringStateComplete) {
      // If ICE gathering is completed, generate a null ICE candidate, to
      // signal end of candidates.
      ScheduleDispatchEvent(RTCPeerConnectionIceEvent::Create(nullptr));
    }
  }
}

bool RTCPeerConnection::SetIceGatheringState(
    ICEGatheringState ice_gathering_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed &&
      ice_gathering_state_ != ice_gathering_state) {
    ice_gathering_state_ = ice_gathering_state;
    return true;
  }
  return false;
}

void RTCPeerConnection::ChangeIceConnectionState(
    ICEConnectionState ice_connection_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed) {
    ScheduleDispatchEvent(
        Event::Create(EventTypeNames::iceconnectionstatechange),
        WTF::Bind(&RTCPeerConnection::SetIceConnectionState,
                  WrapPersistent(this), ice_connection_state));
  }
}

bool RTCPeerConnection::SetIceConnectionState(
    ICEConnectionState ice_connection_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed &&
      ice_connection_state_ != ice_connection_state) {
    ice_connection_state_ = ice_connection_state;
    if (ice_connection_state_ == kICEConnectionStateConnected)
      RecordRapporMetrics();

    return true;
  }
  return false;
}

void RTCPeerConnection::CloseInternal() {
  DCHECK(signaling_state_ != RTCPeerConnection::kSignalingStateClosed);
  peer_handler_->Stop();
  closed_ = true;

  ChangeIceConnectionState(kICEConnectionStateClosed);
  ChangeSignalingState(kSignalingStateClosed);
  Document* document = ToDocument(GetExecutionContext());
  HostsUsingFeatures::CountAnyWorld(
      *document, HostsUsingFeatures::Feature::kRTCPeerConnectionUsed);

  connection_handle_for_scheduler_.reset();
}

void RTCPeerConnection::ScheduleDispatchEvent(Event* event) {
  ScheduleDispatchEvent(event, BoolFunction());
}

void RTCPeerConnection::ScheduleDispatchEvent(Event* event,
                                              BoolFunction setup_function) {
  scheduled_events_.push_back(
      new EventWrapper(event, std::move(setup_function)));

  dispatch_scheduled_event_runner_->RunAsync();
}

void RTCPeerConnection::DispatchScheduledEvent() {
  if (stopped_)
    return;

  HeapVector<Member<EventWrapper>> events;
  events.swap(scheduled_events_);

  HeapVector<Member<EventWrapper>>::iterator it = events.begin();
  for (; it != events.end(); ++it) {
    if ((*it)->Setup()) {
      DispatchEvent((*it)->event_.Release());
    }
  }

  events.clear();
}

void RTCPeerConnection::RecordRapporMetrics() {
  Document* document = ToDocument(GetExecutionContext());
  for (const auto& component : tracks_.Keys()) {
    switch (component->Source()->GetType()) {
      case MediaStreamSource::kTypeAudio:
        HostsUsingFeatures::CountAnyWorld(
            *document, HostsUsingFeatures::Feature::kRTCPeerConnectionAudio);
        break;
      case MediaStreamSource::kTypeVideo:
        HostsUsingFeatures::CountAnyWorld(
            *document, HostsUsingFeatures::Feature::kRTCPeerConnectionVideo);
        break;
      default:
        NOTREACHED();
    }
  }

  if (has_data_channels_)
    HostsUsingFeatures::CountAnyWorld(
        *document, HostsUsingFeatures::Feature::kRTCPeerConnectionDataChannel);
}

void RTCPeerConnection::Trace(blink::Visitor* visitor) {
  visitor->Trace(tracks_);
  visitor->Trace(rtp_senders_);
  visitor->Trace(rtp_receivers_);
  visitor->Trace(dispatch_scheduled_event_runner_);
  visitor->Trace(scheduled_events_);
  EventTargetWithInlineData::Trace(visitor);
  PausableObject::Trace(visitor);
  MediaStreamObserver::Trace(visitor);
}

int RTCPeerConnection::PeerConnectionCount() {
  return InstanceCounters::CounterValue(
      InstanceCounters::kRTCPeerConnectionCounter);
}

int RTCPeerConnection::PeerConnectionCountLimit() {
  return kMaxPeerConnections;
}

}  // namespace blink
