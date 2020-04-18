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


#include "libwds/rtsp/clientrtpports.h"

namespace wds {
namespace rtsp {

namespace {
const char profile[] = "RTP/AVP/UDP;unicast";
const char mode[] = "mode=play";
}

ClientRtpPorts::ClientRtpPorts(unsigned short rtp_port_0,
    unsigned short rtp_port_1)
  : Property(ClientRTPPortsPropertyType),
    rtp_port_0_(rtp_port_0),
    rtp_port_1_(rtp_port_1) {
}

ClientRtpPorts::~ClientRtpPorts() {
}

std::string ClientRtpPorts::ToString() const {
  return PropertyName::wfd_client_rtp_ports + std::string(SEMICOLON)
    + std::string(SPACE) + profile
    + std::string(SPACE) + std::to_string(rtp_port_0_)
    + std::string(SPACE) + std::to_string(rtp_port_1_)
    + std::string(SPACE) + mode;
}

}  // namespace rtsp
}  // namespace wds
