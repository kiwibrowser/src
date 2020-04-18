// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/rtc_peer_connection_handler.h"

#include <ctype.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "base/unguessable_token.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/renderer/media/stream/media_stream_constraints_util.h"
#include "content/renderer/media/stream/media_stream_track.h"
#include "content/renderer/media/webrtc/peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/peer_connection_tracker.h"
#include "content/renderer/media/webrtc/rtc_certificate.h"
#include "content/renderer/media/webrtc/rtc_data_channel_handler.h"
#include "content/renderer/media/webrtc/rtc_dtmf_sender_handler.h"
#include "content/renderer/media/webrtc/rtc_event_log_output_sink.h"
#include "content/renderer/media/webrtc/rtc_event_log_output_sink_proxy.h"
#include "content/renderer/media/webrtc/rtc_stats.h"
#include "content/renderer/media/webrtc/webrtc_audio_device_impl.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter.h"
#include "content/renderer/media/webrtc/webrtc_set_remote_description_observer.h"
#include "content/renderer/media/webrtc/webrtc_uma_histograms.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/media_switches.h"
#include "third_party/blink/public/platform/web_media_constraints.h"
#include "third_party/blink/public/platform/web_rtc_answer_options.h"
#include "third_party/blink/public/platform/web_rtc_configuration.h"
#include "third_party/blink/public/platform/web_rtc_data_channel_init.h"
#include "third_party/blink/public/platform/web_rtc_ice_candidate.h"
#include "third_party/blink/public/platform/web_rtc_legacy_stats.h"
#include "third_party/blink/public/platform/web_rtc_offer_options.h"
#include "third_party/blink/public/platform/web_rtc_rtp_sender.h"
#include "third_party/blink/public/platform/web_rtc_session_description.h"
#include "third_party/blink/public/platform/web_rtc_session_description_request.h"
#include "third_party/blink/public/platform/web_rtc_void_request.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/webrtc/api/rtceventlogoutput.h"
#include "third_party/webrtc/pc/mediasession.h"

using webrtc::DataChannelInterface;
using webrtc::IceCandidateInterface;
using webrtc::MediaStreamInterface;
using webrtc::PeerConnectionInterface;
using webrtc::PeerConnectionObserver;
using webrtc::StatsReport;
using webrtc::StatsReports;

namespace content {
namespace {

// Used to back histogram value of "WebRTC.PeerConnection.RtcpMux",
// so treat as append-only.
enum RtcpMux {
  RTCP_MUX_DISABLED,
  RTCP_MUX_ENABLED,
  RTCP_MUX_NO_MEDIA,
  RTCP_MUX_MAX
};

// Converter functions from libjingle types to WebKit types.
blink::WebRTCPeerConnectionHandlerClient::ICEGatheringState
GetWebKitIceGatheringState(
    webrtc::PeerConnectionInterface::IceGatheringState state) {
  using blink::WebRTCPeerConnectionHandlerClient;
  switch (state) {
    case webrtc::PeerConnectionInterface::kIceGatheringNew:
      return WebRTCPeerConnectionHandlerClient::kICEGatheringStateNew;
    case webrtc::PeerConnectionInterface::kIceGatheringGathering:
      return WebRTCPeerConnectionHandlerClient::kICEGatheringStateGathering;
    case webrtc::PeerConnectionInterface::kIceGatheringComplete:
      return WebRTCPeerConnectionHandlerClient::kICEGatheringStateComplete;
    default:
      NOTREACHED();
      return WebRTCPeerConnectionHandlerClient::kICEGatheringStateNew;
  }
}

blink::WebRTCPeerConnectionHandlerClient::ICEConnectionState
GetWebKitIceConnectionState(
    webrtc::PeerConnectionInterface::IceConnectionState ice_state) {
  using blink::WebRTCPeerConnectionHandlerClient;
  switch (ice_state) {
    case webrtc::PeerConnectionInterface::kIceConnectionNew:
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateStarting;
    case webrtc::PeerConnectionInterface::kIceConnectionChecking:
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateChecking;
    case webrtc::PeerConnectionInterface::kIceConnectionConnected:
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateConnected;
    case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateCompleted;
    case webrtc::PeerConnectionInterface::kIceConnectionFailed:
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateFailed;
    case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateDisconnected;
    case webrtc::PeerConnectionInterface::kIceConnectionClosed:
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateClosed;
    default:
      NOTREACHED();
      return WebRTCPeerConnectionHandlerClient::kICEConnectionStateClosed;
  }
}

blink::WebRTCPeerConnectionHandlerClient::SignalingState
GetWebKitSignalingState(webrtc::PeerConnectionInterface::SignalingState state) {
  using blink::WebRTCPeerConnectionHandlerClient;
  switch (state) {
    case webrtc::PeerConnectionInterface::kStable:
      return WebRTCPeerConnectionHandlerClient::kSignalingStateStable;
    case webrtc::PeerConnectionInterface::kHaveLocalOffer:
      return WebRTCPeerConnectionHandlerClient::kSignalingStateHaveLocalOffer;
    case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
      return WebRTCPeerConnectionHandlerClient::
          kSignalingStateHaveLocalPrAnswer;
    case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
      return WebRTCPeerConnectionHandlerClient::kSignalingStateHaveRemoteOffer;
    case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
      return WebRTCPeerConnectionHandlerClient::
          kSignalingStateHaveRemotePrAnswer;
    case webrtc::PeerConnectionInterface::kClosed:
      return WebRTCPeerConnectionHandlerClient::kSignalingStateClosed;
    default:
      NOTREACHED();
      return WebRTCPeerConnectionHandlerClient::kSignalingStateClosed;
  }
}

blink::WebRTCSessionDescription CreateWebKitSessionDescription(
    const std::string& sdp, const std::string& type) {
  blink::WebRTCSessionDescription description;
  description.Initialize(blink::WebString::FromUTF8(type),
                         blink::WebString::FromUTF8(sdp));
  return description;
}

blink::WebRTCSessionDescription
CreateWebKitSessionDescription(
    const webrtc::SessionDescriptionInterface* native_desc) {
  if (!native_desc) {
    LOG(ERROR) << "Native session description is null.";
    return blink::WebRTCSessionDescription();
  }

  std::string sdp;
  if (!native_desc->ToString(&sdp)) {
    LOG(ERROR) << "Failed to get SDP string of native session description.";
    return blink::WebRTCSessionDescription();
  }

  return CreateWebKitSessionDescription(sdp, native_desc->type());
}

void RunClosureWithTrace(const base::Closure& closure,
                         const char* trace_event_name) {
  TRACE_EVENT0("webrtc", trace_event_name);
  closure.Run();
}

void RunSynchronousClosure(const base::Closure& closure,
                           const char* trace_event_name,
                           base::WaitableEvent* event) {
  {
    TRACE_EVENT0("webrtc", trace_event_name);
    closure.Run();
  }
  event->Signal();
}

void GetSdpAndTypeFromSessionDescription(
    const base::Callback<const webrtc::SessionDescriptionInterface*()>&
        description_callback,
    std::string* sdp, std::string* type) {
  const webrtc::SessionDescriptionInterface* description =
      description_callback.Run();
  if (description) {
    description->ToString(sdp);
    *type = description->type();
  }
}

// Converter functions from Blink types to WebRTC types.

// This function doesn't assume |webrtc_config| is empty. Any fields in
// |blink_config| replace the corresponding fields in |webrtc_config|, but
// fields that only exist in |webrtc_config| are left alone.
void GetNativeRtcConfiguration(
    const blink::WebRTCConfiguration& blink_config,
    webrtc::PeerConnectionInterface::RTCConfiguration* webrtc_config) {
  DCHECK(webrtc_config);

  webrtc_config->servers.clear();
  for (const blink::WebRTCIceServer& blink_server : blink_config.ice_servers) {
    webrtc::PeerConnectionInterface::IceServer server;
    server.username = blink_server.username.Utf8();
    server.password = blink_server.credential.Utf8();
    server.uri = blink_server.url.GetString().Utf8();
    webrtc_config->servers.push_back(server);
  }

  switch (blink_config.ice_transport_policy) {
    case blink::WebRTCIceTransportPolicy::kRelay:
      webrtc_config->type = webrtc::PeerConnectionInterface::kRelay;
      break;
    case blink::WebRTCIceTransportPolicy::kAll:
      webrtc_config->type = webrtc::PeerConnectionInterface::kAll;
      break;
    default:
      NOTREACHED();
  }

  switch (blink_config.bundle_policy) {
    case blink::WebRTCBundlePolicy::kBalanced:
      webrtc_config->bundle_policy =
          webrtc::PeerConnectionInterface::kBundlePolicyBalanced;
      break;
    case blink::WebRTCBundlePolicy::kMaxBundle:
      webrtc_config->bundle_policy =
          webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
      break;
    case blink::WebRTCBundlePolicy::kMaxCompat:
      webrtc_config->bundle_policy =
          webrtc::PeerConnectionInterface::kBundlePolicyMaxCompat;
      break;
    default:
      NOTREACHED();
  }

  switch (blink_config.rtcp_mux_policy) {
    case blink::WebRTCRtcpMuxPolicy::kNegotiate:
      webrtc_config->rtcp_mux_policy =
          webrtc::PeerConnectionInterface::kRtcpMuxPolicyNegotiate;
      break;
    case blink::WebRTCRtcpMuxPolicy::kRequire:
      webrtc_config->rtcp_mux_policy =
          webrtc::PeerConnectionInterface::kRtcpMuxPolicyRequire;
      break;
    default:
      NOTREACHED();
  }

  switch (blink_config.sdp_semantics) {
    case blink::WebRTCSdpSemantics::kDefault:
    case blink::WebRTCSdpSemantics::kPlanB:
      webrtc_config->sdp_semantics = webrtc::SdpSemantics::kPlanB;
      break;
    case blink::WebRTCSdpSemantics::kUnifiedPlan:
      webrtc_config->sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
      break;
    default:
      NOTREACHED();
  }

  webrtc_config->certificates.clear();
  for (const std::unique_ptr<blink::WebRTCCertificate>& blink_certificate :
       blink_config.certificates) {
    webrtc_config->certificates.push_back(
        static_cast<RTCCertificate*>(blink_certificate.get())
            ->rtcCertificate());
  }

  webrtc_config->ice_candidate_pool_size = blink_config.ice_candidate_pool_size;
}

void CopyConstraintsIntoRtcConfiguration(
    const blink::WebMediaConstraints constraints,
    webrtc::PeerConnectionInterface::RTCConfiguration* configuration) {
  // Copy info from constraints into configuration, if present.
  if (constraints.IsEmpty()) {
    return;
  }

  bool the_value;
  if (GetConstraintValueAsBoolean(
          constraints, &blink::WebMediaTrackConstraintSet::enable_i_pv6,
          &the_value)) {
    configuration->disable_ipv6 = !the_value;
  } else {
    // Note: IPv6 WebRTC value is "disable" while Blink is "enable".
    configuration->disable_ipv6 = false;
  }

  if (GetConstraintValueAsBoolean(
          constraints, &blink::WebMediaTrackConstraintSet::enable_dscp,
          &the_value)) {
    configuration->set_dscp(the_value);
  }

  if (GetConstraintValueAsBoolean(
          constraints,
          &blink::WebMediaTrackConstraintSet::goog_cpu_overuse_detection,
          &the_value)) {
    configuration->set_cpu_adaptation(the_value);
  }

  if (GetConstraintValueAsBoolean(
          constraints,
          &blink::WebMediaTrackConstraintSet::
              goog_enable_video_suspend_below_min_bitrate,
          &the_value)) {
    configuration->set_suspend_below_min_bitrate(the_value);
  }

  if (!GetConstraintValueAsBoolean(
          constraints,
          &blink::WebMediaTrackConstraintSet::enable_rtp_data_channels,
          &configuration->enable_rtp_data_channel)) {
    configuration->enable_rtp_data_channel = false;
  }
  int rate;
  if (GetConstraintValueAsInteger(
          constraints,
          &blink::WebMediaTrackConstraintSet::goog_screencast_min_bitrate,
          &rate)) {
    configuration->screencast_min_bitrate = rtc::Optional<int>(rate);
  }
  configuration->combined_audio_video_bwe = ConstraintToOptional(
      constraints,
      &blink::WebMediaTrackConstraintSet::goog_combined_audio_video_bwe);
  configuration->enable_dtls_srtp = ConstraintToOptional(
      constraints, &blink::WebMediaTrackConstraintSet::enable_dtls_srtp);
}

class SessionDescriptionRequestTracker {
 public:
  SessionDescriptionRequestTracker(
      const base::WeakPtr<RTCPeerConnectionHandler>& handler,
      const base::WeakPtr<PeerConnectionTracker>& tracker,
      PeerConnectionTracker::Action action)
      : handler_(handler), tracker_(tracker), action_(action) {}

