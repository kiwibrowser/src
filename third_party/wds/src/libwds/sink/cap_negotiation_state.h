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

#ifndef LIBWDS_SINK_CAP_NEGOTIATION_STATE_H_
#define LIBWDS_SINK_CAP_NEGOTIATION_STATE_H_

#include "libwds/common/message_handler.h"

namespace wds {
namespace sink {

// Capability negotiation state for RTSP sink.
// Includes M3 and M4 messages handling
class CapNegotiationState : public MessageSequenceWithOptionalSetHandler {
 public:
  CapNegotiationState(const InitParams& init_params);
};

class M4Handler final : public MessageReceiver<rtsp::Request::M4> {
 public:
  M4Handler(const InitParams& init_params);
  std::unique_ptr<rtsp::Reply> HandleMessage(rtsp::Message* message) override;
};

class M3Handler final : public MessageReceiver<rtsp::Request::M3> {
 public:
  M3Handler(const InitParams& init_params);
  std::unique_ptr<rtsp::Reply> HandleMessage(rtsp::Message* message) override;
};

}  // namespace sink
}  // namespace wds

#endif // LIBWDS_SINK_CAP_NEGOTIATION_STATE_H_
