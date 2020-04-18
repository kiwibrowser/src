// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_rtp_sender.h"
#include "third_party/blink/public/platform/web_rtc_dtmf_sender_handler.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_track.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_dtmf_sender.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_error_util.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_void_request_script_promise_resolver_impl.h"
#include "third_party/blink/renderer/modules/peerconnection/web_rtc_stats_report_callback_resolver.h"
#include "third_party/blink/renderer/platform/peerconnection/rtc_void_request.h"

namespace blink {

namespace {

class ReplaceTrackRequest : public RTCVoidRequest {
 public:
  ReplaceTrackRequest(RTCRtpSender* sender,
                      MediaStreamTrack* with_track,
                      ScriptPromiseResolver* resolver)
      : sender_(sender), with_track_(with_track), resolver_(resolver) {}
  ~ReplaceTrackRequest() override {}

  void RequestSucceeded() override {
    sender_->SetTrack(with_track_);
    resolver_->Resolve();
  }

  void RequestFailed(const webrtc::RTCError& error) override {
    resolver_->Reject(CreateDOMExceptionFromRTCError(error));
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(sender_);
    visitor->Trace(with_track_);
    visitor->Trace(resolver_);
    RTCVoidRequest::Trace(visitor);
  }

 private:
  Member<RTCRtpSender> sender_;
  Member<MediaStreamTrack> with_track_;
  Member<ScriptPromiseResolver> resolver_;
};

class SetParametersRequest : public RTCVoidRequestScriptPromiseResolverImpl {
 public:
  SetParametersRequest(ScriptPromiseResolver* resolver, RTCRtpSender* sender)
      : RTCVoidRequestScriptPromiseResolverImpl(resolver), sender_(sender) {}

  void RequestSucceeded() override {
    sender_->ClearLastReturnedParameters();
    RTCVoidRequestScriptPromiseResolverImpl::RequestSucceeded();
  }

  void RequestFailed(const webrtc::RTCError& error) override {
    sender_->ClearLastReturnedParameters();
    RTCVoidRequestScriptPromiseResolverImpl::RequestFailed(error);
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(sender_);
    RTCVoidRequestScriptPromiseResolverImpl::Trace(visitor);
  }