  void TrackOnSuccess(const webrtc::SessionDescriptionInterface* desc) {
    DCHECK(thread_checker_.CalledOnValidThread());
    if (tracker_ && handler_) {
      std::string value;
      if (desc) {
        desc->ToString(&value);
        value = "type: " + desc->type() + ", sdp: " + value;
      }
      tracker_->TrackSessionDescriptionCallback(
          handler_.get(), action_, "OnSuccess", value);
    }
  }

  void TrackOnFailure(const webrtc::RTCError& error) {
    DCHECK(thread_checker_.CalledOnValidThread());
    if (handler_ && tracker_) {
      tracker_->TrackSessionDescriptionCallback(handler_.get(), action_,
                                                "OnFailure", error.message());
    }
  }

 private:
  const base::WeakPtr<RTCPeerConnectionHandler> handler_;
  const base::WeakPtr<PeerConnectionTracker> tracker_;
  PeerConnectionTracker::Action action_;
  base::ThreadChecker thread_checker_;
};

// Class mapping responses from calls to libjingle CreateOffer/Answer and
// the blink::WebRTCSessionDescriptionRequest.
class CreateSessionDescriptionRequest
    : public webrtc::CreateSessionDescriptionObserver {
 public:
  explicit CreateSessionDescriptionRequest(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
      const blink::WebRTCSessionDescriptionRequest& request,
      const base::WeakPtr<RTCPeerConnectionHandler>& handler,
      const base::WeakPtr<PeerConnectionTracker>& tracker,
      PeerConnectionTracker::Action action)
      : main_thread_(main_thread),
        webkit_request_(request),
        tracker_(handler, tracker, action) {
  }

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE, base::BindOnce(&CreateSessionDescriptionRequest::OnSuccess,
                                    this, desc));
      return;
    }

    tracker_.TrackOnSuccess(desc);
    webkit_request_.RequestSucceeded(CreateWebKitSessionDescription(desc));
    webkit_request_.Reset();
    delete desc;
  }
  void OnFailure(webrtc::RTCError error) override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE, base::BindOnce(&CreateSessionDescriptionRequest::OnFailure,
                                    this, std::move(error)));
      return;
    }

    tracker_.TrackOnFailure(error);
    // TODO(hta): Convert CreateSessionDescriptionRequest.OnFailure
    webkit_request_.RequestFailed(error);
    webkit_request_.Reset();
  }

 protected:
  ~CreateSessionDescriptionRequest() override {
    // This object is reference counted and its callback methods |OnSuccess| and
    // |OnFailure| will be invoked on libjingle's signaling thread and posted to
    // the main thread. Since the main thread may complete before the signaling
    // thread has deferenced this object there is no guarantee that this object
    // is destructed on the main thread.
    DLOG_IF(ERROR, !webkit_request_.IsNull())
        << "CreateSessionDescriptionRequest not completed. Shutting down?";
  }

  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  blink::WebRTCSessionDescriptionRequest webkit_request_;
  SessionDescriptionRequestTracker tracker_;
};

// Class mapping responses from calls to libjingle SetLocalDescription and a
// blink::WebRTCVoidRequest.
class SetLocalDescriptionRequest
    : public webrtc::SetSessionDescriptionObserver {
 public:
  explicit SetLocalDescriptionRequest(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
      const blink::WebRTCVoidRequest& request,
      const base::WeakPtr<RTCPeerConnectionHandler>& handler,
      const base::WeakPtr<PeerConnectionTracker>& tracker,
      PeerConnectionTracker::Action action)
      : main_thread_(main_thread),
        webkit_request_(request),
        tracker_(handler, tracker, action) {}

  void OnSuccess() override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(&SetLocalDescriptionRequest::OnSuccess, this));
      return;
    }
    tracker_.TrackOnSuccess(nullptr);
    webkit_request_.RequestSucceeded();
    webkit_request_.Reset();
  }
  void OnFailure(webrtc::RTCError error) override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE, base::BindOnce(&SetLocalDescriptionRequest::OnFailure,
                                    this, std::move(error)));
      return;
    }
    tracker_.TrackOnFailure(error);
    webkit_request_.RequestFailed(error);
    webkit_request_.Reset();
  }

 protected:
  ~SetLocalDescriptionRequest() override {
    // This object is reference counted and its callback methods |OnSuccess| and
    // |OnFailure| will be invoked on libjingle's signaling thread and posted to
    // the main thread. Since the main thread may complete before the signaling
    // thread has deferenced this object there is no guarantee that this object
    // is destructed on the main thread.
    DLOG_IF(ERROR, !webkit_request_.IsNull())
        << "SetLocalDescriptionRequest not completed. Shutting down?";
  }

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  blink::WebRTCVoidRequest webkit_request_;
  SessionDescriptionRequestTracker tracker_;
};

blink::WebRTCLegacyStatsMemberType
WebRTCLegacyStatsMemberTypeFromStatsValueType(
    webrtc::StatsReport::Value::Type type) {
  switch (type) {
    case StatsReport::Value::kInt:
      return blink::kWebRTCLegacyStatsMemberTypeInt;
    case StatsReport::Value::kInt64:
      return blink::kWebRTCLegacyStatsMemberTypeInt64;
    case StatsReport::Value::kFloat:
      return blink::kWebRTCLegacyStatsMemberTypeFloat;
    case StatsReport::Value::kString:
    case StatsReport::Value::kStaticString:
      return blink::kWebRTCLegacyStatsMemberTypeString;
    case StatsReport::Value::kBool:
      return blink::kWebRTCLegacyStatsMemberTypeBool;
    case StatsReport::Value::kId:
      return blink::kWebRTCLegacyStatsMemberTypeId;
  }
  NOTREACHED();
  return blink::kWebRTCLegacyStatsMemberTypeInt;
}

