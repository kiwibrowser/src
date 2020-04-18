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

#include <algorithm>

#include "libwds/common/message_handler.h"
#include "libwds/public/media_manager.h"

namespace wds {

using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;

bool MessageHandler::HandleTimeoutEvent(unsigned timer_id) const {
  return false;
}

MessageHandler::~MessageHandler() {}

MessageSequenceHandler::MessageSequenceHandler(const InitParams& init_params)
  : MessageHandler(init_params),
    current_handler_(nullptr) {
}

MessageSequenceHandler::~MessageSequenceHandler() {
}

void MessageSequenceHandler::Start() {
  if (current_handler_) {
    return;
  }
  current_handler_ = handlers_.front();
  current_handler_->Start();
}

void MessageSequenceHandler::Reset() {
  if (current_handler_) {
    current_handler_->Reset();
    current_handler_ = nullptr;
  }
}

bool MessageSequenceHandler::CanSend(Message* message) const {
  return current_handler_ && current_handler_->CanSend(message);
}

void MessageSequenceHandler::Send(std::unique_ptr<Message> message) {
  assert(current_handler_);
  current_handler_->Send(std::move(message));
}

bool MessageSequenceHandler::CanHandle(Message* message) const {
  return current_handler_ && current_handler_->CanHandle(message);
}

void MessageSequenceHandler::Handle(std::unique_ptr<Message> message) {
  assert(current_handler_);
  current_handler_->Handle(std::move(message));
}

void MessageSequenceHandler::AddSequencedHandler(MessageHandlerPtr handler) {
  assert(!current_handler_); // We are not started
  assert(handler);
  assert(handlers_.end() == std::find(
      handlers_.begin(), handlers_.end(), handler));
  handlers_.push_back(handler);
  handler->set_observer(this);
}

void MessageSequenceHandler::OnCompleted(MessageHandlerPtr handler) {
  assert(handler == current_handler_);
  current_handler_->Reset();

  auto it = std::find(handlers_.begin(), handlers_.end(), handler);
  assert(handlers_.end() != it);
  if (++it == handlers_.end()) {
    observer_->OnCompleted(shared_from_this());
    return;
  }

  current_handler_ = *it;
  current_handler_->Start();
}

void MessageSequenceHandler::OnError(MessageHandlerPtr handler) {
  assert(handler == current_handler_);
  handler->Reset();
  observer_->OnError(shared_from_this());
}

bool MessageSequenceHandler::HandleTimeoutEvent(unsigned timer_id) const {
  return current_handler_->HandleTimeoutEvent(timer_id);
}

MessageSequenceWithOptionalSetHandler::MessageSequenceWithOptionalSetHandler(
    const InitParams& init_params)
  : MessageSequenceHandler(init_params) {
}

MessageSequenceWithOptionalSetHandler::~MessageSequenceWithOptionalSetHandler() {
}

void MessageSequenceWithOptionalSetHandler::Start() {
  MessageSequenceHandler::Start();
  for (MessageHandlerPtr handler : optional_handlers_)
    handler->Start();
}

void MessageSequenceWithOptionalSetHandler::Reset() {
  MessageSequenceHandler::Reset();
  for (MessageHandlerPtr handler : optional_handlers_)
    handler->Reset();
}

bool MessageSequenceWithOptionalSetHandler::CanSend(Message* message) const {
  for (MessageHandlerPtr handler : optional_handlers_)
    if (handler->CanSend(message))
      return true;

  if (MessageSequenceHandler::CanSend(message))
    return true;

  return false;
}

void MessageSequenceWithOptionalSetHandler::Send(std::unique_ptr<Message> message) {
  for (MessageHandlerPtr handler : optional_handlers_) {
    if (handler->CanSend(message.get())) {
      handler->Send(std::move(message));
      return;
    }
  }

  if (MessageSequenceHandler::CanSend(message.get())) {
    MessageSequenceHandler::Send(std::move(message));
    return;
  }

  observer_->OnError(shared_from_this());
}

bool MessageSequenceWithOptionalSetHandler::CanHandle(Message* message) const {
  if (MessageSequenceHandler::CanHandle(message))
    return true;

  for (MessageHandlerPtr handler : optional_handlers_)
    if (handler->CanHandle(message))
      return true;

  return false;
}

void MessageSequenceWithOptionalSetHandler::Handle(std::unique_ptr<Message> message) {
  if (MessageSequenceHandler::CanHandle(message.get())) {
     MessageSequenceHandler::Handle(std::move(message));
     return;
  }

  for (MessageHandlerPtr handler : optional_handlers_) {
    if (handler->CanHandle(message.get())) {
      handler->Handle(std::move(message));
      return;
    }
  }

  observer_->OnError(shared_from_this());
}

void MessageSequenceWithOptionalSetHandler::AddOptionalHandler(
    MessageHandlerPtr handler) {
  assert(handler);
  assert(optional_handlers_.end() == std::find(
      optional_handlers_.begin(), optional_handlers_.end(), handler));
  optional_handlers_.push_back(handler);
  handler->set_observer(this);
}

void MessageSequenceWithOptionalSetHandler::OnCompleted(MessageHandlerPtr handler) {
  auto it = std::find(
      optional_handlers_.begin(), optional_handlers_.end(), handler);
  if (it != optional_handlers_.end()) {
    handler->Reset();
    handler->Start();
    return;
  }
  MessageSequenceHandler::OnCompleted(handler);
}

void MessageSequenceWithOptionalSetHandler::OnError(MessageHandlerPtr handler) {
  handler->Reset();
  observer_->OnError(shared_from_this());
}

bool MessageSequenceWithOptionalSetHandler::HandleTimeoutEvent(unsigned timer_id) const {
  for (MessageHandlerPtr handler : optional_handlers_)
    if (handler->HandleTimeoutEvent(timer_id))
      return true;
  return MessageSequenceHandler::HandleTimeoutEvent(timer_id);
}

// MessageReceiverBase
MessageReceiverBase::MessageReceiverBase(const InitParams& init_params)
  : MessageHandler(init_params),
    wait_for_message_(false) {}

MessageReceiverBase::~MessageReceiverBase() {}

bool MessageReceiverBase::CanHandle(Message* message) const {
  assert(message);
  return wait_for_message_;
}

void MessageReceiverBase::Start() { wait_for_message_ = true; }
void MessageReceiverBase::Reset() { wait_for_message_ = false; }
bool MessageReceiverBase::CanSend(Message* message) const { return false; }
void MessageReceiverBase::Send(std::unique_ptr<Message> message) {}
void MessageReceiverBase::Handle(std::unique_ptr<Message> message) {
  assert(message);
  if (!CanHandle(message.get())) {
    observer_->OnError(shared_from_this());
    return;
  }
  wait_for_message_ = false;
  std::unique_ptr<Reply> reply = HandleMessage(message.get());
  if (!reply) {
    observer_->OnError(shared_from_this());
    return;
  }
  reply->header().set_cseq(message->cseq());
  sender_->SendRTSPData(reply->ToString());
  observer_->OnCompleted(shared_from_this());
}

MessageSenderBase::MessageSenderBase(const InitParams& init_params)
  : MessageHandler(init_params) {
}

MessageSenderBase::~MessageSenderBase() {
  for (const ParcelData& data : parcel_queue_)
    sender_->ReleaseTimer(data.timer_id);
}

void MessageSenderBase::Reset() {
  while (!parcel_queue_.empty()) {
    sender_->ReleaseTimer(parcel_queue_.front().timer_id);
    parcel_queue_.pop_front();
  }
}

void MessageSenderBase::Send(std::unique_ptr<Message> message) {
  assert(message);
  if (!CanSend(message.get())) {
    observer_->OnError(shared_from_this());
    return;
  }
  parcel_queue_.push_back(
      {message->cseq(), sender_->CreateTimer(GetResponseTimeout())});
  sender_->SendRTSPData(message->ToString());
}

bool MessageSenderBase::CanHandle(Message* message) const {
  assert(message);
  return message->is_reply() && !parcel_queue_.empty() &&
         (message->cseq() == parcel_queue_.front().cseq);
}

void MessageSenderBase::Handle(std::unique_ptr<Message> message) {
  assert(message);
  if (!CanHandle(message.get())) {
    observer_->OnError(shared_from_this());
    return;
  }
  sender_->ReleaseTimer(parcel_queue_.front().timer_id);
  parcel_queue_.pop_front();

  if (!HandleReply(static_cast<Reply*>(message.get()))) {
    observer_->OnError(shared_from_this());
    return;
  }

  if (parcel_queue_.empty()) {
    observer_->OnCompleted(shared_from_this());
  }
}

bool MessageSenderBase::HandleTimeoutEvent(unsigned timer_id) const {
  for (const ParcelData& data : parcel_queue_)
    if (data.timer_id == timer_id)
      return true;

  return false;
}

int MessageSenderBase::GetResponseTimeout() const {
  return kDefaultTimeoutValue;
}

SequencedMessageSender::SequencedMessageSender(const InitParams& init_params)
  : MessageSenderBase(init_params),
    to_be_send_(nullptr) {
}

SequencedMessageSender::~SequencedMessageSender() {
}

void SequencedMessageSender::Start() {
  auto message = CreateMessage();
  to_be_send_ = message.get();
  Send(std::move(message));
}

void SequencedMessageSender::Reset() {
  to_be_send_ = nullptr;
  MessageSenderBase::Reset();
}

bool SequencedMessageSender::CanSend(Message* message) const {
  return message && (message == to_be_send_);
}

}
