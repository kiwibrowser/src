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

#include "libwds/public/source.h"

#include "libwds/source/cap_negotiation_state.h"
#include "libwds/source/init_state.h"
#include "libwds/source/streaming_state.h"
#include "libwds/source/session_state.h"
#include "libwds/common/message_handler.h"
#include "libwds/common/rtsp_input_handler.h"
#include "libwds/public/wds_export.h"
#include "libwds/rtsp/getparameter.h"
#include "libwds/rtsp/setparameter.h"
#include "libwds/rtsp/triggermethod.h"
#include "libwds/public/media_manager.h"

namespace wds {

using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;

namespace {

bool InitializeRequestId(Request* request) {
  Request::ID id = Request::UNKNOWN;
  switch(request->method()) {
  case Request::MethodOptions:
    id = Request::M2;
    break;
  case Request::MethodSetup:
    id = Request::M6;
    break;
  case Request::MethodPlay:
    id = Request::M7;
    break;
  case Request::MethodTeardown:
    id = Request::M8;
    break;
  case Request::MethodPause:
    id = Request::M9;
    break;
  case Request::MethodSetParameter:
    if (auto payload = rtsp::ToPropertyMapPayload(request->payload())) {
      if (payload->HasProperty(rtsp::RoutePropertyType))
        id = Request::M10;
      else if (payload->HasProperty(rtsp::ConnectorTypePropertyType))
        id = Request::M11;
      else if (payload->HasProperty(rtsp::StandbyPropertyType))
        id = Request::M12;
      else if (payload->HasProperty(rtsp::IDRRequestPropertyType))
        id = Request::M13;
      else if (payload->HasProperty(rtsp::UIBCCapabilityPropertyType))
        id = Request::M14;
      else if (payload->HasProperty(rtsp::UIBCSettingPropertyType))
        id = Request::M15;
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

class SourceStateMachine : public MessageSequenceHandler {
 public:
   SourceStateMachine(const InitParams& init_params, unsigned& timer_id)
     : MessageSequenceHandler(init_params) {
     MessageHandlerPtr m16_sender = make_ptr(new source::M16Sender(init_params));
     AddSequencedHandler(make_ptr(new source::InitState(init_params)));
     AddSequencedHandler(make_ptr(new source::CapNegotiationState(init_params)));
     AddSequencedHandler(make_ptr(new source::SessionState(init_params, timer_id, m16_sender)));
     AddSequencedHandler(make_ptr(new source::StreamingState(init_params, m16_sender)));
   }
};

class SourceImpl final : public Source, public RTSPInputHandler, public MessageHandler::Observer {
 public:
  SourceImpl(Delegate* delegate, SourceMediaManager* mng, Peer::Observer* observer);

 private:
  // Source implementation.
  void Start() override;
  void Reset() override;
  void RTSPDataReceived(const std::string& message) override;
  bool Teardown() override;
  bool Play() override;
  bool Pause() override;

  // public MessageHandler::Observer
  void OnCompleted(MessageHandlerPtr handler) override;
  void OnError(MessageHandlerPtr handler) override;

  void OnTimerEvent(unsigned timer_id) override;

  // RTSPInputHandler
  void MessageParsed(std::unique_ptr<Message> message) override;
  void ParserErrorOccurred(const std::string& invalid_input) override;

  // Keep-alive function
  void SendKeepAlive();
  void ResetAndTeardownMedia();

  unsigned keep_alive_timer_;
  std::shared_ptr<SourceStateMachine> state_machine_;
  Delegate* delegate_;
  SourceMediaManager* media_manager_;
  Peer::Observer* observer_;
};

SourceImpl::SourceImpl(Delegate* delegate, SourceMediaManager* mng, Peer::Observer* observer)
  : keep_alive_timer_(0),
    state_machine_(new SourceStateMachine({delegate, mng, this}, keep_alive_timer_)),
    delegate_(delegate),
    media_manager_(mng),
    observer_(observer) {
}

void SourceImpl::Start() {
  state_machine_->Start();
}

void SourceImpl::Reset() {
  state_machine_->Reset();
  delegate_->ReleaseTimer(keep_alive_timer_);
}

void SourceImpl::RTSPDataReceived(const std::string& message) {
  AddInput(message);
}

void SourceImpl::OnTimerEvent(unsigned timer_id) {
  if (keep_alive_timer_ == timer_id)
    SendKeepAlive();
  else if (state_machine_->HandleTimeoutEvent(timer_id) && observer_)
    observer_->ErrorOccurred(TimeoutError);
}

void SourceImpl::SendKeepAlive() {
  delegate_->ReleaseTimer(keep_alive_timer_);
  auto get_param = std::unique_ptr<Request>(
      new rtsp::GetParameter("rtsp://localhost/wfd1.0"));
  get_param->header().set_cseq(delegate_->GetNextCSeq());
  get_param->set_id(Request::M16);

  assert(state_machine_->CanSend(get_param.get()));
  state_machine_->Send(std::move(get_param));
  keep_alive_timer_ =
      delegate_->CreateTimer(kDefaultKeepAliveTimeout - kDefaultTimeoutValue);
  assert(keep_alive_timer_);
}

namespace  {

std::unique_ptr<Message> CreateM5(int send_cseq, rtsp::TriggerMethod::Method method) {
  auto set_param = std::unique_ptr<Request>(
      new rtsp::SetParameter("rtsp://localhost/wfd1.0"));
  set_param->header().set_cseq(send_cseq);
  auto payload = new rtsp::PropertyMapPayload();
  payload->AddProperty(
      std::shared_ptr<rtsp::Property>(new rtsp::TriggerMethod(method)));
  set_param->set_payload(std::unique_ptr<rtsp::Payload>(payload));
  set_param->set_id(Request::M5);
  return std::move(set_param);
}

}

bool SourceImpl::Teardown() {
  auto m5 = CreateM5(delegate_->GetNextCSeq(),
                     rtsp::TriggerMethod::TEARDOWN);

  if (!state_machine_->CanSend(m5.get()))
    return false;
  state_machine_->Send(std::move(m5));
  return true;
}

bool SourceImpl::Play() {
  auto m5 = CreateM5(delegate_->GetNextCSeq(),
                     rtsp::TriggerMethod::PLAY);

  if (!state_machine_->CanSend(m5.get()))
    return false;
  state_machine_->Send(std::move(m5));
  return true;
}

bool SourceImpl::Pause() {
  auto m5 = CreateM5(delegate_->GetNextCSeq(),
                     rtsp::TriggerMethod::PAUSE);

  if (!state_machine_->CanSend(m5.get()))
    return false;
  state_machine_->Send(std::move(m5));
  return true;
}

void SourceImpl::OnCompleted(MessageHandlerPtr handler) {
  assert(handler == state_machine_);
  if (observer_)
    observer_->SessionCompleted();
}

void SourceImpl::OnError(MessageHandlerPtr handler) {
  assert(handler == state_machine_);
  if (observer_)
    observer_->ErrorOccurred(UnexpectedMessageError);
}

void SourceImpl::MessageParsed(std::unique_ptr<Message> message) {
  if (message->is_request() && !InitializeRequestId(ToRequest(message.get()))) {
    WDS_ERROR("Cannot identify the received message");
    if (observer_)
      observer_->ErrorOccurred(UnexpectedMessageError);
    return;
  }
  if (!state_machine_->CanHandle(message.get())) {
    WDS_ERROR("Cannot handle the received message with Id: %d", ToRequest(message.get())->id());
    if (observer_)
      observer_->ErrorOccurred(UnexpectedMessageError);
    return;
  }
  state_machine_->Handle(std::move(message));
}

void SourceImpl::ParserErrorOccurred(const std::string& invalid_input) {
  WDS_ERROR("Failed to parse: %s", invalid_input.c_str());
  if (observer_)
    observer_->ErrorOccurred(MessageParseError);
}

Source* Source::Create(Delegate* delegate, SourceMediaManager* mng, Peer::Observer* observer) {
  return new SourceImpl(delegate, mng, observer);
}

}  // namespace wds
