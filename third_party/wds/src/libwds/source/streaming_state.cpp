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

#include "libwds/source/streaming_state.h"

#include "libwds/public/media_manager.h"
#include "libwds/source/cap_negotiation_state.h"
#include "libwds/source/session_state.h"
#include "libwds/rtsp/reply.h"

namespace wds {

using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;

namespace source {

class M8Handler final : public MessageReceiver<rtsp::Request::M8> {
 public:
  M8Handler(const InitParams& init_params)
    : MessageReceiver<Request::M8>(init_params) {
  }

 private:
  std::unique_ptr<rtsp::Reply> HandleMessage(
      rtsp::Message* message) override {
    manager_->Teardown();
    return std::unique_ptr<Reply>(new Reply(rtsp::STATUS_OK));
  }
};

class M9Handler final : public MessageReceiver<Request::M9> {
 public:
  M9Handler(const InitParams& init_params)
    : MessageReceiver<Request::M9>(init_params) {
  }

  std::unique_ptr<Reply> HandleMessage(
      Message* message) override {
    int response_code = rtsp::STATUS_NotAcceptable;
    if (!manager_->IsPaused()) {
      manager_->Pause();
      response_code = rtsp::STATUS_OK;
    }
    return std::unique_ptr<Reply>(new Reply(response_code));
  }
};

class M5Sender final : public OptionalMessageSender<Request::M5> {
 public:
  M5Sender(const InitParams& init_params)
    : OptionalMessageSender<Request::M5>(init_params) {
  }
  bool HandleReply(Reply* reply) override {
    return (reply->response_code() == rtsp::STATUS_OK);
  }
};

class M13Handler final : public MessageReceiver<Request::M13> {
 public:
  M13Handler(const InitParams& init_params)
    : MessageReceiver<Request::M13>(init_params) {
  }

  std::unique_ptr<Reply> HandleMessage(
      Message* message) override {
    ToSourceMediaManager(manager_)->SendIDRPicture();
    return std::unique_ptr<Reply>(new Reply(rtsp::STATUS_OK));
  }
};

StreamingState::StreamingState(const InitParams& init_params,
    MessageHandlerPtr m16_sender)
  : MessageSequenceWithOptionalSetHandler(init_params) {
  AddSequencedHandler(make_ptr(new M8Handler(init_params)));

  AddOptionalHandler(make_ptr(new M5Sender(init_params)));
  AddOptionalHandler(make_ptr(new M7Handler(init_params)));
  AddOptionalHandler(make_ptr(new M9Handler(init_params)));
  AddOptionalHandler(make_ptr(new M13Handler(init_params)));
  AddOptionalHandler(m16_sender);
}

StreamingState::~StreamingState() {
}

}  // source
}  // wds
