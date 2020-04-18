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

#include "libwds/source/init_state.h"

#include "libwds/rtsp/options.h"
#include "libwds/rtsp/reply.h"

namespace wds {

using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;

namespace source {

class M1Handler final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;
 private:
  std::unique_ptr<Message> CreateMessage() override {
    rtsp::Options* options = new rtsp::Options("*");
    options->header().set_cseq(sender_->GetNextCSeq());
    options->header().set_require_wfd_support(true);
    return std::unique_ptr<Message>(options);
  }

  bool HandleReply(Reply* reply) override {
    return (reply->response_code() == rtsp::STATUS_OK);
  }

};

class M2Handler final : public MessageReceiver<Request::M2> {
 public:
  M2Handler(const InitParams& init_params)
    : MessageReceiver<Request::M2>(init_params) {
  }
  std::unique_ptr<Reply> HandleMessage(
      Message* message) override {
    auto reply = std::unique_ptr<Reply>(new Reply(rtsp::STATUS_OK));
    std::vector<rtsp::Method> supported_methods;
    supported_methods.push_back(rtsp::ORG_WFA_WFD_1_0);
    supported_methods.push_back(rtsp::GET_PARAMETER);
    supported_methods.push_back(rtsp::SET_PARAMETER);
    supported_methods.push_back(rtsp::PLAY);
    supported_methods.push_back(rtsp::PAUSE);
    supported_methods.push_back(rtsp::SETUP);
    supported_methods.push_back(rtsp::TEARDOWN);
    reply->header().set_supported_methods(supported_methods);
    return reply;
  }
};

InitState::InitState(const InitParams& init_params)
  : MessageSequenceHandler(init_params) {
  AddSequencedHandler(make_ptr(new M1Handler(init_params)));
  AddSequencedHandler(make_ptr(new M2Handler(init_params)));
}

InitState::~InitState() {
}

}  // source
}  // wds