// Class mapping responses from calls to libjingle
// GetStats into a blink::WebRTCStatsCallback.
class StatsResponse : public webrtc::StatsObserver {
 public:
  StatsResponse(const scoped_refptr<LocalRTCStatsRequest>& request,
                scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : request_(request.get()), main_thread_(task_runner) {
    // Measure the overall time it takes to satisfy a getStats request.
    TRACE_EVENT_ASYNC_BEGIN0("webrtc", "getStats_Native", this);
    signaling_thread_checker_.DetachFromThread();
  }

  void OnComplete(const StatsReports& reports) override {
    DCHECK(signaling_thread_checker_.CalledOnValidThread());
    TRACE_EVENT0("webrtc", "StatsResponse::OnComplete");
    // We can't use webkit objects directly since they use a single threaded
    // heap allocator.
    std::vector<Report*>* report_copies = new std::vector<Report*>();
    report_copies->reserve(reports.size());
    for (auto* r : reports)
      report_copies->push_back(new Report(r));

    main_thread_->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&StatsResponse::DeliverCallback, this,
                       base::Unretained(report_copies)),
        base::BindOnce(&StatsResponse::DeleteReports,
                       base::Unretained(report_copies)));
  }

 private:
  class Report : public blink::WebRTCLegacyStats {
   public:
    class MemberIterator : public blink::WebRTCLegacyStatsMemberIterator {
     public:
      MemberIterator(const StatsReport::Values::const_iterator& it,
                     const StatsReport::Values::const_iterator& end)
          : it_(it), end_(end) {}

      // blink::WebRTCLegacyStatsMemberIterator
      bool IsEnd() const override { return it_ == end_; }
      void Next() override { ++it_; }
      blink::WebString GetName() const override {
        return blink::WebString::FromUTF8(it_->second->display_name());
      }
      blink::WebRTCLegacyStatsMemberType GetType() const override {
        return WebRTCLegacyStatsMemberTypeFromStatsValueType(
            it_->second->type());
      }
      int ValueInt() const override { return it_->second->int_val(); }
      int64_t ValueInt64() const override { return it_->second->int64_val(); }
      float ValueFloat() const override { return it_->second->float_val(); }
      blink::WebString ValueString() const override {
        const StatsReport::ValuePtr& value = it_->second;
        if (value->type() == StatsReport::Value::kString)
          return blink::WebString::FromUTF8(value->string_val());
        DCHECK_EQ(value->type(), StatsReport::Value::kStaticString);
        return blink::WebString::FromUTF8(value->static_string_val());
      }
      bool ValueBool() const override { return it_->second->bool_val(); }
      blink::WebString ValueToString() const override {
        const StatsReport::ValuePtr& value = it_->second;
        if (value->type() == StatsReport::Value::kString)
          return blink::WebString::FromUTF8(value->string_val());
        if (value->type() == StatsReport::Value::kStaticString)
          return blink::WebString::FromUTF8(value->static_string_val());
        return blink::WebString::FromUTF8(value->ToString());
      }

     private:
      StatsReport::Values::const_iterator it_;
      StatsReport::Values::const_iterator end_;
    };

    Report(const StatsReport* report)
        : thread_checker_(),
          id_(report->id()->ToString()),
          type_(report->type()),
          type_name_(report->TypeToString()),
          timestamp_(report->timestamp()),
          values_(report->values()) {}

    ~Report() override {
      // Since the values vector holds pointers to const objects that are bound
      // to the signaling thread, they must be released on the same thread.
      DCHECK(thread_checker_.CalledOnValidThread());
    }

    // blink::WebRTCLegacyStats
    blink::WebString Id() const override {
      return blink::WebString::FromUTF8(id_);
    }
    blink::WebString GetType() const override {
      return blink::WebString::FromUTF8(type_name_);
    }
    double Timestamp() const override { return timestamp_; }
    blink::WebRTCLegacyStatsMemberIterator* Iterator() const override {
      return new MemberIterator(values_.cbegin(), values_.cend());
    }

    bool HasValues() const {
      return values_.size() > 0;
    }

   private:
    const base::ThreadChecker thread_checker_;
    const std::string id_;
    const StatsReport::StatsType type_;
    const std::string type_name_;
    const double timestamp_;
    const StatsReport::Values values_;
  };

  static void DeleteReports(std::vector<Report*>* reports) {
    TRACE_EVENT0("webrtc", "StatsResponse::DeleteReports");
    for (auto* p : *reports)
      delete p;
    delete reports;
  }

  void DeliverCallback(const std::vector<Report*>* reports) {
    DCHECK(main_thread_->BelongsToCurrentThread());
    TRACE_EVENT0("webrtc", "StatsResponse::DeliverCallback");

    rtc::scoped_refptr<LocalRTCStatsResponse> response(
        request_->createResponse().get());
    for (const auto* report : *reports) {
      if (report->HasValues())
        AddReport(response.get(), *report);
    }

    // Record the getStats operation as done before calling into Blink so that
    // we don't skew the perf measurements of the native code with whatever the
    // callback might be doing.
    TRACE_EVENT_ASYNC_END0("webrtc", "getStats_Native", this);
    request_->requestSucceeded(response);
    request_ = nullptr;  // must be freed on the main thread.
  }

  void AddReport(LocalRTCStatsResponse* response, const Report& report) {
    response->addStats(report);
  }

  rtc::scoped_refptr<LocalRTCStatsRequest> request_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  base::ThreadChecker signaling_thread_checker_;
};

void GetStatsOnSignalingThread(
    const scoped_refptr<webrtc::PeerConnectionInterface>& pc,
    webrtc::PeerConnectionInterface::StatsOutputLevel level,
    const scoped_refptr<webrtc::StatsObserver>& observer,
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> selector) {
  TRACE_EVENT0("webrtc", "GetStatsOnSignalingThread");

  if (selector) {
    bool belongs_to_pc = false;
    for (const auto& sender : pc->GetSenders()) {
      if (sender->track() == selector) {
        belongs_to_pc = true;
        break;
      }
    }
    if (!belongs_to_pc) {
      for (const auto& receiver : pc->GetReceivers()) {
        if (receiver->track() == selector) {
          belongs_to_pc = true;
          break;
        }
      }
    }
    if (!belongs_to_pc) {
      DVLOG(1) << "GetStats: Track not found.";
      observer->OnComplete(StatsReports());
      return;
    }
  }

  if (!pc->GetStats(observer.get(), selector.get(), level)) {
    DVLOG(1) << "GetStats failed.";
    observer->OnComplete(StatsReports());
  }
}

void GetRTCStatsOnSignalingThread(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_thread,
    scoped_refptr<webrtc::PeerConnectionInterface> native_peer_connection,
    std::unique_ptr<blink::WebRTCStatsReportCallback> callback) {
  TRACE_EVENT0("webrtc", "GetRTCStatsOnSignalingThread");

  native_peer_connection->GetStats(
      RTCStatsCollectorCallbackImpl::Create(main_thread, std::move(callback)));
}

class PeerConnectionUMAObserver : public webrtc::UMAObserver {
 public:
  PeerConnectionUMAObserver() {}
  ~PeerConnectionUMAObserver() override {}
  void IncrementEnumCounter(webrtc::PeerConnectionEnumCounterType counter_type,
                            int counter,
                            int counter_max) override {
    switch (counter_type) {
      case webrtc::kEnumCounterAddressFamily:
        UMA_HISTOGRAM_EXACT_LINEAR("WebRTC.PeerConnection.IPMetrics", counter,
                                   counter_max);
        break;
      case webrtc::kEnumCounterIceCandidatePairTypeUdp:
        UMA_HISTOGRAM_EXACT_LINEAR(
            "WebRTC.PeerConnection.CandidatePairType_UDP", counter,
            counter_max);
        break;
      case webrtc::kEnumCounterIceCandidatePairTypeTcp:
        UMA_HISTOGRAM_EXACT_LINEAR(
            "WebRTC.PeerConnection.CandidatePairType_TCP", counter,
            counter_max);
        break;
      case webrtc::kEnumCounterDtlsHandshakeError:
        UMA_HISTOGRAM_EXACT_LINEAR("WebRTC.PeerConnection.DtlsHandshakeError",
                                   counter, counter_max);
        break;
      case webrtc::kEnumCounterIceRestart:
        UMA_HISTOGRAM_EXACT_LINEAR("WebRTC.PeerConnection.IceRestartState",
                                   counter, counter_max);
        break;
      case webrtc::kEnumCounterIceRegathering:
        UMA_HISTOGRAM_EXACT_LINEAR("WebRTC.PeerConnection.IceRegatheringReason",
                                   counter, counter_max);
        break;
      case webrtc::kEnumCounterKeyProtocol:
        UMA_HISTOGRAM_ENUMERATION(
            "WebRTC.PeerConnection.KeyProtocol",
            static_cast<webrtc::KeyExchangeProtocolType>(counter),
            webrtc::kEnumCounterKeyProtocolMax);
        break;
      case webrtc::kEnumCounterSdpSemanticNegotiated:
        UMA_HISTOGRAM_ENUMERATION(
            "WebRTC.PeerConnection.SdpSemanticNegotiated",
            static_cast<webrtc::SdpSemanticNegotiated>(counter),
            webrtc::kSdpSemanticNegotiatedMax);
        break;
      case webrtc::kEnumCounterKeyProtocolMediaType:
        UMA_HISTOGRAM_ENUMERATION(
            "WebRTC.PeerConnection.KeyProtocolByMedia",
            static_cast<webrtc::KeyExchangeProtocolMedia>(counter),
            webrtc::kEnumCounterKeyProtocolMediaTypeMax);
        break;
      case webrtc::kEnumCounterSdpFormatReceived:
        UMA_HISTOGRAM_ENUMERATION(
            "WebRTC.PeerConnection.SdpFormatReceived",
            static_cast<webrtc::SdpFormatReceived>(counter),
            webrtc::kSdpFormatReceivedMax);
        break;
      default:
        // The default clause is expected to be reached when new enum types are
        // added.
        break;
    }
  }

  void IncrementSparseEnumCounter(
      webrtc::PeerConnectionEnumCounterType counter_type,
      int counter) override {
    switch (counter_type) {
      case webrtc::kEnumCounterAudioSrtpCipher:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SrtpCryptoSuite.Audio",
                                 counter);
        break;
      case webrtc::kEnumCounterAudioSslCipher:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SslCipherSuite.Audio",
                                 counter);
        break;
      case webrtc::kEnumCounterVideoSrtpCipher:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SrtpCryptoSuite.Video",
                                 counter);
        break;
      case webrtc::kEnumCounterVideoSslCipher:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SslCipherSuite.Video",
                                 counter);
        break;
      case webrtc::kEnumCounterDataSrtpCipher:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SrtpCryptoSuite.Data",
                                 counter);
        break;
      case webrtc::kEnumCounterDataSslCipher:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SslCipherSuite.Data",
                                 counter);
        break;
      case webrtc::kEnumCounterSrtpUnprotectError:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SrtpUnprotectError",
                                 counter);
        break;
      case webrtc::kEnumCounterSrtcpUnprotectError:
        base::UmaHistogramSparse("WebRTC.PeerConnection.SrtcpUnprotectError",
                                 counter);
        break;
      default:
        // The default clause is expected to reach when new enum types are
        // added.
        break;
    }
  }

  void AddHistogramSample(webrtc::PeerConnectionUMAMetricsName type,
                          int value) override {
    // Runs on libjingle's signaling thread.
    switch (type) {
      case webrtc::kTimeToConnect:
        UMA_HISTOGRAM_MEDIUM_TIMES(
            "WebRTC.PeerConnection.TimeToConnect",
            base::TimeDelta::FromMilliseconds(value));
        break;
      case webrtc::kNetworkInterfaces_IPv4:
        UMA_HISTOGRAM_COUNTS_100("WebRTC.PeerConnection.IPv4Interfaces",
                                 value);
        break;
      case webrtc::kNetworkInterfaces_IPv6:
        UMA_HISTOGRAM_COUNTS_100("WebRTC.PeerConnection.IPv6Interfaces",
                                 value);
        break;
      default:
        // The default clause is expected to reach when new enum types are
        // added.
        break;
    }
  }
};

void ConvertOfferOptionsToWebrtcOfferOptions(
    const blink::WebRTCOfferOptions& options,
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions* output) {
  output->offer_to_receive_audio = options.OfferToReceiveAudio();
  output->offer_to_receive_video = options.OfferToReceiveVideo();
  output->voice_activity_detection = options.VoiceActivityDetection();
  output->ice_restart = options.IceRestart();
}

void ConvertAnswerOptionsToWebrtcAnswerOptions(
    const blink::WebRTCAnswerOptions& options,
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions* output) {
  output->voice_activity_detection = options.VoiceActivityDetection();
}

