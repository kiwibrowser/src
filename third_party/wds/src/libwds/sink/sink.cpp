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

#include "libwds/public/sink.h"

#include "libwds/common/message_handler.h"
#include "libwds/common/rtsp_input_handler.h"
#include "libwds/public/wds_export.h"
#include "libwds/rtsp/pause.h"
#include "libwds/rtsp/play.h"
#include "libwds/rtsp/teardown.h"
#include "libwds/rtsp/triggermethod.h"
#include "libwds/public/media_manager.h"
#include "libwds/sink/cap_negotiation_state.h"
#include "libwds/sink/init_state.h"
#include "libwds/sink/session_state.h"
#include "libwds/sink/streaming_state.h"

namespace wds {
using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;

namespace {

// todo: check mandatory parameters for each message
bool InitializeRequestId(Request* request) {
  Request::ID id = Request::UNKNOWN;
  switch(request->method()) {
  case Request::MethodOptions:
    id = Request::M1;
    break;
  case Request::MethodGetParameter:
    if (auto payload = rtsp::ToGetParameterPayload(request->payload())) {
      if (!payload->properties().empty())
        id = Request::M3;
    } else {
      id = Request::M16;
    }
    break;
  case Request::MethodSetParameter:
    if (auto payload = rtsp::ToPropertyMapPayload(request->payload())) {
      if (payload->HasProperty(rtsp::PresentationURLPropertyType))
        id = Request::M4;
      else if (payload->HasProperty(rtsp::AVFormatChangeTimingPropertyType))
        id = Request::M4;
      else if (payload->HasProperty(rtsp::TriggerMethodPropertyType))
        id = Request::M5;
      break;
    }
  default:
    WDS_ERROR("Failed to identify the received message");
    return false;
  }

  request->set_id(id);
  return true;
}

}

class SinkStateMachine : public MessageSequenceHandler {
 public:
   SinkStateMachine(const InitParams& init_params)
     : MessageSequenceHandler(init_params),
       keep_alive_timer_(0) {
     auto m6_handler = make_ptr(new sink::M6Handler(init_params, keep_alive_timer_));
     auto m16_handler = make_ptr(new sink::M16Handler(init_params, keep_alive_timer_));
     AddSequencedHandler(make_ptr(new sink::InitState(init_params)));
     AddSequencedHandler(make_ptr(new sink::CapNegotiationState(init_params)));
     AddSequencedHandler(make_ptr(new sink::SessionState(init_params, m6_handler, m16_handler)));
     AddSequencedHandler(make_ptr(new sink::StreamingState(init_params, m16_handler)));
   }

   SinkStateMachine(Peer::Delegate* sender, SinkMediaManager* mng)
     : SinkStateMachine({sender, mng, this}) {}

 private:
   unsigned keep_alive_timer_;
};

class SinkImpl final : public Sink, public RTSPInputHandler, public MessageHandler::Observer {
 public:
  SinkImpl(Delegate* delegate, SinkMediaManager* mng);

 private:
  // Sink implementation.
  void Start() override;
  void Reset() override;
  void RTSPDataReceived(const std::string& message) override;
  bool Teardown() override;
  bool Play() override;
  bool Pause() override;

  // RTSPInputHandler
  void MessageParsed(std::unique_ptr<Message> message) override;

  // public MessageHandler::Observer
  void OnCompleted(MessageHandlerPtr handler) override;
  void OnError(MessageHandlerPtr handler) override;
  void OnTimerEvent(unsigned timer_id) override;

  bool HandleCommand(std::unique_ptr<Message> command);

  template <class WfdMessage, Request::ID id>
  std::unique_ptr<Message> CreateCommand();

  void ResetAndTeardownMedia();

  std::shared_ptr<SinkStateMachine> state_machine_;
  Delegate* delegate_;
  SinkMediaManager* manager_;
};

SinkImpl::SinkImpl(Delegate* delegate, SinkMediaManager* mng)
  : state_machine_(new SinkStateMachine({delegate, mng, this})),
    delegate_(delegate),
    manager_(mng) {
}

void SinkImpl::Start() {
  state_machine_->Start();
}

void SinkImpl::Reset() {
  state_machine_->Reset();
}

void SinkImpl::RTSPDataReceived(const std::string& message) {
  AddInput(message);
}

template <class WfdMessage, Request::ID id>
std::unique_ptr<Message> SinkImpl::CreateCommand() {
  auto message = new WfdMessage(manager_->GetPresentationUrl());
  message->header().set_session(manager_->GetSessionId());
  message->header().set_cseq(delegate_->GetNextCSeq());
  message->set_id(id);
  return std::unique_ptr<Message>(message);
}

bool SinkImpl::HandleCommand(std::unique_ptr<Message> command) {
  if (manager_->GetSessionId().empty() ||
      manager_->GetPresentationUrl().empty())
    return false;

  if (!state_machine_->CanSend(command.get()))
    return false;
  state_machine_->Send(std::move(command));
  return true;
}

bool SinkImpl::Teardown() {
  return HandleCommand(CreateCommand<rtsp::Teardown, Request::M8>());
}

bool SinkImpl::Play() {
  return HandleCommand(CreateCommand<rtsp::Play, Request::M7>());
}

bool SinkImpl::Pause() {
  return HandleCommand(CreateCommand<rtsp::Pause, Request::M9>());
}

void SinkImpl::MessageParsed(std::unique_ptr<Message> message) {
  if (message->is_request() && !InitializeRequestId(ToRequest(message.get()))) {
    WDS_ERROR("Cannot identify the received message");
    return;
  }
  if (!state_machine_->CanHandle(message.get())) {
    WDS_ERROR("Cannot handle the received message with Id: %d", ToRequest(message.get())->id());
    return;
  }
  state_machine_->Handle(std::move(message));
}

void SinkImpl::ResetAndTeardownMedia() {
  manager_->Teardown();
  state_machine_->Reset();
}

void SinkImpl::OnCompleted(MessageHandlerPtr handler) {
  assert(handler == state_machine_);
  ResetAndTeardownMedia();
}

void SinkImpl::OnError(MessageHandlerPtr handler) {
   assert(handler == state_machine_);
   ResetAndTeardownMedia();
}

void SinkImpl::OnTimerEvent(unsigned timer_id) {
  if (state_machine_->HandleTimeoutEvent(timer_id))
    state_machine_->Reset();
}

Sink* Sink::Create(Delegate* delegate, SinkMediaManager* mng) {
  return new SinkImpl(delegate, mng);
}

}  // namespace wds
