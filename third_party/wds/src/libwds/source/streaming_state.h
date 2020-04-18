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

#ifndef LIBWDS_SOURCE_STREAMING_STATE_H_
#define LIBWDS_SOURCE_STREAMING_STATE_H_

#include "libwds/common/message_handler.h"

namespace wds {
namespace source {

// Streaming state for RTSP source.
// Includes M8 message handling and optionally can handle M3, M4, M7, M9-M15
class StreamingState : public MessageSequenceWithOptionalSetHandler {
 public:
  StreamingState(const InitParams& init_params, MessageHandlerPtr m16_sender);
  ~StreamingState() override;
};

}  // source
}  // wds

#endif // LIBWDS_SOURCE_STREAMING_STATE_H_