void ConvertConstraintsToWebrtcOfferOptions(
    const blink::WebMediaConstraints& constraints,
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions* output) {
  if (constraints.IsEmpty()) {
    return;
  }
  std::string failing_name;
  if (constraints.Basic().HasMandatoryOutsideSet(
          {constraints.Basic().offer_to_receive_audio.GetName(),
           constraints.Basic().offer_to_receive_video.GetName(),
           constraints.Basic().voice_activity_detection.GetName(),
           constraints.Basic().ice_restart.GetName()},
          failing_name)) {
    // TODO(hta): Reject the calling operation with "constraint error"
    // https://crbug.com/594894
    DLOG(ERROR) << "Invalid mandatory constraint to CreateOffer/Answer: "
                << failing_name;
  }
  GetConstraintValueAsInteger(
      constraints, &blink::WebMediaTrackConstraintSet::offer_to_receive_audio,
      &output->offer_to_receive_audio);
  GetConstraintValueAsInteger(
      constraints, &blink::WebMediaTrackConstraintSet::offer_to_receive_video,
      &output->offer_to_receive_video);
  GetConstraintValueAsBoolean(
      constraints, &blink::WebMediaTrackConstraintSet::voice_activity_detection,
      &output->voice_activity_detection);
  GetConstraintValueAsBoolean(constraints,
                              &blink::WebMediaTrackConstraintSet::ice_restart,
                              &output->ice_restart);
}

std::set<RTCPeerConnectionHandler*>* GetPeerConnectionHandlers() {
  static std::set<RTCPeerConnectionHandler*>* handlers =
      new std::set<RTCPeerConnectionHandler*>();
  return handlers;
}

// Counts the number of senders that have |web_stream| as an associated stream.
size_t GetLocalStreamUsageCount(
    const std::vector<std::unique_ptr<RTCRtpSender>>& rtp_senders,
    const blink::WebMediaStream& web_stream) {
  size_t usage_count = 0;
  for (const auto& sender : rtp_senders) {
    for (const auto& stream_ref : sender->stream_refs()) {
      if (stream_ref->adapter().web_stream().UniqueId() ==
          web_stream.UniqueId()) {
        ++usage_count;
        break;
      }
    }
  }
  return usage_count;
}

// Counts the number of receivers that have |webrtc_stream| as an associated
// stream.
size_t GetRemoteStreamUsageCount(
    const std::map<uintptr_t, std::unique_ptr<RTCRtpReceiver>>& rtp_receivers,
    const webrtc::MediaStreamInterface* webrtc_stream) {
  size_t usage_count = 0;
  for (const auto& receiver_entry : rtp_receivers) {
    if (receiver_entry.second->HasStream(webrtc_stream))
      ++usage_count;
  }
  return usage_count;
}

enum SdpSemanticRequested {
  kSdpSemanticRequestedDefault,
  kSdpSemanticRequestedPlanB,
  kSdpSemanticRequestedUnifiedPlan,
  kSdpSemanticRequestedMax
};

SdpSemanticRequested GetSdpSemanticRequested(
    blink::WebRTCSdpSemantics sdp_semantics) {
  switch (sdp_semantics) {
    case blink::WebRTCSdpSemantics::kDefault:
      return kSdpSemanticRequestedDefault;
    case blink::WebRTCSdpSemantics::kPlanB:
      return kSdpSemanticRequestedPlanB;
    case blink::WebRTCSdpSemantics::kUnifiedPlan:
      return kSdpSemanticRequestedUnifiedPlan;
  }
  NOTREACHED();
  return kSdpSemanticRequestedDefault;
}

}  // namespace

// Implementation of LocalRTCStatsRequest.
LocalRTCStatsRequest::LocalRTCStatsRequest(blink::WebRTCStatsRequest impl)
    : impl_(impl) {
}

LocalRTCStatsRequest::LocalRTCStatsRequest() {}
LocalRTCStatsRequest::~LocalRTCStatsRequest() {}

bool LocalRTCStatsRequest::hasSelector() const {
  return impl_.HasSelector();
}

blink::WebMediaStreamTrack LocalRTCStatsRequest::component() const {
  return impl_.Component();
}

scoped_refptr<LocalRTCStatsResponse> LocalRTCStatsRequest::createResponse() {
  return scoped_refptr<LocalRTCStatsResponse>(
      new rtc::RefCountedObject<LocalRTCStatsResponse>(impl_.CreateResponse()));
}

void LocalRTCStatsRequest::requestSucceeded(
    const LocalRTCStatsResponse* response) {
  impl_.RequestSucceeded(response->webKitStatsResponse());
}

// Implementation of LocalRTCStatsResponse.
blink::WebRTCStatsResponse LocalRTCStatsResponse::webKitStatsResponse() const {
  return impl_;
}

void LocalRTCStatsResponse::addStats(const blink::WebRTCLegacyStats& stats) {
  impl_.AddStats(stats);
}

// Processes the resulting state changes of a SetRemoteDescription call.
class RTCPeerConnectionHandler::WebRtcSetRemoteDescriptionObserverImpl
    : public WebRtcSetRemoteDescriptionObserver {
 public:
  WebRtcSetRemoteDescriptionObserverImpl(
      base::WeakPtr<RTCPeerConnectionHandler> handler,
      blink::WebRTCVoidRequest web_request,
      base::WeakPtr<PeerConnectionTracker> tracker,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : handler_(handler),
        main_thread_(task_runner),
        web_request_(web_request),
        tracker_(tracker) {}

  void OnSetRemoteDescriptionComplete(
      webrtc::RTCErrorOr<WebRtcSetRemoteDescriptionObserver::States>
          states_or_error) override {
    if (!states_or_error.ok()) {
      auto& error = states_or_error.error();
      if (tracker_ && handler_) {
        tracker_->TrackSessionDescriptionCallback(
            handler_.get(),
            PeerConnectionTracker::ACTION_SET_REMOTE_DESCRIPTION, "OnFailure",
            error.message());
      }
      web_request_.RequestFailed(error);
      web_request_.Reset();
      return;
    }

    auto& states = states_or_error.value();
    if (handler_) {
      // Determine which receivers have been removed before processing the
      // removal as to not invalidate the iterator.
      std::vector<RTCRtpReceiver*> removed_receivers;
      for (auto it = handler_->rtp_receivers_.begin();
           it != handler_->rtp_receivers_.end(); ++it) {
        if (ReceiverWasRemoved(*it->second, states.receiver_states))
          removed_receivers.push_back(it->second.get());
      }

      // Update stream states (which tracks belong to which streams).
      for (auto& stream_state : GetStreamStates(states, removed_receivers)) {
        stream_state.stream_ref->adapter().SetTracks(
            std::move(stream_state.track_refs));
      }

      // Process the addition of remote receivers/tracks.
      for (auto& receiver_state : states.receiver_states) {
        if (ReceiverWasAdded(receiver_state)) {
          handler_->OnAddRemoteTrack(receiver_state.receiver,
                                     std::move(receiver_state.track_ref),
                                     std::move(receiver_state.stream_refs));
        }
      }
      // Process the removal of remote receivers/tracks.
      for (auto* removed_receiver : removed_receivers)
        handler_->OnRemoveRemoteTrack(removed_receiver->webrtc_receiver());
      if (tracker_) {
        tracker_->TrackSessionDescriptionCallback(
            handler_.get(),
            PeerConnectionTracker::ACTION_SET_REMOTE_DESCRIPTION, "OnSuccess",
            "");
      }
    }
    // Resolve the promise in a post to ensure any events scheduled for
    // dispatching will have fired by the time the promise is resolved.
    // TODO(hbos): Don't schedule/post to fire events/resolve the promise.
    // Instead, do it all synchronously. This must happen as the last step
    // before returning so that all effects of SRD have occurred when the event
    // executes. https://crbug.com/788558
    main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &RTCPeerConnectionHandler::WebRtcSetRemoteDescriptionObserverImpl::
                ResolvePromise,
            this));
  }

 private:
  ~WebRtcSetRemoteDescriptionObserverImpl() override {}

  // Describes which tracks belong to a stream in terms of AdapterRefs.
  struct StreamState {
    StreamState(
        std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> stream_ref)
        : stream_ref(std::move(stream_ref)) {}

    std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef> stream_ref;
    std::vector<std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>>
        track_refs;
  };

  void ResolvePromise() {
    web_request_.RequestSucceeded();
    web_request_.Reset();
  }

  bool ReceiverWasAdded(const WebRtcReceiverState& receiver_state) {
    return handler_->rtp_receivers_.find(
               RTCRtpReceiver::getId(receiver_state.receiver.get())) ==
           handler_->rtp_receivers_.end();
  }

  bool ReceiverWasRemoved(
      const RTCRtpReceiver& receiver,
      const std::vector<WebRtcReceiverState>& receiver_states) {
    for (const auto& receiver_state : receiver_states) {
      if (receiver_state.receiver == receiver.webrtc_receiver())
        return false;
    }
    return true;
  }

  // Determines the stream states from the current state of receivers and the
  // receivers that are about to be removed. Produces a stable order of streams.
  std::vector<StreamState> GetStreamStates(
      const WebRtcSetRemoteDescriptionObserver::States& states,
      const std::vector<RTCRtpReceiver*>& removed_receivers) {
    states.CheckInvariants();
    std::vector<StreamState> stream_states;
    // The receiver's track belongs to all of its streams. A stream may be
    // associated with multiple tracks (multiple receivers).
    for (auto& receiver_state : states.receiver_states) {
      for (auto& stream_ref : receiver_state.stream_refs) {
        CHECK(stream_ref);
        CHECK(stream_ref->adapter().is_initialized());
        CHECK(!stream_ref->adapter().web_stream().IsNull());
        CHECK(stream_ref->adapter().webrtc_stream());
        auto* stream_state =
            GetOrAddStreamStateForStream(*stream_ref, &stream_states);
        auto track_ref = receiver_state.track_ref->Copy();
        CHECK(!track_ref->web_track().IsNull());
        CHECK(track_ref->webrtc_track());
        stream_state->track_refs.push_back(std::move(track_ref));
      }
    }
    // The track of removed receivers do not belong to any stream. Make sure we
    // have a stream state for any streams belonging to receivers about to be
    // removed in case it was the last receiver referencing that stream.
    for (auto* removed_receiver : removed_receivers) {
      for (auto& stream_ref : removed_receiver->StreamAdapterRefs()) {
        CHECK(!stream_ref->adapter().web_stream().IsNull());
        CHECK(stream_ref->adapter().webrtc_stream());
        GetOrAddStreamStateForStream(*stream_ref, &stream_states);
      }
    }
    states.CheckInvariants();
    return stream_states;
  }

  StreamState* GetOrAddStreamStateForStream(
      const WebRtcMediaStreamAdapterMap::AdapterRef& stream_ref,
      std::vector<StreamState>* stream_states) {
    auto* webrtc_stream = stream_ref.adapter().webrtc_stream().get();
    for (auto& stream_state : *stream_states) {
      if (stream_state.stream_ref->adapter().webrtc_stream().get() ==
          webrtc_stream) {
        return &stream_state;
      }
    }
    stream_states->push_back(StreamState(stream_ref.Copy()));
    return &stream_states->back();
  }

  base::WeakPtr<RTCPeerConnectionHandler> handler_;
  scoped_refptr<base::SequencedTaskRunner> main_thread_;
  blink::WebRTCVoidRequest web_request_;
  base::WeakPtr<PeerConnectionTracker> tracker_;
};

