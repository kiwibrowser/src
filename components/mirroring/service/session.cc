// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/session.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/mirroring/service/udp_socket_client.h"
#include "components/mirroring/service/video_capture_client.h"
#include "crypto/random.h"
#include "media/cast/net/cast_transport.h"
#include "media/cast/sender/audio_sender.h"
#include "media/cast/sender/video_sender.h"
#include "media/video/video_encode_accelerator.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "net/base/ip_endpoint.h"

using media::cast::FrameSenderConfig;
using media::cast::RtpPayloadType;
using media::cast::CastTransportStatus;
using media::cast::Codec;
using media::cast::FrameEvent;
using media::cast::PacketEvent;
using media::cast::OperationalStatus;
using media::cast::Packet;

namespace mirroring {

namespace {

// The interval for CastTransport to send Frame/PacketEvents to Session for
// logging.
constexpr base::TimeDelta kSendEventsInterval = base::TimeDelta::FromSeconds(1);

// The duration for OFFER/ANSWER exchange. If timeout, notify the client that
// the session failed to start.
constexpr base::TimeDelta kOfferAnswerExchangeTimeout =
    base::TimeDelta::FromSeconds(15);

// Used for OFFER/ANSWER message exchange. Some receivers will error out on
// payloadType values other than the ones hard-coded here.
constexpr int kAudioPayloadType = 127;
constexpr int kVideoPayloadType = 96;

constexpr int kAudioSsrcMin = 1;
constexpr int kAudioSsrcMax = 5e5;
constexpr int kVideoSsrcMin = 5e5 + 1;
constexpr int kVideoSsrcMax = 10e5;

// Maximum number of bytes of file data allowed in a single Crash report. As of
// this writing, the total report upload size is capped at 20 MB.
//
// 2 KB of "overhead bytes" are subtracted to account for all of the non-file
// data in a report upload, including HTTP headers/requests and form data.
constexpr int kMaxCrashReportBytes = (20 * 1024 - 2) * 1024;

class TransportClient final : public media::cast::CastTransport::Client {
 public:
  explicit TransportClient(Session* session) : session_(session) {}
  ~TransportClient() override {}

  // media::cast::CastTransport::Client implementation.

  void OnStatusChanged(CastTransportStatus status) override {
    session_->OnTransportStatusChanged(status);
  }

  void OnLoggingEventsReceived(
      std::unique_ptr<std::vector<FrameEvent>> frame_events,
      std::unique_ptr<std::vector<PacketEvent>> packet_events) override {
    session_->OnLoggingEventsReceived(std::move(frame_events),
                                      std::move(packet_events));
  }

  void ProcessRtpPacket(std::unique_ptr<Packet> packet) override {
    NOTREACHED();
  }

 private:
  Session* const session_;  // Outlives this class.

