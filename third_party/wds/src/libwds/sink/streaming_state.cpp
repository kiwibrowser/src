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

#include "libwds/sink/streaming_state.h"

#include "libwds/public/media_manager.h"

#include "libwds/rtsp/pause.h"
#include "libwds/rtsp/play.h"
#include "libwds/rtsp/reply.h"
#include "libwds/rtsp/teardown.h"
#include "libwds/rtsp/triggermethod.h"
#include "libwds/sink/cap_negotiation_state.h"
#include "libwds/sink/session_state.h"

namespace wds {
using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;
using rtsp::TriggerMethod;

namespace sink {

template <TriggerMethod::Method method>
class M5Handler final : public MessageReceiver<Request::M5> {
 public:
  explicit M5Handler(const InitParams& init_params)
    : MessageReceiver<Request::M5>(init_params) {
  }

  bool CanHandle(Message* message) const override {
    if (!MessageReceiver<Request::M5>::CanHandle(message))
      return false;
    auto payload = ToPropertyMapPayload(message->payload());
    if (!payload) {
      WDS_ERROR("Failed to obtain payload in M5 handler.");
      return false;
    }
    auto property =
        static_cast<TriggerMethod*>(
            payload->GetProperty(rtsp::TriggerMethodPropertyType).get());
    return method == property->method();
  }

  std::unique_ptr<Reply> HandleMessage(Message* message) override {
    return std::unique_ptr<Reply>(new Reply());
  }
};

class M7Sender final : public SequencedMessageSender {
 public:
    using SequencedMessageSender::SequencedMessageSender;
 private:
  std::unique_ptr<Message> CreateMessage() override {
    rtsp::Play* play = new rtsp::Play(ToSinkMediaManager(manager_)->GetPresentationUrl());
    play->header().set_session(manager_->GetSessionId());
    play->header().set_cseq (sender_->GetNextCSeq());
    return std::unique_ptr<Message>(play);
  }

  bool HandleReply(Reply* reply) override {
    if (reply->response_code() == rtsp::STATUS_OK) {
      manager_->Play();
      return true;
    }
    return false;
  }
};

class PlayHandler : public MessageSequenceHandler {
 public:
  explicit PlayHandler(const InitParams& init_params)
  : MessageSequenceHandler(init_params) {
    AddSequencedHandler(make_ptr(new M5Handler<TriggerMethod::PLAY>(init_params)));
    AddSequencedHandler(make_ptr(new M7Sender(init_params)));
  }
};

class M8Sender final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;
 private:
  std::unique_ptr<Message> CreateMessage() override {
    rtsp::Teardown* teardown = new rtsp::Teardown(ToSinkMediaManager(manager_)->GetPresentationUrl());
    teardown->header().set_session(manager_->GetSessionId());
    teardown->header().set_cseq(sender_->GetNextCSeq());
    return std::unique_ptr<Message>(teardown);
  }

  bool HandleReply(Reply* reply) override {
    if (!manager_->GetSessionId().empty() &&
        (reply->response_code() == rtsp::STATUS_OK)) {
      manager_->Teardown();
      return true;
    }
    return false;
  }
};

TeardownHandler::TeardownHandler(const InitParams& init_params)
  : MessageSequenceHandler(init_params) {
  AddSequencedHandler(make_ptr(new M5Handler<TriggerMethod::TEARDOWN>(init_params)));
  AddSequencedHandler(make_ptr(new M8Sender(init_params)));
}

class M9Sender final : public SequencedMessageSender {
 public:
    using SequencedMessageSender::SequencedMessageSender;
 private:
  std::unique_ptr<Message> CreateMessage() override {
    rtsp::Pause* pause = new rtsp::Pause(ToSinkMediaManager(manager_)->GetPresentationUrl());
    pause->header().set_session(manager_->GetSessionId());
    pause->header().set_cseq(sender_->GetNextCSeq());
    return std::unique_ptr<Message>(pause);
  }

  bool HandleReply(Reply* reply) override {
    if (reply->response_code() == rtsp::STATUS_OK) {
      manager_->Pause();
      return true;
    }
    return false;
  }
};

class PauseHandler : public MessageSequenceHandler {
 public:
  explicit PauseHandler(const InitParams& init_params)
  : MessageSequenceHandler(init_params) {
    AddSequencedHandler(make_ptr(new M5Handler<TriggerMethod::PAUSE>(init_params)));
    AddSequencedHandler(make_ptr(new M9Sender(init_params)));
  }
};

class M7SenderOptional final : public OptionalMessageSender<Request::M7> {
 public:
  M7SenderOptional(const InitParams& init_params)
    : OptionalMessageSender<Request::M7>(init_params) {
  }
 private:
  bool HandleReply(Reply* reply) override {
    if (reply->response_code() == rtsp::STATUS_OK) {
      manager_->Play();
      return true;
    }
    return false;
  }

  bool CanSend(Message* message) const override {
    if (OptionalMessageSender<Request::M7>::CanSend(message)
        && manager_->IsPaused())
      return true;
    return false;
  }
};

class M8SenderOptional final : public OptionalMessageSender<Request::M8> {
 public:
  M8SenderOptional(const InitParams& init_params)
    : OptionalMessageSender<Request::M8>(init_params) {
  }
 private:
  bool HandleReply(Reply* reply) override {
    // todo: if successfull, switch to init state
    if (reply->response_code() == rtsp::STATUS_OK) {
      manager_->Teardown();
      return true;
    }

    return false;
  }
};

class M9SenderOptional final : public OptionalMessageSender<Request::M9> {
 public:
  M9SenderOptional(const InitParams& init_params)
    : OptionalMessageSender<Request::M9>(init_params) {
  }
 private:
  bool HandleReply(Reply* reply) override {
    if (reply->response_code() == rtsp::STATUS_OK) {
      manager_->Pause();
      return true;
    }
    return false;
  }

  bool CanSend(Message* message) const override {
    if (OptionalMessageSender<Request::M9>::CanSend(message)
        && !manager_->IsPaused())
      return true;
    return false;
  }
};

StreamingState::StreamingState(const InitParams& init_params, MessageHandlerPtr m16_handler)
  : MessageSequenceWithOptionalSetHandler(init_params) {
  AddSequencedHandler(make_ptr(new TeardownHandler(init_params)));
  AddOptionalHandler(make_ptr(new PlayHandler(init_params)));
  AddOptionalHandler(make_ptr(new PauseHandler(init_params)));
  AddOptionalHandler(make_ptr(new M3Handler(init_params)));
  AddOptionalHandler(make_ptr(new M4Handler(init_params)));

  // optional senders that handle sending play, pause and teardown
  AddOptionalHandler(make_ptr(new M7SenderOptional(init_params)));
  AddOptionalHandler(make_ptr(new M8SenderOptional(init_params)));
  AddOptionalHandler(make_ptr(new M9SenderOptional(init_params)));
  AddOptionalHandler(m16_handler);
}

}  // sink
}  // wds
