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


#ifndef LIBWDS_RTSP_TRANSPORT_HEADER_H_
#define LIBWDS_RTSP_TRANSPORT_HEADER_H_

#include <string>

namespace wds {
namespace rtsp {

class TransportHeader {
  public:
    TransportHeader();
    virtual ~TransportHeader();

    unsigned int client_port() const;
    void set_client_port(unsigned int client_port);

    unsigned int server_port() const;
    void set_server_port(unsigned int server_port);

    bool client_supports_rtcp() const;
    void set_client_supports_rtcp(bool client_supports_rtcp);

    bool server_supports_rtcp() const;
    void set_server_supports_rtcp(bool server_supports_rtcp);

    std::string ToString() const;

 private:
    unsigned int client_port_;
    unsigned int server_port_;
    bool client_supports_rtcp_;
    bool server_supports_rtcp_;
};

}  // namespace rtsp
}  // namespace wds

#endif // LIBWDS_RTSP_TRANSPORT_HEADER_H_
