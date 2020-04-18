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

#ifndef LIBWDS_COMMON_MESSAGE_HANDLER_H_
#define LIBWDS_COMMON_MESSAGE_HANDLER_H_

#include <cassert>
#include <list>
#include <vector>
#include <memory>
#include <utility>

#include "libwds/rtsp/message.h"
#include "libwds/rtsp/reply.h"
#include "libwds/public/logging.h"
#include "libwds/public/peer.h"

namespace wds {

// Default keep-alive timer is 60 seconds
const int kDefaultKeepAliveTimeout = 60;
// Default timeout for RTSP message exchange
const int kDefaultTimeoutValue = 5;

class MediaManager;

class MessageHandler;
using MessageHandlerPtr = std::shared_ptr<MessageHandler>;

inline MessageHandlerPtr make_ptr(MessageHandler* handler) {
  return MessageHandlerPtr(handler);
}

class MessageHandler : public std::enable_shared_from_this<MessageHandler> {
 public:
  class Observer {
   public:
    virtual void OnCompleted(MessageHandlerPtr handler) {}
    virtual void OnError(MessageHandlerPtr handler) {}

   protected:
    virtual ~Observer() {}
  };

  struct InitParams {
    Peer::Delegate* sender;
    MediaManager* manager;
    Observer* observer;
  };

  virtual ~MessageHandler();

  virtual void Start() = 0;
  virtual void Reset() = 0;

  virtual bool CanSend(rtsp::Message* message) const = 0;
  virtual void Send(std::unique_ptr<rtsp::Message> message) = 0;

  virtual bool CanHandle(rtsp::Message* message) const = 0;
  virtual void Handle(std::unique_ptr<rtsp::Message> message) = 0;

  // For handlers that require timeout
  virtual bool HandleTimeoutEvent(unsigned timer_id) const;

  void set_observer(Observer* observer) {
    assert(observer);
    observer_ = observer;
  }

protected:
  explicit MessageHandler(const InitParams& init_params)
    : sender_(init_params.sender),
      manager_(init_params.manager),
      observer_(init_params.observer) {
    assert(sender_);
    assert(manager_);
    assert(observer_);
  }

  Peer::Delegate* sender_;
  MediaManager* manager_;
  Observer* observer_;
};

class MessageSequenceHandler : public MessageHandler,
                               public MessageHandler::Observer {
 public:
  explicit MessageSequenceHandler(const InitParams& init_params);
  ~MessageSequenceHandler() override;
  void Start() override;
  void Reset() override;

  bool CanSend(rtsp::Message* message) const override;
  void Send(std::unique_ptr<rtsp::Message> message) override;

  bool CanHandle(rtsp::Message* message) const override;
  void Handle(std::unique_ptr<rtsp::Message> message) override;

  bool HandleTimeoutEvent(unsigned timer_id) const override;

 protected:
  void AddSequencedHandler(MessageHandlerPtr handler);
  // MessageHandler::Observer implementation.
  void OnCompleted(MessageHandlerPtr handler) override;
  void OnError(MessageHandlerPtr handler) override;

  std::vector<MessageHandlerPtr> handlers_;
  MessageHandlerPtr current_handler_;
};

class MessageSequenceWithOptionalSetHandler : public MessageSequenceHandler {
 public:
  explicit MessageSequenceWithOptionalSetHandler(const InitParams& init_params);
  ~MessageSequenceWithOptionalSetHandler() override;
  void Start() override;
  void Reset() override;
  bool CanSend(rtsp::Message* message) const override;
  void Send(std::unique_ptr<rtsp::Message> message) override;
  bool CanHandle(rtsp::Message* message) const override;
  void Handle(std::unique_ptr<rtsp::Message> message) override;

  bool HandleTimeoutEvent(unsigned timer_id) const override;

 protected:
  void AddOptionalHandler(MessageHandlerPtr handler);
  // MessageHandler::Observer implementation.
  void OnCompleted(MessageHandlerPtr handler) override;
  void OnError(MessageHandlerPtr handler) override;

  std::vector<MessageHandlerPtr> optional_handlers_;
};

// This is aux classes to handle single message.
// There are two common scenarious:
// 1. We send a message and wait for reply
// class Handler : public MessageSender
//
// 2. We wait for the message and reply ourselves.
// class Handler : public MessageReceiver<type of the message
// we're waiting for>
class MessageReceiverBase : public MessageHandler {
 public:
  explicit MessageReceiverBase(const InitParams& init_params);
  ~MessageReceiverBase() override;

 protected:
  virtual std::unique_ptr<wds::rtsp::Reply> HandleMessage(rtsp::Message* message) = 0;
  bool CanHandle(rtsp::Message* message) const override;
  void Handle(std::unique_ptr<rtsp::Message> message) override;

 private:
  void Start() override;
  void Reset() override;
  bool CanSend(rtsp::Message* message) const override;
  void Send(std::unique_ptr<rtsp::Message> message) override;

  bool wait_for_message_;
};

template <rtsp::Request::ID id>
class MessageReceiver : public MessageReceiverBase {
 public:
  using MessageReceiverBase::MessageReceiverBase;

 protected:
  bool CanHandle(rtsp::Message* message) const override {
    return MessageReceiverBase::CanHandle(message) && message->is_request() &&
           id == ToRequest(message)->id();
  }
};

class MessageSenderBase : public MessageHandler {
 public:
  explicit MessageSenderBase(const InitParams& init_params);
  ~MessageSenderBase() override;

 protected:
  virtual bool HandleReply(rtsp::Reply* reply) = 0;
  void Send(std::unique_ptr<rtsp::Message> message) override;
  void Reset() override;
  bool HandleTimeoutEvent(unsigned timer_id) const override;


 private:
  bool CanHandle(rtsp::Message* message) const override;
  void Handle(std::unique_ptr<rtsp::Message> message) override;

  virtual int GetResponseTimeout() const;

  struct ParcelData {
    int cseq;
    unsigned timer_id;
  };
  std::list<ParcelData> parcel_queue_;
};

// To be used for optional senders.
template <rtsp::Request::ID id>
class OptionalMessageSender : public MessageSenderBase {
 public:
  using MessageSenderBase::MessageSenderBase;

 protected:
  bool CanSend(rtsp::Message* message) const override {
    assert(message);
    return message->is_request() && ToRequest(message)->id() == id;
  }

 private:
  void Start() override {}
};

// To be used for sequensed senders.
class SequencedMessageSender : public MessageSenderBase {
 public:
  explicit SequencedMessageSender(const InitParams& init_params);
  ~SequencedMessageSender() override;

 protected:
  virtual std::unique_ptr<rtsp::Message> CreateMessage() = 0;

 private:
  void Start() override;
  void Reset() override;
  bool CanSend(rtsp::Message* message) const override;

  rtsp::Message* to_be_send_;
};

}  // namespace wds
#endif // LIBWDS_COMMON_MESSAGE_HANDLER_H_
