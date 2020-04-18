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

#include "libwds/sink/session_state.h"

#include "libwds/public/media_manager.h"

#include "libwds/rtsp/play.h"
#include "libwds/rtsp/reply.h"
#include "libwds/rtsp/setup.h"
#include "libwds/rtsp/transportheader.h"
#include "libwds/sink/cap_negotiation_state.h"
#include "libwds/sink/streaming_state.h"

namespace wds {
using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;

namespace sink {

M16Handler::M16Handler(const InitParams& init_params, unsigned& keep_alive_timer)
  : MessageReceiver<Request::M16>(init_params),
    keep_alive_timer_(keep_alive_timer) { }

bool M16Handler::HandleTimeoutEvent(unsigned timer_id) const {
  return timer_id == keep_alive_timer_;
}

std::unique_ptr<Reply> M16Handler::HandleMessage(Message* message) {
  // Reset keep alive timer;
  sender_->ReleaseTimer(keep_alive_timer_);
  keep_alive_timer_ = sender_->CreateTimer(60);

  return std::unique_ptr<Reply>(new Reply(rtsp::STATUS_OK));
}

M6Handler::M6Handler(const InitParams& init_params, unsigned& keep_alive_timer)
  : SequencedMessageSender(init_params),
    keep_alive_timer_(keep_alive_timer) {}

std::unique_ptr<Message> M6Handler::CreateMessage() {
  auto setup = new rtsp::Setup(ToSinkMediaManager(manager_)->GetPresentationUrl());
  auto transport = new rtsp::TransportHeader();
  // we assume here that there is no coupled secondary sink
  transport->set_client_port(ToSinkMediaManager(manager_)->GetLocalRtpPorts().first);
  setup->header().set_transport(transport);
  setup->header().set_cseq(sender_->GetNextCSeq());
  setup->header().set_require_wfd_support(true);

  return std::unique_ptr<Message>(setup);
}

bool M6Handler::HandleReply(Reply* reply) {
  const std::string& session_id = reply->header().session();
  if(reply->response_code() == rtsp::STATUS_OK && !session_id.empty()) {
    ToSinkMediaManager(manager_)->SetSessionId(session_id);
    // FIXME : take timeout value from session.
    keep_alive_timer_ = sender_->CreateTimer(60);
    return true;
  }

  return false;
}

class M7Handler final : public SequencedMessageSender {
 public:
    using SequencedMessageSender::SequencedMessageSender;
 private:
  std::unique_ptr<Message> CreateMessage() override {
    rtsp::Play* play = new rtsp::Play(ToSinkMediaManager(manager_)->GetPresentationUrl());
    play->header().set_session(manager_->GetSessionId());
    play->header().set_cseq(sender_->GetNextCSeq());
    play->header().set_require_wfd_support(true);

    return std::unique_ptr<Message>(play);
  }

  bool HandleReply(Reply* reply) override {
    return (reply->response_code() == rtsp::STATUS_OK);
  }
};

SessionState::SessionState(const InitParams& init_params, MessageHandlerPtr m6_handler, MessageHandlerPtr m16_handler)
  : MessageSequenceWithOptionalSetHandler(init_params) {
  AddSequencedHandler(m6_handler);
  AddSequencedHandler(make_ptr(new M7Handler(init_params)));

  AddOptionalHandler(make_ptr(new M3Handler(init_params)));
  AddOptionalHandler(make_ptr(new M4Handler(init_params)));
  AddOptionalHandler(make_ptr(new TeardownHandler(init_params)));
  AddOptionalHandler(m16_handler);
}

}  // namespace sink
}  // namespace wds
