/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "libwds/source/cap_negotiation_state.h"

#include "libwds/rtsp/audiocodecs.h"
#include "libwds/rtsp/clientrtpports.h"
#include "libwds/rtsp/getparameter.h"
#include "libwds/rtsp/payload.h"
#include "libwds/rtsp/presentationurl.h"
#include "libwds/rtsp/reply.h"
#include "libwds/rtsp/setparameter.h"
#include "libwds/rtsp/videoformats.h"
#include "libwds/public/media_manager.h"

namespace wds {

using rtsp::AudioCodecs;
using rtsp::ClientRtpPorts;
using rtsp::GetParameter;
using rtsp::Message;
using rtsp::Payload;
using rtsp::Property;
using rtsp::Request;
using rtsp::Reply;
using rtsp::SetParameter;
using rtsp::VideoFormats;

namespace source {

class M3Handler final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;

 private:
  std::unique_ptr<Message> CreateMessage() override;
  bool HandleReply(Reply* reply) override;
};

class M4Handler final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;

 private:
  std::unique_ptr<Message> CreateMessage() override;
  bool HandleReply(Reply* reply) override;
};

std::unique_ptr<Message> M3Handler::CreateMessage() {
  GetParameter* get_param = new GetParameter("rtsp://localhost/wfd1.0");
  get_param->header().set_cseq(sender_->GetNextCSeq());
  std::vector<std::string> props;

  SessionType media_type = ToSourceMediaManager(manager_)->GetSessionType();
  if (media_type & VideoSession)
    props.push_back("wfd_video_formats");
  if (media_type & AudioSession)
    props.push_back("wfd_audio_codecs");

  props.push_back("wfd_client_rtp_ports");
  get_param->set_payload(
      std::unique_ptr<Payload>(new rtsp::GetParameterPayload(props)));
  return std::unique_ptr<Message>(get_param);
}

bool M3Handler::HandleReply(Reply* reply) {
  if (reply->response_code() != rtsp::STATUS_OK)
    return false;

  SourceMediaManager* source_manager = ToSourceMediaManager(manager_);
  auto payload = ToPropertyMapPayload(reply->payload());
  if (!payload){
    WDS_ERROR("Failed to obtain payload from reply.");
    return false;
  }
  auto property = payload->GetProperty(rtsp::ClientRTPPortsPropertyType);
  auto ports = static_cast<ClientRtpPorts*>(property.get());
  if (!ports){
    WDS_ERROR("Failed to obtain RTP ports from source.");
    return false;
  }
  source_manager->SetSinkRtpPorts(ports->rtp_port_0(), ports->rtp_port_1());

  auto video_formats = static_cast<VideoFormats*>(
      payload->GetProperty(rtsp::VideoFormatsPropertyType).get());

  auto audio_codecs = static_cast<AudioCodecs*>(
      payload->GetProperty(rtsp::AudioCodecsPropertyType).get());

  if (!video_formats && (source_manager->GetSessionType() & VideoSession)) {
    WDS_ERROR("Failed to obtain WFD_VIDEO_FORMATS property");
    return false;
  }

  if (!audio_codecs && (source_manager->GetSessionType() & AudioSession)) {
    WDS_ERROR("Failed to obtain WFD_AUDIO_CODECS property");
    return false;
  }

  if (video_formats && !source_manager->InitOptimalVideoFormat(
      video_formats->GetNativeFormat(),
      video_formats->GetH264VideoCodecs())) {
    WDS_ERROR("Cannot initalize optimal video format from the supported by sink.");
    return false;
  }

  if (audio_codecs && !source_manager->InitOptimalAudioFormat(
      audio_codecs->audio_codecs())) {
    WDS_ERROR("Cannot initalize optimal audio format from the supported by sink.");
    return false;
  }

  return true;
}

std::unique_ptr<Message> M4Handler::CreateMessage() {
  SetParameter* set_param = new SetParameter("rtsp://localhost/wfd1.0");
  set_param->header().set_cseq(sender_->GetNextCSeq());
  SourceMediaManager* source_manager = ToSourceMediaManager(manager_);
  const auto& ports = source_manager->GetSinkRtpPorts();
  auto payload = new rtsp::PropertyMapPayload();

  payload->AddProperty(
      std::shared_ptr<Property>(new ClientRtpPorts(ports.first, ports.second)));
  std::string presentation_Url_1 = "rtsp://" + sender_->GetLocalIPAddress() + "/wfd1.0/streamid=0";
  payload->AddProperty(
      std::shared_ptr<Property>(new rtsp::PresentationUrl(presentation_Url_1, "")));

  if (source_manager->GetSessionType() & VideoSession) {
    payload->AddProperty(
        std::shared_ptr<VideoFormats>(new VideoFormats(
            NativeVideoFormat(),  // Should be all zeros.
            false,
            {source_manager->GetOptimalVideoFormat()})));
  }

  if (source_manager->GetSessionType() & AudioSession) {
    payload->AddProperty(
        std::shared_ptr<AudioCodecs>(new AudioCodecs({source_manager->GetOptimalAudioFormat()})));
  }

  set_param->set_payload(std::unique_ptr<Payload>(payload));

  return std::unique_ptr<Message>(set_param);
}

bool M4Handler::HandleReply(Reply* reply) {
  return (reply->response_code() == rtsp::STATUS_OK);
}

CapNegotiationState::CapNegotiationState(const InitParams &init_params)
  : MessageSequenceHandler(init_params) {
  AddSequencedHandler(make_ptr(new M3Handler(init_params)));
  AddSequencedHandler(make_ptr(new M4Handler(init_params)));
}

CapNegotiationState::~CapNegotiationState() {
}

}  // namespace source
}  // namespace wds
