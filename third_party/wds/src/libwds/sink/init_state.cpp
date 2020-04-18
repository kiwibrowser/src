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

#include "libwds/sink/init_state.h"

#include "libwds/rtsp/options.h"
#include "libwds/rtsp/reply.h"

namespace wds {
namespace sink {
using rtsp::Message;
using rtsp::Request;
using rtsp::Reply;

class M1Handler final : public MessageReceiver<Request::M1> {
 public:
  M1Handler(const InitParams& init_params, int& source_init_cseq)
    : MessageReceiver<Request::M1>(init_params),
      source_init_cseq_(source_init_cseq) {
  }
  virtual std::unique_ptr<Reply> HandleMessage(Message* message) override {
    source_init_cseq_ = message->cseq();
    auto reply = std::unique_ptr<Reply>(new Reply(rtsp::STATUS_OK));
    std::vector<rtsp::Method> supported_methods;
    supported_methods.push_back(rtsp::ORG_WFA_WFD_1_0);
    supported_methods.push_back(rtsp::GET_PARAMETER);
    supported_methods.push_back(rtsp::SET_PARAMETER);
    reply->header().set_supported_methods(supported_methods);
    return reply;
  }

 private:
  int& source_init_cseq_;
};

class M2Handler final : public SequencedMessageSender {
 public:
  M2Handler(const InitParams& init_params, int& source_init_cseq)
    : SequencedMessageSender(init_params),
      source_init_cseq_(source_init_cseq) {
  }
 private:
  virtual std::unique_ptr<Message> CreateMessage() override {
    auto options = new rtsp::Options("*");
    options->header().set_cseq(sender_->GetNextCSeq(&source_init_cseq_));
    options->header().set_require_wfd_support(true);
    return std::unique_ptr<Message>(options);
  }

  virtual bool HandleReply(Reply* reply) override {
    const rtsp::Header& header = reply->header();

    if (reply->response_code() == rtsp::STATUS_OK
        && header.has_method(rtsp::Method::ORG_WFA_WFD_1_0)
        && header.has_method(rtsp::Method::GET_PARAMETER)
        && header.has_method(rtsp::Method::SET_PARAMETER)
        && header.has_method(rtsp::Method::SETUP)
        && header.has_method(rtsp::Method::PLAY)
        && header.has_method(rtsp::Method::TEARDOWN)
        && header.has_method(rtsp::Method::PAUSE)) {
      return true;
    }

    return false;
  }
  int& source_init_cseq_;
};

InitState::InitState(const InitParams& init_params)
  : MessageSequenceHandler(init_params),
    source_init_cseq_(0) {
  AddSequencedHandler(make_ptr(new M1Handler(init_params, source_init_cseq_)));
  AddSequencedHandler(make_ptr(new M2Handler(init_params, source_init_cseq_)));
}

}  // sink
}  // wds