// Receives notifications from a PeerConnection object about state changes. The
// callbacks we receive here come on the webrtc signaling thread, so this class
// takes care of delivering them to an RTCPeerConnectionHandler instance on the
// main thread. In order to do safe PostTask-ing, the class is reference counted
// and checks for the existence of the RTCPeerConnectionHandler instance before
// delivering callbacks on the main thread.
class RTCPeerConnectionHandler::Observer
    : public base::RefCountedThreadSafe<RTCPeerConnectionHandler::Observer>,
      public PeerConnectionObserver,
      public RtcEventLogOutputSink {
 public:
  Observer(const base::WeakPtr<RTCPeerConnectionHandler>& handler,
           scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : handler_(handler), main_thread_(task_runner) {}

  // When an RTC event log is sent back from PeerConnection, it arrives here.
  void OnWebRtcEventLogWrite(const std::string& output) override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(
              &RTCPeerConnectionHandler::Observer::OnWebRtcEventLogWrite, this,
              output));
    } else if (handler_) {
      handler_->OnWebRtcEventLogWrite(output);
    }
  }

 protected:
  friend class base::RefCountedThreadSafe<RTCPeerConnectionHandler::Observer>;
  ~Observer() override = default;

  void OnSignalingChange(
      PeerConnectionInterface::SignalingState new_state) override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(&RTCPeerConnectionHandler::Observer::OnSignalingChange,
                         this, new_state));
    } else if (handler_) {
      handler_->OnSignalingChange(new_state);
    }
  }

  // TODO(hbos): Remove once no longer mandatory to implement.
  void OnAddStream(rtc::scoped_refptr<MediaStreamInterface>) override {}
  void OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface>) override {}

  void OnDataChannel(
      rtc::scoped_refptr<DataChannelInterface> data_channel) override {
    std::unique_ptr<RtcDataChannelHandler> handler(
        new RtcDataChannelHandler(main_thread_, data_channel));
    main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(&RTCPeerConnectionHandler::Observer::OnDataChannelImpl,
                       this, std::move(handler)));
  }

  void OnRenegotiationNeeded() override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(
              &RTCPeerConnectionHandler::Observer::OnRenegotiationNeeded,
              this));
    } else if (handler_) {
      handler_->OnRenegotiationNeeded();
    }
  }

  void OnIceConnectionChange(
      PeerConnectionInterface::IceConnectionState new_state) override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(
              &RTCPeerConnectionHandler::Observer::OnIceConnectionChange, this,
              new_state));
    } else if (handler_) {
      handler_->OnIceConnectionChange(new_state);
    }
  }

  void OnIceGatheringChange(
      PeerConnectionInterface::IceGatheringState new_state) override {
    if (!main_thread_->BelongsToCurrentThread()) {
      main_thread_->PostTask(
          FROM_HERE,
          base::BindOnce(
              &RTCPeerConnectionHandler::Observer::OnIceGatheringChange, this,
              new_state));
    } else if (handler_) {
      handler_->OnIceGatheringChange(new_state);
    }
  }

  void OnIceCandidate(const IceCandidateInterface* candidate) override {
    std::string sdp;
    if (!candidate->ToString(&sdp)) {
      NOTREACHED() << "OnIceCandidate: Could not get SDP string.";
      return;
    }

    main_thread_->PostTask(
        FROM_HERE,
        base::BindOnce(&RTCPeerConnectionHandler::Observer::OnIceCandidateImpl,
                       this, sdp, candidate->sdp_mid(),
                       candidate->sdp_mline_index(),
                       candidate->candidate().component(),
                       candidate->candidate().address().family()));
  }

  void OnDataChannelImpl(std::unique_ptr<RtcDataChannelHandler> handler) {
    DCHECK(main_thread_->BelongsToCurrentThread());
    if (handler_)
      handler_->OnDataChannel(std::move(handler));
  }

  void OnIceCandidateImpl(const std::string& sdp, const std::string& sdp_mid,
      int sdp_mline_index, int component, int address_family) {
    DCHECK(main_thread_->BelongsToCurrentThread());
    if (handler_) {
      handler_->OnIceCandidate(sdp, sdp_mid, sdp_mline_index, component,
          address_family);
    }
  }

 private:
  const base::WeakPtr<RTCPeerConnectionHandler> handler_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
};

RTCPeerConnectionHandler::RTCPeerConnectionHandler(
    blink::WebRTCPeerConnectionHandlerClient* client,
    PeerConnectionDependencyFactory* dependency_factory,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : id_(base::ToUpperASCII(base::UnguessableToken::Create().ToString())),
      initialize_called_(false),
      client_(client),
      is_closed_(false),
      dependency_factory_(dependency_factory),
      track_adapter_map_(
          new WebRtcMediaStreamTrackAdapterMap(dependency_factory_,
                                               task_runner)),
      stream_adapter_map_(new WebRtcMediaStreamAdapterMap(dependency_factory_,
                                                          task_runner,
                                                          track_adapter_map_)),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {
  CHECK(client_);

  GetPeerConnectionHandlers()->insert(this);
}

RTCPeerConnectionHandler::~RTCPeerConnectionHandler() {
  DCHECK(thread_checker_.CalledOnValidThread());

  Stop();

  GetPeerConnectionHandlers()->erase(this);
  if (peer_connection_tracker_)
    peer_connection_tracker_->UnregisterPeerConnection(this);

  UMA_HISTOGRAM_COUNTS_10000(
      "WebRTC.NumDataChannelsPerPeerConnection", num_data_channels_created_);
}

void RTCPeerConnectionHandler::associateWithFrame(blink::WebLocalFrame* frame) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(frame);
  frame_ = frame;
}

bool RTCPeerConnectionHandler::Initialize(
    const blink::WebRTCConfiguration& server_configuration,
    const blink::WebMediaConstraints& options) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(frame_);

  CHECK(!initialize_called_);
  initialize_called_ = true;

  peer_connection_tracker_ =
      RenderThreadImpl::current()->peer_connection_tracker()->AsWeakPtr();

  GetNativeRtcConfiguration(server_configuration, &configuration_);

  // Choose between RTC smoothness algorithm and prerenderer smoothing.
  // Prerenderer smoothing is turned on if RTC smoothness is turned off.
  configuration_.set_prerenderer_smoothing(
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableRTCSmoothnessAlgorithm));

  configuration_.set_experiment_cpu_load_estimator(
      base::FeatureList::IsEnabled(media::kNewEncodeCpuLoadEstimator));

  // Copy all the relevant constraints into |config|.
  CopyConstraintsIntoRtcConfiguration(options, &configuration_);

  peer_connection_observer_ =
      new Observer(weak_factory_.GetWeakPtr(), task_runner_);
  native_peer_connection_ = dependency_factory_->CreatePeerConnection(
      configuration_, frame_, peer_connection_observer_.get());

  if (!native_peer_connection_.get()) {
    LOG(ERROR) << "Failed to initialize native PeerConnection.";
    return false;
  }

  if (peer_connection_tracker_) {
    peer_connection_tracker_->RegisterPeerConnection(this, configuration_,
                                                     options, frame_);
  }

  uma_observer_ = new rtc::RefCountedObject<PeerConnectionUMAObserver>();
  native_peer_connection_->RegisterUMAObserver(uma_observer_.get());

  UMA_HISTOGRAM_ENUMERATION(
      "WebRTC.PeerConnection.SdpSemanticRequested",
      GetSdpSemanticRequested(server_configuration.sdp_semantics),
      kSdpSemanticRequestedMax);

  return true;
}

bool RTCPeerConnectionHandler::InitializeForTest(
    const blink::WebRTCConfiguration& server_configuration,
    const blink::WebMediaConstraints& options,
    const base::WeakPtr<PeerConnectionTracker>& peer_connection_tracker) {
  DCHECK(thread_checker_.CalledOnValidThread());

  CHECK(!initialize_called_);
  initialize_called_ = true;

  GetNativeRtcConfiguration(server_configuration, &configuration_);

  peer_connection_observer_ =
      new Observer(weak_factory_.GetWeakPtr(), task_runner_);
  CopyConstraintsIntoRtcConfiguration(options, &configuration_);

  native_peer_connection_ = dependency_factory_->CreatePeerConnection(
      configuration_, nullptr, peer_connection_observer_.get());
  if (!native_peer_connection_.get()) {
    LOG(ERROR) << "Failed to initialize native PeerConnection.";
    return false;
  }
  peer_connection_tracker_ = peer_connection_tracker;
  return true;
}

void RTCPeerConnectionHandler::CreateOffer(
    const blink::WebRTCSessionDescriptionRequest& request,
    const blink::WebMediaConstraints& options) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::createOffer");

  scoped_refptr<CreateSessionDescriptionRequest> description_request(
      new rtc::RefCountedObject<CreateSessionDescriptionRequest>(
          task_runner_, request, weak_factory_.GetWeakPtr(),
          peer_connection_tracker_,
          PeerConnectionTracker::ACTION_CREATE_OFFER));

  // TODO(tommi): Do this asynchronously via e.g. PostTaskAndReply.
  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtc_options;
  ConvertConstraintsToWebrtcOfferOptions(options, &webrtc_options);
  native_peer_connection_->CreateOffer(description_request.get(),
                                       webrtc_options);

  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackCreateOffer(this, options);
}

void RTCPeerConnectionHandler::CreateOffer(
    const blink::WebRTCSessionDescriptionRequest& request,
    const blink::WebRTCOfferOptions& options) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::createOffer");

  scoped_refptr<CreateSessionDescriptionRequest> description_request(
      new rtc::RefCountedObject<CreateSessionDescriptionRequest>(
          task_runner_, request, weak_factory_.GetWeakPtr(),
          peer_connection_tracker_,
          PeerConnectionTracker::ACTION_CREATE_OFFER));

  // TODO(tommi): Do this asynchronously via e.g. PostTaskAndReply.
  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtc_options;
  ConvertOfferOptionsToWebrtcOfferOptions(options, &webrtc_options);
  native_peer_connection_->CreateOffer(description_request.get(),
                                       webrtc_options);

  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackCreateOffer(this, options);
}

