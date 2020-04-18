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

#include "libwds/source/session_state.h"

#include "libwds/public/media_manager.h"
#include "libwds/source/cap_negotiation_state.h"
#include "libwds/rtsp/reply.h"
#include "libwds/rtsp/setparameter.h"
#include "libwds/rtsp/triggermethod.h"

namespace wds {

using rtsp::Message;
using rtsp::Payload;
using rtsp::Request;
using rtsp::Reply;

namespace source {

class M5Handler final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;

 private:
  std::unique_ptr<Message> CreateMessage() override {
    rtsp::SetParameter* set_param = new rtsp::SetParameter("rtsp://localhost/wfd1.0");
    set_param->header().set_cseq(sender_->GetNextCSeq());
    auto payload = new rtsp::PropertyMapPayload();
    payload->AddProperty(
        std::shared_ptr<rtsp::Property>(new rtsp::TriggerMethod(rtsp::TriggerMethod::SETUP)));
    set_param->set_payload(std::unique_ptr<Payload>(payload));
    return std::unique_ptr<Message>(set_param);
  }

  bool HandleReply(Reply* reply) override {
    return (reply->response_code() == rtsp::STATUS_OK);
  }

};

class M6Handler final : public MessageReceiver<Request::M6> {
 public:
  M6Handler(const InitParams& init_params, unsigned& timer_id)
    : MessageReceiver<Request::M6>(init_params),
      keep_alive_timer_(timer_id) {
  }

  std::unique_ptr<Reply> HandleMessage(
      Message* message) override {
    auto reply = std::unique_ptr<Reply>(new Reply(rtsp::STATUS_OK));
    reply->header().set_session(manager_->GetSessionId());
    reply->header().set_timeout(kDefaultKeepAliveTimeout);

    auto transport = new rtsp::TransportHeader();
    // we assume here that there is no coupled secondary sink
    transport->set_client_port(ToSourceMediaManager(manager_)->GetSinkRtpPorts().first);
    transport->set_server_port(ToSourceMediaManager(manager_)->GetLocalRtpPort());
    reply->header().set_transport(transport);

    return reply;
  }

  void Handle(std::unique_ptr<Message> message) override {
    MessageReceiver<Request::M6>::Handle(std::move(message));
    keep_alive_timer_ =
        sender_->CreateTimer(kDefaultTimeoutValue);
  }

  unsigned& keep_alive_timer_;
};

M7Handler::M7Handler(const InitParams& init_params)
  : MessageReceiver<Request::M7>(init_params) {
}

std::unique_ptr<Reply> M7Handler::HandleMessage(
    Message* message) {
  if (!manager_->IsPaused())
    return std::unique_ptr<Reply>(new Reply(rtsp::STATUS_NotAcceptable));
  manager_->Play();
  return std::unique_ptr<Reply>(new Reply(rtsp::STATUS_OK));
}

M16Sender::M16Sender(const InitParams& init_params)
  : OptionalMessageSender<Request::M16>(init_params) {
}

bool M16Sender::HandleReply(Reply* reply) {
  return (reply->response_code() == rtsp::STATUS_OK);
}

SessionState::SessionState(const InitParams& init_params, unsigned& timer_id,
    MessageHandlerPtr& m16_sender)
  : MessageSequenceWithOptionalSetHandler(init_params) {
  AddSequencedHandler(make_ptr(new M5Handler(init_params)));
  AddSequencedHandler(make_ptr(new M6Handler(init_params, timer_id)));
  AddSequencedHandler(make_ptr(new M7Handler(init_params)));

  AddOptionalHandler(m16_sender);
}

SessionState::~SessionState() {
}

}  // source
}  // wds