 private:
  Member<RTCRtpSender> sender_;
};

bool HasInvalidModification(const RTCRtpParameters& parameters,
                            const RTCRtpParameters& new_parameters) {
  if (parameters.hasTransactionId() != new_parameters.hasTransactionId() ||
      (parameters.hasTransactionId() &&
       parameters.transactionId() != new_parameters.transactionId()))
    return true;

  if (parameters.hasEncodings() != new_parameters.hasEncodings())
    return true;
  if (parameters.hasEncodings()) {
    if (parameters.encodings().size() != new_parameters.encodings().size())
      return true;
  }

  if (parameters.hasCodecs() != new_parameters.hasCodecs())
    return true;

  if (parameters.hasCodecs()) {
    if (parameters.codecs().size() != new_parameters.codecs().size())
      return true;

    for (std::size_t i = 0; i < parameters.codecs().size(); ++i) {
      const auto& codec = parameters.codecs()[i];
      const auto& new_codec = new_parameters.codecs()[i];
      if (codec.hasPayloadType() != new_codec.hasPayloadType() ||
          (codec.hasPayloadType() &&
           codec.payloadType() != new_codec.payloadType()) ||
          codec.hasMimeType() != new_codec.hasMimeType() ||
          (codec.hasMimeType() && codec.mimeType() != new_codec.mimeType()) ||
          codec.hasClockRate() != new_codec.hasClockRate() ||
          (codec.hasClockRate() &&
           codec.clockRate() != new_codec.clockRate()) ||
          codec.hasChannels() != new_codec.hasChannels() ||
          (codec.hasChannels() && codec.channels() != new_codec.channels()) ||
          codec.hasSdpFmtpLine() != new_codec.hasSdpFmtpLine() ||
          (codec.hasSdpFmtpLine() &&
           codec.sdpFmtpLine() != new_codec.sdpFmtpLine())) {
        return true;
      }
    }
  }

  return false;
}

// Relative weights for each priority as defined in RTCWEB-DATA
// https://tools.ietf.org/html/draft-ietf-rtcweb-data-channel
const double kPriorityWeightVeryLow = 0.5;
const double kPriorityWeightLow = 1;
const double kPriorityWeightMedium = 2;
const double kPriorityWeightHigh = 4;

std::string PriorityFromDouble(double priority) {
  // Find the middle point between 2 priority weights to match them to a
  // WebRTC priority
  const double very_low_upper_bound =
      (kPriorityWeightVeryLow + kPriorityWeightLow) / 2;
  const double low_upper_bound =
      (kPriorityWeightLow + kPriorityWeightMedium) / 2;
  const double medium_upper_bound =
      (kPriorityWeightMedium + kPriorityWeightHigh) / 2;

  if (priority < webrtc::kDefaultBitratePriority * very_low_upper_bound) {
    return "very-low";
  }
  if (priority < webrtc::kDefaultBitratePriority * low_upper_bound) {
    return "low";
  }
  if (priority < webrtc::kDefaultBitratePriority * medium_upper_bound) {
    return "medium";
  }
  return "high";
}

double PriorityToDouble(const WTF::String& priority) {
  double result = 1;

  if (priority == "very-low") {
    result = webrtc::kDefaultBitratePriority * kPriorityWeightVeryLow;
  } else if (priority == "low") {
    result = webrtc::kDefaultBitratePriority * kPriorityWeightLow;
  } else if (priority == "medium") {
    result = webrtc::kDefaultBitratePriority * kPriorityWeightMedium;
  } else if (priority == "high") {
    result = webrtc::kDefaultBitratePriority * kPriorityWeightHigh;
  } else {
    NOTREACHED();
  }
  return result;
}

std::tuple<std::vector<webrtc::RtpEncodingParameters>,
           webrtc::DegradationPreference>
ToRtpParameters(const RTCRtpParameters& parameters) {
  std::vector<webrtc::RtpEncodingParameters> encodings;
  if (parameters.hasEncodings()) {
    encodings.reserve(parameters.encodings().size());

    for (const auto& encoding : parameters.encodings()) {
      // TODO(orphis): Forward missing fields from the WebRTC library:
      // codecPayloadType, dtx, ptime, maxFramerate, scaleResolutionDownBy,
      // rid
      encodings.push_back({});
      encodings.back().active = encoding.active();
      encodings.back().bitrate_priority = PriorityToDouble(encoding.priority());
      if (encoding.hasMaxBitrate())
        encodings.back().max_bitrate_bps = clampTo<int>(encoding.maxBitrate());
    }
  }

  webrtc::DegradationPreference degradation_preference =
      webrtc::DegradationPreference::BALANCED;
  return std::make_tuple(encodings, degradation_preference);
}

}  // namespace

RTCRtpSender::RTCRtpSender(RTCPeerConnection* pc,
                           std::unique_ptr<WebRTCRtpSender> sender,
                           MediaStreamTrack* track,
                           MediaStreamVector streams)
    : pc_(pc),
      sender_(std::move(sender)),
      track_(track),
      streams_(std::move(streams)) {
  DCHECK(pc_);
  DCHECK(sender_);
  DCHECK(track_);
  kind_ = track->kind();
}

MediaStreamTrack* RTCRtpSender::track() {
  return track_;
}

ScriptPromise RTCRtpSender::replaceTrack(ScriptState* script_state,
                                         MediaStreamTrack* with_track) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  if (pc_->IsClosed()) {
    resolver->Reject(DOMException::Create(kInvalidStateError,
                                          "The peer connection is closed."));
    return promise;
  }
  WebMediaStreamTrack web_track;
  if (with_track)
    web_track = with_track->Component();
  ReplaceTrackRequest* request =
      new ReplaceTrackRequest(this, with_track, resolver);
  sender_->ReplaceTrack(web_track, request);
  return promise;
}