void RTCPeerConnectionHandler::CreateAnswer(
    const blink::WebRTCSessionDescriptionRequest& request,
    const blink::WebMediaConstraints& options) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::createAnswer");
  scoped_refptr<CreateSessionDescriptionRequest> description_request(
      new rtc::RefCountedObject<CreateSessionDescriptionRequest>(
          task_runner_, request, weak_factory_.GetWeakPtr(),
          peer_connection_tracker_,
          PeerConnectionTracker::ACTION_CREATE_ANSWER));
  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtc_options;
  ConvertConstraintsToWebrtcOfferOptions(options, &webrtc_options);
  // TODO(tommi): Do this asynchronously via e.g. PostTaskAndReply.
  native_peer_connection_->CreateAnswer(description_request.get(),
                                        webrtc_options);

  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackCreateAnswer(this, options);
}

void RTCPeerConnectionHandler::CreateAnswer(
    const blink::WebRTCSessionDescriptionRequest& request,
    const blink::WebRTCAnswerOptions& options) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::createAnswer");
  scoped_refptr<CreateSessionDescriptionRequest> description_request(
      new rtc::RefCountedObject<CreateSessionDescriptionRequest>(
          task_runner_, request, weak_factory_.GetWeakPtr(),
          peer_connection_tracker_,
          PeerConnectionTracker::ACTION_CREATE_ANSWER));
  // TODO(tommi): Do this asynchronously via e.g. PostTaskAndReply.
  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions webrtc_options;
  ConvertAnswerOptionsToWebrtcAnswerOptions(options, &webrtc_options);
  native_peer_connection_->CreateAnswer(description_request.get(),
                                        webrtc_options);

  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackCreateAnswer(this, options);
}

bool IsOfferOrAnswer(const webrtc::SessionDescriptionInterface* native_desc) {
  DCHECK(native_desc);
  return native_desc->type() == "offer" || native_desc->type() == "answer";
}

void RTCPeerConnectionHandler::SetLocalDescription(
    const blink::WebRTCVoidRequest& request,
    const blink::WebRTCSessionDescription& description) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::setLocalDescription");

  std::string sdp = description.Sdp().Utf8();
  std::string type = description.GetType().Utf8();

  if (peer_connection_tracker_) {
    peer_connection_tracker_->TrackSetSessionDescription(
        this, sdp, type, PeerConnectionTracker::SOURCE_LOCAL);
  }

  webrtc::SdpParseError error;
  // Since CreateNativeSessionDescription uses the dependency factory, we need
  // to make this call on the current thread to be safe.
  webrtc::SessionDescriptionInterface* native_desc =
      CreateNativeSessionDescription(sdp, type, &error);
  if (!native_desc) {
    std::string reason_str = "Failed to parse SessionDescription. ";
    reason_str.append(error.line);
    reason_str.append(" ");
    reason_str.append(error.description);
    LOG(ERROR) << reason_str;
    request.RequestFailed(webrtc::RTCError(webrtc::RTCErrorType::INTERNAL_ERROR,
                                           std::move(reason_str)));
    if (peer_connection_tracker_) {
      peer_connection_tracker_->TrackSessionDescriptionCallback(
          this, PeerConnectionTracker::ACTION_SET_LOCAL_DESCRIPTION,
          "OnFailure", reason_str);
    }
    return;
  }

  if (!first_local_description_ && IsOfferOrAnswer(native_desc)) {
    first_local_description_.reset(new FirstSessionDescription(native_desc));
    if (first_remote_description_) {
      ReportFirstSessionDescriptions(
          *first_local_description_,
          *first_remote_description_);
    }
  }

  scoped_refptr<SetLocalDescriptionRequest> set_request(
      new rtc::RefCountedObject<SetLocalDescriptionRequest>(
          task_runner_, request, weak_factory_.GetWeakPtr(),
          peer_connection_tracker_,
          PeerConnectionTracker::ACTION_SET_LOCAL_DESCRIPTION));

  signaling_thread()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &RunClosureWithTrace,
          base::Bind(&webrtc::PeerConnectionInterface::SetLocalDescription,
                     native_peer_connection_, base::RetainedRef(set_request),
                     base::Unretained(native_desc)),
          "SetLocalDescription"));
}

void RTCPeerConnectionHandler::SetRemoteDescription(
    const blink::WebRTCVoidRequest& request,
    const blink::WebRTCSessionDescription& description) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::setRemoteDescription");
  std::string sdp = description.Sdp().Utf8();
  std::string type = description.GetType().Utf8();

  if (peer_connection_tracker_) {
    peer_connection_tracker_->TrackSetSessionDescription(
        this, sdp, type, PeerConnectionTracker::SOURCE_REMOTE);
  }

  webrtc::SdpParseError error;
  // Since CreateNativeSessionDescription uses the dependency factory, we need
  // to make this call on the current thread to be safe.
  std::unique_ptr<webrtc::SessionDescriptionInterface> native_desc(
      CreateNativeSessionDescription(sdp, type, &error));
  if (!native_desc) {
    std::string reason_str = "Failed to parse SessionDescription. ";
    reason_str.append(error.line);
    reason_str.append(" ");
    reason_str.append(error.description);
    LOG(ERROR) << reason_str;
    request.RequestFailed(webrtc::RTCError(
        webrtc::RTCErrorType::UNSUPPORTED_OPERATION, std::move(reason_str)));
    if (peer_connection_tracker_) {
      peer_connection_tracker_->TrackSessionDescriptionCallback(
          this, PeerConnectionTracker::ACTION_SET_REMOTE_DESCRIPTION,
          "OnFailure", reason_str);
    }
    return;
  }

  if (!first_remote_description_ && IsOfferOrAnswer(native_desc.get())) {
    first_remote_description_.reset(
        new FirstSessionDescription(native_desc.get()));
    if (first_local_description_) {
      ReportFirstSessionDescriptions(
          *first_local_description_,
          *first_remote_description_);
    }
  }

  scoped_refptr<WebRtcSetRemoteDescriptionObserverImpl> content_observer(
      new WebRtcSetRemoteDescriptionObserverImpl(
          weak_factory_.GetWeakPtr(), request, peer_connection_tracker_,
          task_runner_));

  rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface>
      webrtc_observer(WebRtcSetRemoteDescriptionObserverHandler::Create(
                          task_runner_, native_peer_connection_,
                          stream_adapter_map_, content_observer)
                          .get());

  signaling_thread()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &RunClosureWithTrace,
          base::Bind(
              static_cast<void (webrtc::PeerConnectionInterface::*)(
                  std::unique_ptr<webrtc::SessionDescriptionInterface>,
                  rtc::scoped_refptr<
                      webrtc::SetRemoteDescriptionObserverInterface>)>(
                  &webrtc::PeerConnectionInterface::SetRemoteDescription),
              native_peer_connection_, base::Passed(&native_desc),
              webrtc_observer),
          "SetRemoteDescription"));
}

blink::WebRTCSessionDescription RTCPeerConnectionHandler::LocalDescription() {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::localDescription");

  // Since local_description returns a pointer to a non-reference-counted object
  // that lives on the signaling thread, we cannot fetch a pointer to it and use
  // it directly here. Instead, we access the object completely on the signaling
  // thread.
  std::string sdp, type;
  base::Callback<const webrtc::SessionDescriptionInterface*()> description_cb =
      base::Bind(&webrtc::PeerConnectionInterface::local_description,
                 native_peer_connection_);
  RunSynchronousClosureOnSignalingThread(
      base::Bind(&GetSdpAndTypeFromSessionDescription,
                 std::move(description_cb), base::Unretained(&sdp),
                 base::Unretained(&type)),
      "localDescription");

  return CreateWebKitSessionDescription(sdp, type);
}

blink::WebRTCSessionDescription RTCPeerConnectionHandler::RemoteDescription() {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::remoteDescription");
  // Since local_description returns a pointer to a non-reference-counted object
  // that lives on the signaling thread, we cannot fetch a pointer to it and use
  // it directly here. Instead, we access the object completely on the signaling
  // thread.
  std::string sdp, type;
  base::Callback<const webrtc::SessionDescriptionInterface*()> description_cb =
      base::Bind(&webrtc::PeerConnectionInterface::remote_description,
                 native_peer_connection_);
  RunSynchronousClosureOnSignalingThread(
      base::Bind(&GetSdpAndTypeFromSessionDescription,
                 std::move(description_cb), base::Unretained(&sdp),
                 base::Unretained(&type)),
      "remoteDescription");

  return CreateWebKitSessionDescription(sdp, type);
}

webrtc::RTCErrorType RTCPeerConnectionHandler::SetConfiguration(
    const blink::WebRTCConfiguration& blink_config) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::setConfiguration");
  GetNativeRtcConfiguration(blink_config, &configuration_);

  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackSetConfiguration(this, configuration_);

  webrtc::RTCError webrtc_error;
  bool ret =
      native_peer_connection_->SetConfiguration(configuration_, &webrtc_error);
  // The boolean return value is made redundant by the error output param; just
  // DCHECK that they're consistent.
  DCHECK_EQ(ret, webrtc_error.type() == webrtc::RTCErrorType::NONE);
  return webrtc_error.type();
}

bool RTCPeerConnectionHandler::AddICECandidate(
    const blink::WebRTCVoidRequest& request,
    scoped_refptr<blink::WebRTCICECandidate> candidate) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::addICECandidate");
  // Libjingle currently does not accept callbacks for addICECandidate.
  // For that reason we are going to call callbacks from here.

  // TODO(tommi): Instead of calling addICECandidate here, we can do a
  // PostTaskAndReply kind of a thing.
  bool result = AddICECandidate(std::move(candidate));
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&RTCPeerConnectionHandler::OnaddICECandidateResult,
                     weak_factory_.GetWeakPtr(), request, result));
  // On failure callback will be triggered.
  return true;
}

bool RTCPeerConnectionHandler::AddICECandidate(
    scoped_refptr<blink::WebRTCICECandidate> candidate) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::addICECandidate");
  std::unique_ptr<webrtc::IceCandidateInterface> native_candidate(
      dependency_factory_->CreateIceCandidate(candidate->SdpMid().Utf8(),
                                              candidate->SdpMLineIndex(),
                                              candidate->Candidate().Utf8()));
  bool return_value = false;

  if (native_candidate) {
    return_value =
        native_peer_connection_->AddIceCandidate(native_candidate.get());
    LOG_IF(ERROR, !return_value) << "Error processing ICE candidate.";
  } else {
    LOG(ERROR) << "Could not create native ICE candidate.";
  }

  if (peer_connection_tracker_) {
    peer_connection_tracker_->TrackAddIceCandidate(
        this, std::move(candidate), PeerConnectionTracker::SOURCE_REMOTE,
        return_value);
  }
  return return_value;
}

