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


#include "libwds/rtsp/transportheader.h"
#include "libwds/rtsp/constants.h"

namespace wds {
namespace rtsp {

namespace {
  const char kTransport[] = "Transport: RTP/AVP/UDP;unicast;client_port=";
  const char kServerPort[] = ";server_port=";
}

TransportHeader::TransportHeader() :
  client_port_(0),
  server_port_(0),
  client_supports_rtcp_(false),
  server_supports_rtcp_(false) {

}

TransportHeader::~TransportHeader() {
}

unsigned int TransportHeader::client_port() const {
  return client_port_;
}

void TransportHeader::set_client_port(unsigned int client_port) {
  client_port_ = client_port;
}

unsigned int TransportHeader::server_port() const {
  return server_port_;
}

void TransportHeader::set_server_port(unsigned int server_port) {
  server_port_ = server_port;
}

bool TransportHeader::client_supports_rtcp() const {
  return client_supports_rtcp_;
}

void TransportHeader::set_client_supports_rtcp(bool client_supports_rtcp) {
  client_supports_rtcp_ = client_supports_rtcp;
}

bool TransportHeader::server_supports_rtcp() const {
  return server_supports_rtcp_;
}

void TransportHeader::set_server_supports_rtcp(bool server_supports_rtcp) {
  server_supports_rtcp_ = server_supports_rtcp;
}

std::string TransportHeader::ToString() const {
  std::string ret;

  if (client_port_ > 0) {
    ret += kTransport + std::to_string(client_port_);
    if (client_supports_rtcp_)
      ret += "-" + std::to_string(client_port_ + 1);

    if (server_port_ > 0) {
      ret += kServerPort + std::to_string(server_port_);
      if (server_supports_rtcp_)
        ret += "-" + std::to_string(server_port_ + 1);
    }

    ret += CRLF;
  }
  return ret;
}

}  // namespace rtsp
}  // namespace wds