void RTCRtpSender::getParameters(RTCRtpParameters& parameters) {
  // TODO(orphis): Forward missing fields from the WebRTC library:
  // rtcp, headerExtensions, degradationPreference
  std::unique_ptr<webrtc::RtpParameters> webrtc_parameters =
      sender_->GetParameters();

  parameters.setTransactionId(webrtc_parameters->transaction_id.c_str());

  HeapVector<RTCRtpEncodingParameters> encodings;
  encodings.ReserveCapacity(webrtc_parameters->encodings.size());
  for (const auto& web_encoding : webrtc_parameters->encodings) {
    // TODO(orphis): Forward missing fields from the WebRTC library:
    // codecPayloadType, dtx, ptime, maxFramerate, scaleResolutionDownBy, rid
    encodings.emplace_back();
    RTCRtpEncodingParameters& encoding = encodings.back();
    encoding.setActive(web_encoding.active);
    if (web_encoding.max_bitrate_bps)
      encoding.setMaxBitrate(web_encoding.max_bitrate_bps.value());
    encoding.setPriority(
        PriorityFromDouble(web_encoding.bitrate_priority).c_str());
  }
  parameters.setEncodings(encodings);

  HeapVector<RTCRtpCodecParameters> codecs;
  codecs.ReserveCapacity(webrtc_parameters->codecs.size());
  for (const auto& web_codec : webrtc_parameters->codecs) {
    codecs.emplace_back();
    RTCRtpCodecParameters& codec = codecs.back();
    codec.setPayloadType(web_codec.payload_type);
    codec.setMimeType(WTF::String::FromUTF8(web_codec.mime_type().c_str()));
    if (web_codec.clock_rate)
      codec.setClockRate(web_codec.clock_rate.value());
    if (web_codec.num_channels)
      codec.setChannels(web_codec.num_channels.value());
    if (web_codec.parameters.size()) {
      std::string sdp_fmtp_line;
      for (const auto& parameter : web_codec.parameters) {
        if (sdp_fmtp_line.size())
          sdp_fmtp_line += ";";
        sdp_fmtp_line += parameter.first + "=" + parameter.second;
      }
      codec.setSdpFmtpLine(sdp_fmtp_line.c_str());
    }
  }
  parameters.setCodecs(codecs);

  last_returned_parameters_ = parameters;
}

ScriptPromise RTCRtpSender::setParameters(ScriptState* script_state,
                                          const RTCRtpParameters& parameters) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  if (!last_returned_parameters_) {
    resolver->Reject(DOMException::Create(
        kInvalidStateError,
        "getParameters() needs to be called before setParameters()."));
    return promise;
  }
  // The specification mentions that some fields in the dictionary should not
  // be modified. Some of those checks are done in the lower WebRTC layer, but
  // there is no perfect 1-1 mapping between the Javascript layer and native.
  // So we save the last returned dictionary and enforce the check at this
  // level instead.
  if (HasInvalidModification(last_returned_parameters_.value(), parameters)) {
    resolver->Reject(
        DOMException::Create(kInvalidModificationError,
                             "Read-only field modified in setParameters()."));
    return promise;
  }

  // The only values that can be set by setParameters are in the encodings
  // field and the degradationPreference field. We just forward those to the
  // native layer without having to transform all the other read-only
  // parameters.
  std::vector<webrtc::RtpEncodingParameters> encodings;
  webrtc::DegradationPreference degradation_preference;
  std::tie(encodings, degradation_preference) = ToRtpParameters(parameters);

  auto* request = new SetParametersRequest(resolver, this);
  sender_->SetParameters(std::move(encodings), degradation_preference, request);
  return promise;
}

void RTCRtpSender::ClearLastReturnedParameters() {
  last_returned_parameters_.reset();
}

ScriptPromise RTCRtpSender::getStats(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  sender_->GetStats(WebRTCStatsReportCallbackResolver::Create(resolver));
  return promise;
}

WebRTCRtpSender* RTCRtpSender::web_sender() {
  return sender_.get();
}

void RTCRtpSender::SetTrack(MediaStreamTrack* track) {
  track_ = track;
  if (track) {
    if (kind_.IsNull()) {
      kind_ = track->kind();
    } else if (kind_ != track->kind()) {
      LOG(ERROR) << "Trying to set track to a different kind: Old " << kind_
                 << " new " << track->kind();
      NOTREACHED();
    }
  }
}

MediaStreamVector RTCRtpSender::streams() const {
  return streams_;
}

RTCDTMFSender* RTCRtpSender::dtmf() {
  // Lazy initialization of dtmf_ to avoid overhead when not used.
  if (!dtmf_ && kind_ == "audio") {
    auto handler = sender_->GetDtmfSender();
    if (!handler) {
      LOG(ERROR) << "Unable to create DTMF sender attribute on an audio sender";
      return nullptr;
    }
    dtmf_ =
        RTCDTMFSender::Create(pc_->GetExecutionContext(), std::move(handler));
  }
  return dtmf_;
}

void RTCRtpSender::Trace(blink::Visitor* visitor) {
  visitor->Trace(pc_);
  visitor->Trace(track_);
  visitor->Trace(dtmf_);
  visitor->Trace(streams_);
  visitor->Trace(last_returned_parameters_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