void RTCPeerConnectionHandler::OnaddICECandidateResult(
    const blink::WebRTCVoidRequest& webkit_request, bool result) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnaddICECandidateResult");
  if (!result) {
    // We don't have the actual error code from the libjingle, so for now
    // using a generic error string.
    return webkit_request.RequestFailed(
        webrtc::RTCError(webrtc::RTCErrorType::UNSUPPORTED_OPERATION,
                         std::move("Error processing ICE candidate")));
  }

  return webkit_request.RequestSucceeded();
}

void RTCPeerConnectionHandler::GetStats(
    const blink::WebRTCStatsRequest& request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  scoped_refptr<LocalRTCStatsRequest> inner_request(
      new rtc::RefCountedObject<LocalRTCStatsRequest>(request));
  getStats(inner_request);
}

void RTCPeerConnectionHandler::getStats(
    const scoped_refptr<LocalRTCStatsRequest>& request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::getStats");

  rtc::scoped_refptr<webrtc::StatsObserver> observer(
      new rtc::RefCountedObject<StatsResponse>(request, task_runner_));

  rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> selector;
  if (request->hasSelector()) {
    auto track_adapter_ref =
        track_adapter_map_->GetLocalTrackAdapter(request->component());
    if (!track_adapter_ref) {
      track_adapter_ref =
          track_adapter_map_->GetRemoteTrackAdapter(request->component());
    }
    if (track_adapter_ref)
      selector = track_adapter_ref->webrtc_track();
  }

  GetStats(observer, webrtc::PeerConnectionInterface::kStatsOutputLevelStandard,
           std::move(selector));
}

// TODO(tommi,hbos): It's weird to have three {g|G}etStats methods for the
// legacy stats collector API and even more for the new stats API. Clean it up.
// TODO(hbos): Rename old |getStats| and related functions to "getLegacyStats",
// rename new |getStats|'s helper functions from "GetRTCStats*" to "GetStats*".
void RTCPeerConnectionHandler::GetStats(
    webrtc::StatsObserver* observer,
    webrtc::PeerConnectionInterface::StatsOutputLevel level,
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> selector) {
  DCHECK(thread_checker_.CalledOnValidThread());
  signaling_thread()->PostTask(
      FROM_HERE,
      base::BindOnce(&GetStatsOnSignalingThread, native_peer_connection_, level,
                     base::WrapRefCounted(observer), std::move(selector)));
}

void RTCPeerConnectionHandler::GetStats(
    std::unique_ptr<blink::WebRTCStatsReportCallback> callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  signaling_thread()->PostTask(
      FROM_HERE, base::BindOnce(&GetRTCStatsOnSignalingThread, task_runner_,
                                native_peer_connection_, std::move(callback)));
}

std::unique_ptr<blink::WebRTCRtpSender> RTCPeerConnectionHandler::AddTrack(
    const blink::WebMediaStreamTrack& track,
    const blink::WebVector<blink::WebMediaStream>& streams) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::AddTrack");

  // Get or create the associated track and stream adapters.
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_adapter =
      track_adapter_map_->GetOrCreateLocalTrackAdapter(track);
  std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
      stream_adapters(streams.size());
  std::vector<webrtc::MediaStreamInterface*> webrtc_streams(streams.size());
  for (size_t i = 0; i < streams.size(); ++i) {
    stream_adapters[i] =
        stream_adapter_map_->GetOrCreateLocalStreamAdapter(streams[i]);
    webrtc_streams[i] = stream_adapters[i]->adapter().webrtc_stream().get();
  }

  rtc::scoped_refptr<webrtc::RtpSenderInterface> webrtc_sender =
      native_peer_connection_->AddTrack(track_adapter->webrtc_track(),
                                        webrtc_streams);
  if (!webrtc_sender)
    return nullptr;
  DCHECK(FindSender(RTCRtpSender::getId(webrtc_sender)) == rtp_senders_.end());
  rtp_senders_.push_back(std::make_unique<RTCRtpSender>(
      native_peer_connection_, task_runner_, signaling_thread(),
      stream_adapter_map_, std::move(webrtc_sender), std::move(track_adapter),
      std::move(stream_adapters)));
  for (const auto& stream_ref : rtp_senders_.back()->stream_refs()) {
    if (GetLocalStreamUsageCount(rtp_senders_,
                                 stream_ref->adapter().web_stream()) == 1u) {
      // This is the first occurrence of this stream.
      if (peer_connection_tracker_) {
        peer_connection_tracker_->TrackAddStream(
            this, stream_ref->adapter().web_stream(),
            PeerConnectionTracker::SOURCE_LOCAL);
      }
      PerSessionWebRTCAPIMetrics::GetInstance()->IncrementStreamCounter();
      track_metrics_.AddStream(MediaStreamTrackMetrics::SENT_STREAM,
                               stream_ref->adapter().webrtc_stream().get());
    }
  }
  return rtp_senders_.back()->ShallowCopy();
}

bool RTCPeerConnectionHandler::RemoveTrack(blink::WebRTCRtpSender* web_sender) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::RemoveTrack");
  auto it = FindSender(web_sender->Id());
  if (it == rtp_senders_.end())
    return false;
  if (!(*it)->RemoveFromPeerConnection(native_peer_connection_.get()))
    return false;
  auto stream_refs = (*it)->stream_refs();
  // TODO(hbos): In Unified Plan, senders are never removed. The lower layer
  // needs to tell us what to do with the sender: Update its states and/or
  // remove it. https://crbug.com/799030
  rtp_senders_.erase(it);
  for (const auto& stream_ref : stream_refs) {
    if (GetLocalStreamUsageCount(rtp_senders_,
                                 stream_ref->adapter().web_stream()) == 0u) {
      // This was the last occurrence of this stream.
      if (peer_connection_tracker_) {
        peer_connection_tracker_->TrackRemoveStream(
            this, stream_ref->adapter().web_stream(),
            PeerConnectionTracker::SOURCE_LOCAL);
      }
      PerSessionWebRTCAPIMetrics::GetInstance()->DecrementStreamCounter();
      track_metrics_.RemoveStream(MediaStreamTrackMetrics::SENT_STREAM,
                                  stream_ref->adapter().webrtc_stream().get());
    }
  }
  return true;
}

void RTCPeerConnectionHandler::CloseClientPeerConnection() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!is_closed_)
    client_->ClosePeerConnection();
}

void RTCPeerConnectionHandler::StartEventLog(IPC::PlatformFileForTransit file,
                                             int64_t max_file_size_bytes) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(file != IPC::InvalidPlatformFileForTransit());
  // TODO(eladalon): StartRtcEventLog() return value is not useful; remove it
  // or find a way to be able to use it.
  // https://crbug.com/775415
  native_peer_connection_->StartRtcEventLog(
      IPC::PlatformFileForTransitToPlatformFile(file), max_file_size_bytes);
}

void RTCPeerConnectionHandler::StartEventLog() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // TODO(eladalon): StartRtcEventLog() return value is not useful; remove it
  // or find a way to be able to use it.
  // https://crbug.com/775415
  native_peer_connection_->StartRtcEventLog(
      std::make_unique<RtcEventLogOutputSinkProxy>(
          peer_connection_observer_.get()),
      webrtc::RtcEventLog::kImmediateOutput);
}

void RTCPeerConnectionHandler::StopEventLog() {
  DCHECK(thread_checker_.CalledOnValidThread());
  native_peer_connection_->StopRtcEventLog();
}

void RTCPeerConnectionHandler::OnWebRtcEventLogWrite(
    const std::string& output) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (peer_connection_tracker_) {
    peer_connection_tracker_->TrackRtcEventLogWrite(this, output);
  }
}

blink::WebRTCDataChannelHandler* RTCPeerConnectionHandler::CreateDataChannel(
    const blink::WebString& label,
    const blink::WebRTCDataChannelInit& init) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::createDataChannel");
  DVLOG(1) << "createDataChannel label " << label.Utf8();

  webrtc::DataChannelInit config;
  // TODO(jiayl): remove the deprecated reliable field once Libjingle is updated
  // to handle that.
  config.reliable = false;
  config.id = init.id;
  config.ordered = init.ordered;
  config.negotiated = init.negotiated;
  config.maxRetransmits = init.max_retransmits;
  config.maxRetransmitTime = init.max_retransmit_time;
  config.protocol = init.protocol.Utf8();

  rtc::scoped_refptr<webrtc::DataChannelInterface> webrtc_channel(
      native_peer_connection_->CreateDataChannel(label.Utf8(), &config));
  if (!webrtc_channel) {
    DLOG(ERROR) << "Could not create native data channel.";
    return nullptr;
  }
  if (peer_connection_tracker_) {
    peer_connection_tracker_->TrackCreateDataChannel(
        this, webrtc_channel.get(), PeerConnectionTracker::SOURCE_LOCAL);
  }

  ++num_data_channels_created_;

  return new RtcDataChannelHandler(task_runner_, webrtc_channel);
}

void RTCPeerConnectionHandler::Stop() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "RTCPeerConnectionHandler::stop";

  if (is_closed_ || !native_peer_connection_.get())
    return;  // Already stopped.

  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackStop(this);

  native_peer_connection_->Close();

  // This object may no longer forward call backs to blink.
  is_closed_ = true;
}

blink::WebString RTCPeerConnectionHandler::Id() const {
  return blink::WebString::FromASCII(id_);
}

void RTCPeerConnectionHandler::OnSignalingChange(
    webrtc::PeerConnectionInterface::SignalingState new_state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnSignalingChange");

  blink::WebRTCPeerConnectionHandlerClient::SignalingState state =
      GetWebKitSignalingState(new_state);
  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackSignalingStateChange(this, state);
  if (!is_closed_)
    client_->DidChangeSignalingState(state);
}

// Called any time the IceConnectionState changes
void RTCPeerConnectionHandler::OnIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnIceConnectionChange");
  DCHECK(thread_checker_.CalledOnValidThread());
  ReportICEState(new_state);
  if (new_state == webrtc::PeerConnectionInterface::kIceConnectionChecking) {
    ice_connection_checking_start_ = base::TimeTicks::Now();
  } else if (new_state ==
      webrtc::PeerConnectionInterface::kIceConnectionConnected) {
    // If the state becomes connected, send the time needed for PC to become
    // connected from checking to UMA. UMA data will help to know how much
    // time needed for PC to connect with remote peer.
    if (ice_connection_checking_start_.is_null()) {
      // From UMA, we have observed a large number of calls falling into the
      // overflow buckets. One possibility is that the Checking is not signaled
      // before Connected. This is to guard against that situation to make the
      // metric more robust.
      UMA_HISTOGRAM_MEDIUM_TIMES("WebRTC.PeerConnection.TimeToConnect",
                                 base::TimeDelta());
    } else {
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "WebRTC.PeerConnection.TimeToConnect",
        base::TimeTicks::Now() - ice_connection_checking_start_);
    }
  }

  track_metrics_.IceConnectionChange(new_state);
  blink::WebRTCPeerConnectionHandlerClient::ICEConnectionState state =
      GetWebKitIceConnectionState(new_state);
  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackIceConnectionStateChange(this, state);
  if (!is_closed_)
    client_->DidChangeICEConnectionState(state);
}