  DISALLOW_COPY_AND_ASSIGN(TransportClient);
};

// Generates a string with cryptographically secure random bytes.
std::string MakeRandomString(size_t length) {
  std::string result(length, ' ');
  crypto::RandBytes(base::data(result), length);
  return result;
}

int NumberOfEncodeThreads() {
  // Do not saturate CPU utilization just for encoding. On a lower-end system
  // with only 1 or 2 cores, use only one thread for encoding. On systems with
  // more cores, allow half of the cores to be used for encoding.
  return std::min(8, (base::SysInfo::NumberOfProcessors() + 1) / 2);
}

// Scan profiles for hardware VP8 encoder support.
bool IsHardwareVP8EncodingSupported(
    const std::vector<media::VideoEncodeAccelerator::SupportedProfile>&
        profiles) {
  for (const auto& vea_profile : profiles) {
    if (vea_profile.profile >= media::VP8PROFILE_MIN &&
        vea_profile.profile <= media::VP8PROFILE_MAX) {
      return true;
    }
  }
  return false;
}

// Scan profiles for hardware H.264 encoder support.
bool IsHardwareH264EncodingSupported(
    const std::vector<media::VideoEncodeAccelerator::SupportedProfile>&
        profiles) {
// TODO(miu): Look into why H.264 hardware encoder on MacOS is broken.
// http://crbug.com/596674
// TODO(emircan): Look into HW encoder initialization issues on Win.
// https://crbug.com/636064
#if !defined(OS_MACOSX) && !defined(OS_WIN)
  for (const auto& vea_profile : profiles) {
    if (vea_profile.profile >= media::H264PROFILE_MIN &&
        vea_profile.profile <= media::H264PROFILE_MAX) {
      return true;
    }
  }
#endif  // !defined(OS_MACOSX) && !defined(OS_WIN)
  return false;
}

// Helper to add |config| to |config_list| with given |aes_key|.
void AddSenderConfig(int32_t sender_ssrc,
                     FrameSenderConfig config,
                     const std::string& aes_key,
                     const std::string& aes_iv,
                     std::vector<FrameSenderConfig>* config_list) {
  config.aes_key = aes_key;
  config.aes_iv_mask = aes_iv;
  config.sender_ssrc = sender_ssrc;
  config_list->emplace_back(config);
}

// Generate the stream object from |config| and add it to |stream_list|.
void AddStreamObject(int stream_index,
                     const std::string& codec_name,
                     const FrameSenderConfig& config,
                     const MirrorSettings& mirror_settings,
                     base::Value::ListStorage* stream_list) {
  base::Value stream(base::Value::Type::DICTIONARY);
  stream.SetKey("index", base::Value(stream_index));
  stream.SetKey("codecName", base::Value(base::ToLowerASCII(codec_name)));
  stream.SetKey("rtpProfile", base::Value("cast"));
  const bool is_audio =
      (config.rtp_payload_type <= media::cast::RtpPayloadType::AUDIO_LAST);
  stream.SetKey("rtpPayloadType",
                base::Value(is_audio ? kAudioPayloadType : kVideoPayloadType));
  stream.SetKey("ssrc", base::Value(int(config.sender_ssrc)));
  stream.SetKey(
      "targetDelay",
      base::Value(int(config.animated_playout_delay.InMilliseconds())));
  stream.SetKey("aesKey", base::Value(base::HexEncode(config.aes_key.data(),
                                                      config.aes_key.size())));
  stream.SetKey("aesIvMask",
                base::Value(base::HexEncode(config.aes_iv_mask.data(),
                                            config.aes_iv_mask.size())));
  stream.SetKey("timeBase",
                base::Value("1/" + std::to_string(config.rtp_timebase)));
  stream.SetKey("receiverRtcpEventLog", base::Value(true));
  stream.SetKey("rtpExtensions", base::Value("adaptive_playout_delay"));
  if (is_audio) {
    // Note on "AUTO" bitrate calculation: This is based on libopus source
    // at the time of this writing. Internally, it uses the following math:
    //
    //   packet_overhead_bps = 60 bits * num_packets_in_one_second
    //   approx_encoded_signal_bps = frequency * channels
    //   estimated_bps = packet_overhead_bps + approx_encoded_signal_bps
    //
    // For 100 packets/sec at 48 kHz and 2 channels, this is 102kbps.
    const int bitrate = config.max_bitrate > 0
                            ? config.max_bitrate
                            : (60 * config.max_frame_rate +
                               config.rtp_timebase * config.channels);
    stream.SetKey("type", base::Value("audio_source"));
    stream.SetKey("bitRate", base::Value(bitrate));
    stream.SetKey("sampleRate", base::Value(config.rtp_timebase));
    stream.SetKey("channels", base::Value(config.channels));
  } else /* is video */ {
    stream.SetKey("type", base::Value("video_source"));
    stream.SetKey("renderMode", base::Value("video"));
    stream.SetKey("maxFrameRate",
                  base::Value(std::to_string(static_cast<int>(
                                  config.max_frame_rate * 1000)) +
                              "/1000"));
    stream.SetKey("maxBitRate", base::Value(config.max_bitrate));
    base::Value::ListStorage resolutions;
    base::Value resolution(base::Value::Type::DICTIONARY);
    resolution.SetKey("width", base::Value(mirror_settings.max_width()));
    resolution.SetKey("height", base::Value(mirror_settings.max_height()));
    resolutions.emplace_back(std::move(resolution));
    stream.SetKey("resolutions", base::Value(resolutions));
  }
  stream_list->emplace_back(std::move(stream));
}

}  // namespace

Session::Session(int32_t session_id,
                 const CastSinkInfo& sink_info,
                 const gfx::Size& max_resolution,
                 SessionObserver* observer,
                 ResourceProvider* resource_provider,
                 CastMessageChannel* outbound_channel)
    : session_id_(session_id),
      sink_info_(sink_info),
      observer_(observer),
      resource_provider_(resource_provider),
      message_dispatcher_(outbound_channel,
                          base::BindRepeating(&Session::OnResponseParsingError,
                                              base::Unretained(this))),
      weak_factory_(this) {
  DCHECK(resource_provider_);
  mirror_settings_.SetResolutionContraints(max_resolution.width(),
                                           max_resolution.height());
  resource_provider_->GetNetworkContext(mojo::MakeRequest(&network_context_));

  auto wifi_status_monitor =
      std::make_unique<WifiStatusMonitor>(session_id_, &message_dispatcher_);
  network::mojom::URLLoaderFactoryPtr url_loader_factory;
  network_context_->CreateURLLoaderFactory(
      mojo::MakeRequest(&url_loader_factory),
      network::mojom::URLLoaderFactoryParams::New(
          network::mojom::kBrowserProcessId, false, std::string()));

  // Generate session level tags.
  base::Value session_tags(base::Value::Type::DICTIONARY);
  session_tags.SetKey("mirrorSettings", mirror_settings_.ToDictionaryValue());
  session_tags.SetKey("shouldCaptureAudio",
                      base::Value(sink_info_.capability != VIDEO_ONLY));
  session_tags.SetKey("shouldCaptureVideo",
                      base::Value(sink_info_.capability != AUDIO_ONLY));
  session_tags.SetKey("receiverProductName",
                      base::Value(sink_info_.model_name));

  session_monitor_.emplace(
      kMaxCrashReportBytes, sink_info_.ip_address, std::move(session_tags),
      std::move(url_loader_factory), std::move(wifi_status_monitor));

  CreateAndSendOffer();
}

Session::~Session() {
  StopSession();
}

void Session::ReportError(SessionError error) {
  if (session_monitor_.has_value())
    session_monitor_->OnStreamingError(error);
  if (observer_)
    observer_->OnError(error);
  StopSession();
}

void Session::StopSession() {
  DVLOG(1) << __func__;
  if (!resource_provider_)
    return;

  session_monitor_->StopStreamingSession();
  session_monitor_.reset();
  weak_factory_.InvalidateWeakPtrs();
  audio_encode_thread_ = nullptr;
  video_encode_thread_ = nullptr;
  video_capture_client_.reset();
  audio_stream_.reset();
  video_stream_.reset();
  cast_transport_.reset();
  cast_environment_ = nullptr;
  resource_provider_ = nullptr;
  if (observer_) {
    observer_->DidStop();
    observer_ = nullptr;
  }
}

void Session::OnError(const std::string& message) {
  ReportError(SessionError::RTP_STREAM_ERROR);
}

void Session::RequestRefreshFrame() {
  DVLOG(3) << __func__;
  if (video_capture_client_)
    video_capture_client_->RequestRefreshFrame();
}

void Session::OnEncoderStatusChange(OperationalStatus status) {
  switch (status) {
    case OperationalStatus::STATUS_UNINITIALIZED:
    case OperationalStatus::STATUS_CODEC_REINIT_PENDING:
    // Not an error.
    // TODO(miu): As an optimization, signal the client to pause sending more
    // frames until the state becomes STATUS_INITIALIZED again.
    case OperationalStatus::STATUS_INITIALIZED:
      break;
    case OperationalStatus::STATUS_INVALID_CONFIGURATION:
    case OperationalStatus::STATUS_UNSUPPORTED_CODEC:
    case OperationalStatus::STATUS_CODEC_INIT_FAILED:
    case OperationalStatus::STATUS_CODEC_RUNTIME_ERROR:
      ReportError(SessionError::ENCODING_ERROR);
      break;
  }
}

media::VideoEncodeAccelerator::SupportedProfiles
Session::GetSupportedVeaProfiles() {
  // TODO(xjz): Establish GPU channel and query for the supported profiles.
  return media::VideoEncodeAccelerator::SupportedProfiles();
}

void Session::CreateVideoEncodeAccelerator(
    const media::cast::ReceiveVideoEncodeAcceleratorCallback& callback) {
  DVLOG(1) << __func__;
  // TODO(xjz): Establish GPU channel and create the
  // media::MojoVideoEncodeAccelerator with the gpu info.
  if (!callback.is_null())
    callback.Run(video_encode_thread_, nullptr);
}

void Session::CreateVideoEncodeMemory(
    size_t size,
    const media::cast::ReceiveVideoEncodeMemoryCallback& callback) {
  DVLOG(1) << __func__;

  mojo::ScopedSharedBufferHandle mojo_buf =
      mojo::SharedBufferHandle::Create(size);
  if (!mojo_buf->is_valid()) {
    LOG(WARNING) << "Browser failed to allocate shared memory.";
    callback.Run(nullptr);
    return;
  }

  base::SharedMemoryHandle shared_buf;
  if (mojo::UnwrapSharedMemoryHandle(std::move(mojo_buf), &shared_buf, nullptr,
                                     nullptr) != MOJO_RESULT_OK) {
    LOG(WARNING) << "Browser failed to allocate shared memory.";
    callback.Run(nullptr);
    return;
  }

  callback.Run(std::make_unique<base::SharedMemory>(shared_buf, false));
}

void Session::OnTransportStatusChanged(CastTransportStatus status) {
  DVLOG(1) << __func__ << ": status=" << status;
  switch (status) {
    case CastTransportStatus::TRANSPORT_STREAM_UNINITIALIZED:
    case CastTransportStatus::TRANSPORT_STREAM_INITIALIZED:
      return;  // Not errors, do nothing.
    case CastTransportStatus::TRANSPORT_INVALID_CRYPTO_CONFIG:
    case CastTransportStatus::TRANSPORT_SOCKET_ERROR:
      ReportError(SessionError::CAST_TRANSPORT_ERROR);
      break;
  }
}

void Session::OnLoggingEventsReceived(
    std::unique_ptr<std::vector<FrameEvent>> frame_events,
    std::unique_ptr<std::vector<PacketEvent>> packet_events) {
  DCHECK(cast_environment_);
  cast_environment_->logger()->DispatchBatchOfEvents(std::move(frame_events),
                                                     std::move(packet_events));
}

void Session::OnAnswer(const std::string& cast_mode,
                       const std::vector<FrameSenderConfig>& audio_configs,
                       const std::vector<FrameSenderConfig>& video_configs,
                       const ReceiverResponse& response) {
  if (!response.answer || response.type == ResponseType::UNKNOWN) {
    ReportError(ANSWER_TIME_OUT);
    return;
  }

  DCHECK_EQ(ResponseType::ANSWER, response.type);

  if (response.result != "ok") {
    ReportError(ANSWER_NOT_OK);
    return;
  }

  const Answer& answer = *response.answer;
  if (answer.cast_mode != cast_mode) {
    ReportError(ANSWER_MISMATCHED_CAST_MODE);
    return;
  }

  if (answer.send_indexes.size() != answer.ssrcs.size()) {
    ReportError(ANSWER_MISMATCHED_SSRC_LENGTH);
    return;
  }

  // Select Audio/Video config from ANSWER.
  bool has_audio = false;
  bool has_video = false;
  FrameSenderConfig audio_config;
  FrameSenderConfig video_config;
  const int video_start_idx = audio_configs.size();
  const int video_idx_bound = video_configs.size() + video_start_idx;
  for (size_t i = 0; i < answer.send_indexes.size(); ++i) {
    if (answer.send_indexes[i] < 0 ||
        answer.send_indexes[i] >= video_idx_bound) {
      ReportError(ANSWER_SELECT_INVALID_INDEX);
      return;
    }
    if (answer.send_indexes[i] < video_start_idx) {
      // Audio
      if (has_audio) {
        ReportError(ANSWER_SELECT_MULTIPLE_AUDIO);
        return;
      }
      audio_config = audio_configs[answer.send_indexes[i]];
      audio_config.receiver_ssrc = answer.ssrcs[i];
      has_audio = true;
    } else {
      // Video
      if (has_video) {
        ReportError(ANSWER_SELECT_MULTIPLE_VIDEO);
        return;
      }
      video_config = video_configs[answer.send_indexes[i] - video_start_idx];
      video_config.receiver_ssrc = answer.ssrcs[i];
      video_config.video_codec_params.number_of_encode_threads =
          NumberOfEncodeThreads();
      has_video = true;
    }
  }
  if (!has_audio && !has_video) {
    ReportError(ANSWER_NO_AUDIO_OR_VIDEO);
    return;
  }

  if ((has_audio &&
       audio_config.rtp_payload_type == RtpPayloadType::REMOTE_AUDIO) ||
      (has_video &&
       video_config.rtp_payload_type == RtpPayloadType::REMOTE_VIDEO)) {
    NOTIMPLEMENTED();  // TODO(xjz): Add support for media remoting.
    return;
  }

  // Start streaming.
  audio_encode_thread_ = base::CreateSingleThreadTaskRunnerWithTraits(
      {base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED);
  video_encode_thread_ = base::CreateSingleThreadTaskRunnerWithTraits(
      {base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED);
  cast_environment_ = new media::cast::CastEnvironment(
      base::DefaultTickClock::GetInstance(),
      base::ThreadTaskRunnerHandle::Get(), audio_encode_thread_,
      video_encode_thread_);
  auto udp_client = std::make_unique<UdpSocketClient>(
      net::IPEndPoint(sink_info_.ip_address, answer.udp_port),
      network_context_.get(),
      base::BindOnce(&Session::ReportError, weak_factory_.GetWeakPtr(),
                     SessionError::CAST_TRANSPORT_ERROR));
  cast_transport_ = media::cast::CastTransport::Create(
      cast_environment_->Clock(), kSendEventsInterval,
      std::make_unique<TransportClient>(this), std::move(udp_client),
      base::ThreadTaskRunnerHandle::Get());

  if (has_audio) {
    auto audio_sender = std::make_unique<media::cast::AudioSender>(
        cast_environment_, audio_config,
        base::BindRepeating(&Session::OnEncoderStatusChange,
                            weak_factory_.GetWeakPtr()),
        cast_transport_.get());
    audio_stream_ = std::make_unique<AudioRtpStream>(
        std::move(audio_sender), weak_factory_.GetWeakPtr());
    // TODO(xjz): Start audio capturing.
    NOTIMPLEMENTED();
  }

  if (has_video) {
    auto video_sender = std::make_unique<media::cast::VideoSender>(
        cast_environment_, video_config,
        base::BindRepeating(&Session::OnEncoderStatusChange,
                            weak_factory_.GetWeakPtr()),
        base::BindRepeating(&Session::CreateVideoEncodeAccelerator,
                            weak_factory_.GetWeakPtr()),
        base::BindRepeating(&Session::CreateVideoEncodeMemory,
                            weak_factory_.GetWeakPtr()),
        cast_transport_.get(),
        base::BindRepeating(&Session::SetTargetPlayoutDelay,
                            weak_factory_.GetWeakPtr()));
    video_stream_ = std::make_unique<VideoRtpStream>(
        std::move(video_sender), weak_factory_.GetWeakPtr());
    media::mojom::VideoCaptureHostPtr video_host;
    resource_provider_->GetVideoCaptureHost(mojo::MakeRequest(&video_host));
    video_capture_client_ = std::make_unique<VideoCaptureClient>(
        mirror_settings_.GetVideoCaptureParams(), std::move(video_host));
    video_capture_client_->Start(
        base::BindRepeating(&VideoRtpStream::InsertVideoFrame,
                            video_stream_->AsWeakPtr()),
        base::BindOnce(&Session::ReportError, weak_factory_.GetWeakPtr(),
                       SessionError::VIDEO_CAPTURE_ERROR));
  }

  const SessionMonitor::SessionType session_type =
      (has_audio && has_video)
          ? SessionMonitor::AUDIO_AND_VIDEO
          : has_audio ? SessionMonitor::AUDIO_ONLY : SessionMonitor::VIDEO_ONLY;
  session_monitor_->StartStreamingSession(cast_environment_, session_type,
                                          false /* is_remoting */);

  if (observer_)
    observer_->DidStart();
}

void Session::OnResponseParsingError(const std::string& error_message) {
  // TODO(xjz): Log the |error_message| in the mirroring logs.
}

void Session::SetTargetPlayoutDelay(base::TimeDelta playout_delay) {
  if (audio_stream_)
    audio_stream_->SetTargetPlayoutDelay(playout_delay);
  if (video_stream_)
    video_stream_->SetTargetPlayoutDelay(playout_delay);
}

void Session::CreateAndSendOffer() {
  // The random AES key and initialization vector pair used by all streams in
  // this session.
  const std::string aes_key = MakeRandomString(16);  // AES-128.
  const std::string aes_iv = MakeRandomString(16);   // AES has 128-bit blocks.
  std::vector<FrameSenderConfig> audio_configs;
  std::vector<FrameSenderConfig> video_configs;

  // Generate stream list with supported audio / video configs.
  base::Value::ListStorage stream_list;
  int stream_index = 0;
  if (sink_info_.capability != DeviceCapability::VIDEO_ONLY) {
    FrameSenderConfig config = MirrorSettings::GetDefaultAudioConfig(
        RtpPayloadType::AUDIO_OPUS, Codec::CODEC_AUDIO_OPUS);
    AddSenderConfig(base::RandInt(kAudioSsrcMin, kAudioSsrcMax), config,
                    aes_key, aes_iv, &audio_configs);
    AddStreamObject(stream_index++, "OPUS", audio_configs.back(),
                    mirror_settings_, &stream_list);
  }
  if (sink_info_.capability != DeviceCapability::AUDIO_ONLY) {
    const int32_t video_ssrc = base::RandInt(kVideoSsrcMin, kVideoSsrcMax);
    if (IsHardwareVP8EncodingSupported(GetSupportedVeaProfiles())) {
      FrameSenderConfig config = MirrorSettings::GetDefaultVideoConfig(
          RtpPayloadType::VIDEO_VP8, Codec::CODEC_VIDEO_VP8);
      config.use_external_encoder = true;
      AddSenderConfig(video_ssrc, config, aes_key, aes_iv, &video_configs);
      AddStreamObject(stream_index++, "VP8", video_configs.back(),
                      mirror_settings_, &stream_list);
    }
    if (IsHardwareH264EncodingSupported(GetSupportedVeaProfiles())) {
      FrameSenderConfig config = MirrorSettings::GetDefaultVideoConfig(
          RtpPayloadType::VIDEO_H264, Codec::CODEC_VIDEO_H264);
      config.use_external_encoder = true;
      AddSenderConfig(video_ssrc, config, aes_key, aes_iv, &video_configs);
      AddStreamObject(stream_index++, "H264", video_configs.back(),
                      mirror_settings_, &stream_list);
    }
    if (video_configs.empty()) {
      FrameSenderConfig config = MirrorSettings::GetDefaultVideoConfig(
          RtpPayloadType::VIDEO_VP8, Codec::CODEC_VIDEO_VP8);
      AddSenderConfig(video_ssrc, config, aes_key, aes_iv, &video_configs);
      AddStreamObject(stream_index++, "VP8", video_configs.back(),
                      mirror_settings_, &stream_list);
    }
  }
  DCHECK(!audio_configs.empty() || !video_configs.empty());

  // Assemble the OFFER message.
  const std::string cast_mode = "mirroring";
  base::Value offer(base::Value::Type::DICTIONARY);
  offer.SetKey("castMode", base::Value(cast_mode));
  offer.SetKey("receiverGetStatus", base::Value("true"));
  offer.SetKey("supportedStreams", base::Value(stream_list));

  const int32_t sequence_number = message_dispatcher_.GetNextSeqNumber();
  base::Value offer_message(base::Value::Type::DICTIONARY);
  offer_message.SetKey("type", base::Value("OFFER"));
  offer_message.SetKey("sessionId", base::Value(session_id_));
  offer_message.SetKey("seqNum", base::Value(sequence_number));
  offer_message.SetKey("offer", std::move(offer));

  CastMessage message_to_receiver;
  message_to_receiver.message_namespace = kWebRtcNamespace;
  const bool did_serialize_offer = base::JSONWriter::Write(
      offer_message, &message_to_receiver.json_format_data);
  DCHECK(did_serialize_offer);

  message_dispatcher_.RequestReply(
      message_to_receiver, ResponseType::ANSWER, sequence_number,
      kOfferAnswerExchangeTimeout,
      base::BindOnce(&Session::OnAnswer, base::Unretained(this), cast_mode,
                     audio_configs, video_configs));
}

}  // namespace mirroring
