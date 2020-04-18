// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_P2P_MESSAGES_H_
#define CONTENT_COMMON_P2P_MESSAGES_H_

// IPC messages for the P2P Transport API.

#include <stdint.h>

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/p2p_socket_type.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/ip_address.h"
#include "net/base/network_interfaces.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/webrtc/rtc_base/asyncpacketsocket.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START P2PMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(content::P2PSocketType,
                          content::P2P_SOCKET_TYPE_LAST)
IPC_ENUM_TRAITS_MAX_VALUE(content::P2PSocketOption,
                          content::P2P_SOCKET_OPT_MAX - 1)
IPC_ENUM_TRAITS_MIN_MAX_VALUE(rtc::DiffServCodePoint,
                              rtc::DSCP_NO_CHANGE,
                              rtc::DSCP_CS7)

IPC_STRUCT_TRAITS_BEGIN(net::NetworkInterface)
  IPC_STRUCT_TRAITS_MEMBER(name)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(address)
  IPC_STRUCT_TRAITS_MEMBER(prefix_length)
  IPC_STRUCT_TRAITS_MEMBER(ip_address_attributes)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(rtc::PacketTimeUpdateParams)
  IPC_STRUCT_TRAITS_MEMBER(rtp_sendtime_extension_id)
  IPC_STRUCT_TRAITS_MEMBER(srtp_auth_key)
  IPC_STRUCT_TRAITS_MEMBER(srtp_auth_tag_len)
  IPC_STRUCT_TRAITS_MEMBER(srtp_packet_index)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(rtc::PacketOptions)
  IPC_STRUCT_TRAITS_MEMBER(dscp)
  IPC_STRUCT_TRAITS_MEMBER(packet_id)
  IPC_STRUCT_TRAITS_MEMBER(packet_time_params)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::P2PHostAndIPEndPoint)
  IPC_STRUCT_TRAITS_MEMBER(hostname)
  IPC_STRUCT_TRAITS_MEMBER(ip_address)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::P2PSendPacketMetrics)
  IPC_STRUCT_TRAITS_MEMBER(packet_id)
  IPC_STRUCT_TRAITS_MEMBER(rtc_packet_id)
  IPC_STRUCT_TRAITS_MEMBER(send_time)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::P2PPortRange)
  IPC_STRUCT_TRAITS_MEMBER(min_port)
  IPC_STRUCT_TRAITS_MEMBER(max_port)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::P2PPacketInfo)
  IPC_STRUCT_TRAITS_MEMBER(destination)
  IPC_STRUCT_TRAITS_MEMBER(packet_options)
  IPC_STRUCT_TRAITS_MEMBER(packet_id)
IPC_STRUCT_TRAITS_END()

// P2P Socket messages sent from the browser to the renderer.

IPC_MESSAGE_CONTROL3(P2PMsg_NetworkListChanged,
                     net::NetworkInterfaceList /* networks */,
                     net::IPAddress /* default_ipv4_local_address */,
                     net::IPAddress /* default_ipv6_local_address */)

IPC_MESSAGE_CONTROL2(P2PMsg_GetHostAddressResult,
                     int32_t /* request_id */,
                     net::IPAddressList /* address list*/)

IPC_MESSAGE_CONTROL3(P2PMsg_OnSocketCreated,
                     int /* socket_id */,
                     net::IPEndPoint /* local_address */,
                     net::IPEndPoint /* remote_address */)

// |send_metrics| carries packet_id for this packet.
IPC_MESSAGE_CONTROL2(P2PMsg_OnSendComplete,
                     int /* socket_id */,
                     content::P2PSendPacketMetrics /* send_metrics */)

IPC_MESSAGE_CONTROL1(P2PMsg_OnError,
                     int /* socket_id */)

IPC_MESSAGE_CONTROL2(P2PMsg_OnIncomingTcpConnection,
                     int /* socket_id */,
                     net::IPEndPoint /* socket_address */)

IPC_MESSAGE_CONTROL4(P2PMsg_OnDataReceived,
                     int /* socket_id */,
                     net::IPEndPoint /* socket_address */,
                     std::vector<char> /* data */,
                     base::TimeTicks /* timestamp */ )

// P2P Socket messages sent from the renderer to the browser.

// Start/stop sending P2PMsg_NetworkListChanged messages when network
// configuration changes.
IPC_MESSAGE_CONTROL0(P2PHostMsg_StartNetworkNotifications)
IPC_MESSAGE_CONTROL0(P2PHostMsg_StopNetworkNotifications)

IPC_MESSAGE_CONTROL2(P2PHostMsg_GetHostAddress,
                     std::string /* host_name */,
                     int32_t /* request_id */)

IPC_MESSAGE_CONTROL5(P2PHostMsg_CreateSocket,
                     content::P2PSocketType /* type */,
                     int /* socket_id */,
                     net::IPEndPoint /* local_address */,
                     content::P2PPortRange /* port_range */,
                     content::P2PHostAndIPEndPoint /* remote_address */)

IPC_MESSAGE_CONTROL3(P2PHostMsg_AcceptIncomingTcpConnection,
                    int /* listen_socket_id */,
                    net::IPEndPoint /* remote_address */,
                    int /* connected_socket_id */)

// TODO(sergeyu): Use shared memory to pass the data.
IPC_MESSAGE_CONTROL4(
    P2PHostMsg_Send,
    int /* socket_id */,
    std::vector<char> /* data */,
    content::P2PPacketInfo /* packet_info */,
    net::MutableNetworkTrafficAnnotationTag /* traffic_annotation */)

IPC_MESSAGE_CONTROL1(P2PHostMsg_DestroySocket,
                     int /* socket_id */)

IPC_MESSAGE_CONTROL3(P2PHostMsg_SetOption,
                     int /* socket_id */,
                     content::P2PSocketOption /* socket option type */,
                     int /* value */)

#endif  // CONTENT_COMMON_P2P_MESSAGES_H_