// Called any time the IceGatheringState changes
void RTCPeerConnectionHandler::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState new_state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnIceGatheringChange");

  if (new_state == webrtc::PeerConnectionInterface::kIceGatheringComplete) {
    UMA_HISTOGRAM_COUNTS_100("WebRTC.PeerConnection.IPv4LocalCandidates",
                             num_local_candidates_ipv4_);

    UMA_HISTOGRAM_COUNTS_100("WebRTC.PeerConnection.IPv6LocalCandidates",
                             num_local_candidates_ipv6_);
  } else if (new_state ==
             webrtc::PeerConnectionInterface::kIceGatheringGathering) {
    // ICE restarts will change gathering state back to "gathering",
    // reset the counter.
    ResetUMAStats();
  }

  blink::WebRTCPeerConnectionHandlerClient::ICEGatheringState state =
      GetWebKitIceGatheringState(new_state);
  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackIceGatheringStateChange(this, state);
  if (!is_closed_)
    client_->DidChangeICEGatheringState(state);
}

void RTCPeerConnectionHandler::OnRenegotiationNeeded() {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnRenegotiationNeeded");
  if (peer_connection_tracker_)
    peer_connection_tracker_->TrackOnRenegotiationNeeded(this);
  if (!is_closed_)
    client_->NegotiationNeeded();
}

void RTCPeerConnectionHandler::OnAddRemoteTrack(
    scoped_refptr<webrtc::RtpReceiverInterface> webrtc_receiver,
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef>
        remote_track_adapter_ref,
    std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
        remote_stream_adapter_refs) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnAddRemoteTrack");

  for (const auto& remote_stream_adapter_ref : remote_stream_adapter_refs) {
    // New remote stream?
    if (GetRemoteStreamUsageCount(
            rtp_receivers_,
            remote_stream_adapter_ref->adapter().webrtc_stream().get()) == 0) {
      // Update metrics.
      // TODO(hbos): Update metrics to correspond to track added/removed events,
      // not streams. https://crbug.com/765170
      if (peer_connection_tracker_) {
        peer_connection_tracker_->TrackAddStream(
            this, remote_stream_adapter_ref->adapter().web_stream(),
            PeerConnectionTracker::SOURCE_REMOTE);
      }
      PerSessionWebRTCAPIMetrics::GetInstance()->IncrementStreamCounter();
      track_metrics_.AddStream(
          MediaStreamTrackMetrics::RECEIVED_STREAM,
          remote_stream_adapter_ref->adapter().webrtc_stream().get());
    }
  }

  uintptr_t receiver_id = RTCRtpReceiver::getId(webrtc_receiver.get());
  DCHECK(rtp_receivers_.find(receiver_id) == rtp_receivers_.end());
  const std::unique_ptr<RTCRtpReceiver>& rtp_receiver =
      rtp_receivers_
          .insert(std::make_pair(
              receiver_id,
              std::make_unique<RTCRtpReceiver>(
                  native_peer_connection_, task_runner_, signaling_thread(),
                  webrtc_receiver.get(), std::move(remote_track_adapter_ref),
                  std::move(remote_stream_adapter_refs))))
          .first->second;
  if (!is_closed_)
    client_->DidAddRemoteTrack(rtp_receiver->ShallowCopy());
}

void RTCPeerConnectionHandler::OnRemoveRemoteTrack(
    scoped_refptr<webrtc::RtpReceiverInterface> webrtc_receiver) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnRemoveRemoteTrack");

  std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
      remote_stream_adapter_refs;
  {
    uintptr_t receiver_id = RTCRtpReceiver::getId(webrtc_receiver.get());
    auto it = rtp_receivers_.find(receiver_id);
    DCHECK(it != rtp_receivers_.end());
    remote_stream_adapter_refs = it->second->StreamAdapterRefs();
    if (!is_closed_)
      client_->DidRemoveRemoteTrack(it->second->ShallowCopy());
    rtp_receivers_.erase(it);
  }

  for (const auto& remote_stream_adapter_ref : remote_stream_adapter_refs) {
    // Was this the last usage of the remote stream?
    if (GetRemoteStreamUsageCount(
            rtp_receivers_,
            remote_stream_adapter_ref->adapter().webrtc_stream().get()) == 0) {
      // Update metrics.
      track_metrics_.RemoveStream(
          MediaStreamTrackMetrics::RECEIVED_STREAM,
          remote_stream_adapter_ref->adapter().webrtc_stream().get());
      PerSessionWebRTCAPIMetrics::GetInstance()->DecrementStreamCounter();
      if (peer_connection_tracker_) {
        peer_connection_tracker_->TrackRemoveStream(
            this, remote_stream_adapter_ref->adapter().web_stream(),
            PeerConnectionTracker::SOURCE_REMOTE);
      }
    }
  }
}

void RTCPeerConnectionHandler::OnDataChannel(
    std::unique_ptr<RtcDataChannelHandler> handler) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnDataChannelImpl");

  if (peer_connection_tracker_) {
    peer_connection_tracker_->TrackCreateDataChannel(
        this, handler->channel().get(), PeerConnectionTracker::SOURCE_REMOTE);
  }

  if (!is_closed_)
    client_->DidAddRemoteDataChannel(handler.release());
}

void RTCPeerConnectionHandler::OnIceCandidate(
    const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index,
    int component, int address_family) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "RTCPeerConnectionHandler::OnIceCandidateImpl");
  scoped_refptr<blink::WebRTCICECandidate> web_candidate =
      blink::WebRTCICECandidate::Create(blink::WebString::FromUTF8(sdp),
                                        blink::WebString::FromUTF8(sdp_mid),
                                        sdp_mline_index);
  if (peer_connection_tracker_) {
    peer_connection_tracker_->TrackAddIceCandidate(
        this, web_candidate, PeerConnectionTracker::SOURCE_LOCAL, true);
  }

  // Only the first m line's first component is tracked to avoid
  // miscounting when doing BUNDLE or rtcp mux.
  if (sdp_mline_index == 0 && component == 1) {
    if (address_family == AF_INET) {
      ++num_local_candidates_ipv4_;
    } else if (address_family == AF_INET6) {
      ++num_local_candidates_ipv6_;
    } else {
      NOTREACHED();
    }
  }
  if (!is_closed_)
    client_->DidGenerateICECandidate(std::move(web_candidate));
}

webrtc::SessionDescriptionInterface*
RTCPeerConnectionHandler::CreateNativeSessionDescription(
    const std::string& sdp, const std::string& type,
    webrtc::SdpParseError* error) {
  webrtc::SessionDescriptionInterface* native_desc =
      dependency_factory_->CreateSessionDescription(type, sdp, error);

  LOG_IF(ERROR, !native_desc) << "Failed to create native session description."
                              << " Type: " << type << " SDP: " << sdp;

  return native_desc;
}

RTCPeerConnectionHandler::FirstSessionDescription::FirstSessionDescription(
    const webrtc::SessionDescriptionInterface* sdesc) {
  DCHECK(sdesc);

  for (const auto& content : sdesc->description()->contents()) {
    if (content.type == cricket::NS_JINGLE_RTP) {
      const auto* mdesc =
          static_cast<cricket::MediaContentDescription*>(content.description);
      audio = audio || (mdesc->type() == cricket::MEDIA_TYPE_AUDIO);
      video = video || (mdesc->type() == cricket::MEDIA_TYPE_VIDEO);
      rtcp_mux = rtcp_mux || mdesc->rtcp_mux();
    }
  }
}

void RTCPeerConnectionHandler::ReportFirstSessionDescriptions(
    const FirstSessionDescription& local,
    const FirstSessionDescription& remote) {
  RtcpMux rtcp_mux = RTCP_MUX_ENABLED;
  if ((!local.audio && !local.video) || (!remote.audio && !remote.video)) {
    rtcp_mux = RTCP_MUX_NO_MEDIA;
  } else if (!local.rtcp_mux || !remote.rtcp_mux) {
    rtcp_mux = RTCP_MUX_DISABLED;
  }

  UMA_HISTOGRAM_ENUMERATION(
      "WebRTC.PeerConnection.RtcpMux", rtcp_mux, RTCP_MUX_MAX);

  // TODO(pthatcher): Reports stats about whether we have audio and
  // video or not.
}

std::vector<std::unique_ptr<RTCRtpSender>>::iterator
RTCPeerConnectionHandler::FindSender(uintptr_t id) {
  for (auto it = rtp_senders_.begin(); it != rtp_senders_.end(); ++it) {
    if ((*it)->Id() == id)
      return it;
  }
  return rtp_senders_.end();
}

scoped_refptr<base::SingleThreadTaskRunner>
RTCPeerConnectionHandler::signaling_thread() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return dependency_factory_->GetWebRtcSignalingThread();
}

void RTCPeerConnectionHandler::RunSynchronousClosureOnSignalingThread(
    const base::Closure& closure,
    const char* trace_event_name) {
  DCHECK(thread_checker_.CalledOnValidThread());
  scoped_refptr<base::SingleThreadTaskRunner> thread(signaling_thread());
  if (!thread.get() || thread->BelongsToCurrentThread()) {
    TRACE_EVENT0("webrtc", trace_event_name);
    closure.Run();
  } else {
    base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
    thread->PostTask(FROM_HERE,
                     base::BindOnce(&RunSynchronousClosure, closure,
                                    base::Unretained(trace_event_name),
                                    base::Unretained(&event)));
    event.Wait();
  }
}

void RTCPeerConnectionHandler::ReportICEState(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (ice_state_seen_[new_state])
    return;
  ice_state_seen_[new_state] = true;
  UMA_HISTOGRAM_ENUMERATION("WebRTC.PeerConnection.ConnectionState", new_state,
                            webrtc::PeerConnectionInterface::kIceConnectionMax);
}

void RTCPeerConnectionHandler::ResetUMAStats() {
  DCHECK(thread_checker_.CalledOnValidThread());
  num_local_candidates_ipv6_ = 0;
  num_local_candidates_ipv4_ = 0;
  ice_connection_checking_start_ = base::TimeTicks();
  memset(ice_state_seen_, 0, sizeof(ice_state_seen_));
}
}  // namespace content
