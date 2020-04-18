// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/p2p/socket_host_tcp.h"

#include <stddef.h>
#include <utility>

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/sys_byteorder.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/common/p2p_messages.h"
#include "ipc/ipc_sender.h"
#include "jingle/glue/fake_ssl_client_socket.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_client_socket.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/proxy_resolving_client_socket.h"
#include "services/network/proxy_resolving_client_socket_factory.h"
#include "third_party/webrtc/media/base/rtputils.h"
#include "url/gurl.h"

namespace {

typedef uint16_t PacketLength;
const int kPacketHeaderSize = sizeof(PacketLength);
const int kTcpReadBufferSize = 4096;
const int kPacketLengthOffset = 2;
const int kTurnChannelDataHeaderSize = 4;
const int kTcpRecvSocketBufferSize = 128 * 1024;
const int kTcpSendSocketBufferSize = 128 * 1024;

bool IsTlsClientSocket(content::P2PSocketType type) {
  return (type == content::P2P_SOCKET_STUN_TLS_CLIENT ||
          type == content::P2P_SOCKET_TLS_CLIENT);
}

bool IsPseudoTlsClientSocket(content::P2PSocketType type) {
  return (type == content::P2P_SOCKET_SSLTCP_CLIENT ||
          type == content::P2P_SOCKET_STUN_SSLTCP_CLIENT);
}

}  // namespace

namespace content {

P2PSocketHostTcp::SendBuffer::SendBuffer() : rtc_packet_id(-1) {}
P2PSocketHostTcp::SendBuffer::SendBuffer(
    int32_t rtc_packet_id,
    scoped_refptr<net::DrainableIOBuffer> buffer,
    const net::NetworkTrafficAnnotationTag traffic_annotation)
    : rtc_packet_id(rtc_packet_id),
      buffer(buffer),
      traffic_annotation(traffic_annotation) {}
P2PSocketHostTcp::SendBuffer::SendBuffer(const SendBuffer& rhs) = default;
P2PSocketHostTcp::SendBuffer::~SendBuffer() {}

P2PSocketHostTcpBase::P2PSocketHostTcpBase(
    IPC::Sender* message_sender,
    int socket_id,
    P2PSocketType type,
    net::URLRequestContextGetter* url_context,
    network::ProxyResolvingClientSocketFactory* proxy_resolving_socket_factory)
    : P2PSocketHost(message_sender, socket_id, P2PSocketHost::TCP),
      write_pending_(false),
      connected_(false),
      type_(type),
      url_context_(url_context),
      proxy_resolving_socket_factory_(proxy_resolving_socket_factory) {}

P2PSocketHostTcpBase::~P2PSocketHostTcpBase() {
  if (state_ == STATE_OPEN) {
    DCHECK(socket_.get());
    socket_.reset();
  }
}

bool P2PSocketHostTcpBase::InitAccepted(
    const net::IPEndPoint& remote_address,
    std::unique_ptr<net::StreamSocket> socket) {
  DCHECK(socket);
  DCHECK_EQ(state_, STATE_UNINITIALIZED);

  remote_address_.ip_address = remote_address;
  // TODO(ronghuawu): Add FakeSSLServerSocket.
  socket_ = std::move(socket);
  state_ = STATE_OPEN;
  DoRead();
  return state_ != STATE_ERROR;
}

bool P2PSocketHostTcpBase::Init(const net::IPEndPoint& local_address,
                                uint16_t min_port,
                                uint16_t max_port,
                                const P2PHostAndIPEndPoint& remote_address) {
  DCHECK_EQ(state_, STATE_UNINITIALIZED);

  remote_address_ = remote_address;
  state_ = STATE_CONNECTING;

  net::HostPortPair dest_host_port_pair;
  // If there is a domain name, let's try it first, it's required by some proxy
  // to only take hostname for CONNECT. If it has been DNS resolved, the result
  // is likely cached and shouldn't cause 2nd DNS resolution in the case of
  // direct connect (i.e. no proxy).
  if (!remote_address.hostname.empty()) {
    dest_host_port_pair = net::HostPortPair(remote_address.hostname,
                                            remote_address.ip_address.port());
  } else {
    DCHECK(!remote_address.ip_address.address().empty());
    dest_host_port_pair = net::HostPortPair::FromIPEndPoint(
        remote_address.ip_address);
  }

  // TODO(mallinath) - We are ignoring local_address altogether. We should
  // find a way to inject this into ProxyResolvingClientSocket. This could be
  // a problem on multi-homed host.

  // The default SSLConfig is good enough for us for now.
  const net::SSLConfig ssl_config;
  socket_ = proxy_resolving_socket_factory_->CreateSocket(
      ssl_config, GURL("https://" + dest_host_port_pair.ToString()),
      IsTlsClientSocket(type_));

  if (IsPseudoTlsClientSocket(type_)) {
    socket_ =
        std::make_unique<jingle_glue::FakeSSLClientSocket>(std::move(socket_));
  }

  int status = socket_->Connect(base::BindOnce(
      &P2PSocketHostTcpBase::OnConnected, base::Unretained(this)));
  if (status != net::ERR_IO_PENDING) {
    // We defer execution of ProcessConnectDone instead of calling it
    // directly here as the caller may not expect an error/close to
    // happen here.  This is okay, as from the caller's point of view,
    // the connect always happens asynchronously.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&P2PSocketHostTcpBase::OnConnected,
                                  base::Unretained(this), status));
  }

  return state_ != STATE_ERROR;
}

void P2PSocketHostTcpBase::OnError() {
  socket_.reset();

  if (state_ == STATE_UNINITIALIZED || state_ == STATE_CONNECTING ||
      state_ == STATE_OPEN) {
    message_sender_->Send(new P2PMsg_OnError(id_));
  }

  state_ = STATE_ERROR;
}

void P2PSocketHostTcpBase::OnConnected(int result) {
  DCHECK_EQ(state_, STATE_CONNECTING);
  DCHECK_NE(result, net::ERR_IO_PENDING);

  if (result != net::OK) {
    LOG(WARNING) << "Error from connecting socket, result=" << result;
    OnError();
    return;
  }

  OnOpen();
}

void P2PSocketHostTcpBase::OnOpen() {
  state_ = STATE_OPEN;
  // Setting socket send and receive buffer size.
  if (net::OK != socket_->SetReceiveBufferSize(kTcpRecvSocketBufferSize)) {
    LOG(WARNING) << "Failed to set socket receive buffer size to "
                 << kTcpRecvSocketBufferSize;
  }

  if (net::OK != socket_->SetSendBufferSize(kTcpSendSocketBufferSize)) {
    LOG(WARNING) << "Failed to set socket send buffer size to "
                 << kTcpSendSocketBufferSize;
  }

  if (!DoSendSocketCreateMsg())
    return;

  DCHECK_EQ(state_, STATE_OPEN);
  DoRead();
}

bool P2PSocketHostTcpBase::DoSendSocketCreateMsg() {
  DCHECK(socket_.get());

  net::IPEndPoint local_address;
  int result = socket_->GetLocalAddress(&local_address);
  if (result < 0) {
    LOG(ERROR) << "P2PSocketHostTcpBase::OnConnected: unable to get local"
               << " address: " << result;
    OnError();
    return false;
  }

  VLOG(1) << "Local address: " << local_address.ToString();

  net::IPEndPoint remote_address;

  // GetPeerAddress returns ERR_NAME_NOT_RESOLVED if the socket is connected
  // through a proxy.
  result = socket_->GetPeerAddress(&remote_address);
  if (result < 0 && result != net::ERR_NAME_NOT_RESOLVED) {
    LOG(ERROR) << "P2PSocketHostTcpBase::OnConnected: unable to get peer"
               << " address: " << result;
    OnError();
    return false;
  }

  if (!remote_address.address().empty()) {
    VLOG(1) << "Remote address: " << remote_address.ToString();
    if (remote_address_.ip_address.address().empty()) {
      // Save |remote_address| if address is empty.
      remote_address_.ip_address = remote_address;
    }
  } else {
    VLOG(1) << "Remote address is unknown since connection is proxied";
  }

  // If we are not doing TLS, we are ready to send data now.
  // In case of TLS SignalConnect will be sent only after TLS handshake is
  // successful. So no buffering will be done at socket handlers if any
  // packets sent before that by the application.
  message_sender_->Send(new P2PMsg_OnSocketCreated(
      id_, local_address, remote_address));
  return true;
}

void P2PSocketHostTcpBase::DoRead() {
  int result;
  do {
    if (!read_buffer_.get()) {
      read_buffer_ = new net::GrowableIOBuffer();
      read_buffer_->SetCapacity(kTcpReadBufferSize);
    } else if (read_buffer_->RemainingCapacity() < kTcpReadBufferSize) {
      // Make sure that we always have at least kTcpReadBufferSize of
      // remaining capacity in the read buffer. Normally all packets
      // are smaller than kTcpReadBufferSize, so this is not really
      // required.
      read_buffer_->SetCapacity(read_buffer_->capacity() + kTcpReadBufferSize -
                                read_buffer_->RemainingCapacity());
    }
    result = socket_->Read(
        read_buffer_.get(),
        read_buffer_->RemainingCapacity(),
        base::Bind(&P2PSocketHostTcp::OnRead, base::Unretained(this)));
    DidCompleteRead(result);
  } while (result > 0);
}

void P2PSocketHostTcpBase::OnRead(int result) {
  DidCompleteRead(result);
  if (state_ == STATE_OPEN) {
    DoRead();
  }
}

void P2PSocketHostTcpBase::OnPacket(const std::vector<char>& data) {
  if (!connected_) {
    P2PSocketHost::StunMessageType type;
    bool stun = GetStunPacketType(&*data.begin(), data.size(), &type);
    if (stun && IsRequestOrResponse(type)) {
      connected_ = true;
    } else if (!stun || type == STUN_DATA_INDICATION) {
      LOG(ERROR) << "Received unexpected data packet from "
                 << remote_address_.ip_address.ToString()
                 << " before STUN binding is finished. "
                 << "Terminating connection.";
      OnError();
      return;
    }
  }

  message_sender_->Send(new P2PMsg_OnDataReceived(
      id_, remote_address_.ip_address, data, base::TimeTicks::Now()));

  if (dump_incoming_rtp_packet_)
    DumpRtpPacket(&data[0], data.size(), true);
}

// Note: dscp is not actually used on TCP sockets as this point,
// but may be honored in the future.
void P2PSocketHostTcpBase::Send(
    const net::IPEndPoint& to,
    const std::vector<char>& data,
    const rtc::PacketOptions& options,
    uint64_t packet_id,
    const net::NetworkTrafficAnnotationTag traffic_annotation) {
  if (!socket_) {
    // The Send message may be sent after the an OnError message was
    // sent by hasn't been processed the renderer.
    return;
  }

  if (!(to == remote_address_.ip_address)) {
    // Renderer should use this socket only to send data to |remote_address_|.
    NOTREACHED();
    OnError();
    return;
  }

  if (!connected_) {
    P2PSocketHost::StunMessageType type = P2PSocketHost::StunMessageType();
    bool stun = GetStunPacketType(&*data.begin(), data.size(), &type);
    if (!stun || type == STUN_DATA_INDICATION) {
      LOG(ERROR) << "Page tried to send a data packet to " << to.ToString()
                 << " before STUN binding is finished.";
      OnError();
      return;
    }
  }

  DoSend(to, data, options, traffic_annotation);
}

void P2PSocketHostTcpBase::WriteOrQueue(SendBuffer& send_buffer) {
  IncrementTotalSentPackets();
  if (write_buffer_.buffer.get()) {
    write_queue_.push(send_buffer);
    IncrementDelayedPackets();
    IncrementDelayedBytes(send_buffer.buffer->size());
    return;
  }

  write_buffer_ = send_buffer;
  DoWrite();
}

void P2PSocketHostTcpBase::DoWrite() {
  while (write_buffer_.buffer.get() && state_ == STATE_OPEN &&
         !write_pending_) {
    int result = socket_->Write(
        write_buffer_.buffer.get(), write_buffer_.buffer->BytesRemaining(),
        base::Bind(&P2PSocketHostTcp::OnWritten, base::Unretained(this)),
        net::NetworkTrafficAnnotationTag(write_buffer_.traffic_annotation));
    HandleWriteResult(result);
  }
}

void P2PSocketHostTcpBase::OnWritten(int result) {
  DCHECK(write_pending_);
  DCHECK_NE(result, net::ERR_IO_PENDING);

  write_pending_ = false;
  HandleWriteResult(result);
  DoWrite();
}

void P2PSocketHostTcpBase::HandleWriteResult(int result) {
  DCHECK(write_buffer_.buffer.get());
  if (result >= 0) {
    write_buffer_.buffer->DidConsume(result);
    if (write_buffer_.buffer->BytesRemaining() == 0) {
      base::TimeTicks send_time = base::TimeTicks::Now();
      message_sender_->Send(new P2PMsg_OnSendComplete(
          id_,
          P2PSendPacketMetrics(0, write_buffer_.rtc_packet_id, send_time)));
      if (write_queue_.empty()) {
        write_buffer_.buffer = nullptr;
        write_buffer_.rtc_packet_id = -1;
      } else {
        write_buffer_ = write_queue_.front();
        write_queue_.pop();
        // Update how many bytes are still waiting to be sent.
        DecrementDelayedBytes(write_buffer_.buffer->size());
      }
    }
  } else if (result == net::ERR_IO_PENDING) {
    write_pending_ = true;
  } else {
    ReportSocketError(result, "WebRTC.ICE.TcpSocketWriteErrorCode");

    LOG(ERROR) << "Error when sending data in TCP socket: " << result;
    OnError();
  }
}

std::unique_ptr<P2PSocketHost>
P2PSocketHostTcpBase::AcceptIncomingTcpConnection(
    const net::IPEndPoint& remote_address,
    int id) {
  NOTREACHED();
  OnError();
  return nullptr;
}

void P2PSocketHostTcpBase::DidCompleteRead(int result) {
  DCHECK_EQ(state_, STATE_OPEN);

  if (result == net::ERR_IO_PENDING) {
    return;
  } else if (result < 0) {
    LOG(ERROR) << "Error when reading from TCP socket: " << result;
    OnError();
    return;
  } else if (result == 0) {
    LOG(WARNING) << "Remote peer has shutdown TCP socket.";
    OnError();
    return;
  }

  read_buffer_->set_offset(read_buffer_->offset() + result);
  char* head = read_buffer_->StartOfBuffer();  // Purely a convenience.
  int pos = 0;
  while (pos <= read_buffer_->offset() && state_ == STATE_OPEN) {
    int consumed = ProcessInput(head + pos, read_buffer_->offset() - pos);
    if (!consumed)
      break;
    pos += consumed;
  }
  // We've consumed all complete packets from the buffer; now move any remaining
  // bytes to the head of the buffer and set offset to reflect this.
  if (pos && pos <= read_buffer_->offset()) {
    memmove(head, head + pos, read_buffer_->offset() - pos);
    read_buffer_->set_offset(read_buffer_->offset() - pos);
  }
}

bool P2PSocketHostTcpBase::SetOption(P2PSocketOption option, int value) {
  if (state_ != STATE_OPEN) {
    DCHECK_EQ(state_, STATE_ERROR);
    return false;
  }

  switch (option) {
    case P2P_SOCKET_OPT_RCVBUF:
      return socket_->SetReceiveBufferSize(value) == net::OK;
    case P2P_SOCKET_OPT_SNDBUF:
      return socket_->SetSendBufferSize(value) == net::OK;
    case P2P_SOCKET_OPT_DSCP:
      return false;  // For TCP sockets DSCP setting is not available.
    default:
      NOTREACHED();
      return false;
  }
}

P2PSocketHostTcp::P2PSocketHostTcp(
    IPC::Sender* message_sender,
    int socket_id,
    P2PSocketType type,
    net::URLRequestContextGetter* url_context,
    network::ProxyResolvingClientSocketFactory* proxy_resolving_socket_factory)
    : P2PSocketHostTcpBase(message_sender,
                           socket_id,
                           type,
                           url_context,
                           proxy_resolving_socket_factory) {
  DCHECK(type == P2P_SOCKET_TCP_CLIENT ||
         type == P2P_SOCKET_SSLTCP_CLIENT ||
         type == P2P_SOCKET_TLS_CLIENT);
}

P2PSocketHostTcp::~P2PSocketHostTcp() {
}

int P2PSocketHostTcp::ProcessInput(char* input, int input_len) {
  if (input_len < kPacketHeaderSize)
    return 0;
  int packet_size = base::NetToHost16(*reinterpret_cast<uint16_t*>(input));
  if (input_len < packet_size + kPacketHeaderSize)
    return 0;

  int consumed = kPacketHeaderSize;
  char* cur = input + consumed;
  std::vector<char> data(cur, cur + packet_size);
  OnPacket(data);
  consumed += packet_size;
  return consumed;
}

void P2PSocketHostTcp::DoSend(
    const net::IPEndPoint& to,
    const std::vector<char>& data,
    const rtc::PacketOptions& options,
    const net::NetworkTrafficAnnotationTag traffic_annotation) {
  int size = kPacketHeaderSize + data.size();
  SendBuffer send_buffer(
      options.packet_id,
      new net::DrainableIOBuffer(new net::IOBuffer(size), size),
      traffic_annotation);
  *reinterpret_cast<uint16_t*>(send_buffer.buffer->data()) =
      base::HostToNet16(data.size());
  memcpy(send_buffer.buffer->data() + kPacketHeaderSize, &data[0], data.size());

  cricket::ApplyPacketOptions(
      reinterpret_cast<uint8_t*>(send_buffer.buffer->data()) +
          kPacketHeaderSize,
      send_buffer.buffer->BytesRemaining() - kPacketHeaderSize,
      options.packet_time_params,
      (base::TimeTicks::Now() - base::TimeTicks()).InMicroseconds());

  WriteOrQueue(send_buffer);
}

// P2PSocketHostStunTcp
P2PSocketHostStunTcp::P2PSocketHostStunTcp(
    IPC::Sender* message_sender,
    int socket_id,
    P2PSocketType type,
    net::URLRequestContextGetter* url_context,
    network::ProxyResolvingClientSocketFactory* proxy_resolving_socket_factory)
    : P2PSocketHostTcpBase(message_sender,
                           socket_id,
                           type,
                           url_context,
                           proxy_resolving_socket_factory) {
  DCHECK(type == P2P_SOCKET_STUN_TCP_CLIENT ||
         type == P2P_SOCKET_STUN_SSLTCP_CLIENT ||
         type == P2P_SOCKET_STUN_TLS_CLIENT);
}

P2PSocketHostStunTcp::~P2PSocketHostStunTcp() {
}

int P2PSocketHostStunTcp::ProcessInput(char* input, int input_len) {
  if (input_len < kPacketHeaderSize + kPacketLengthOffset)
    return 0;

  int pad_bytes;
  int packet_size = GetExpectedPacketSize(
      input, input_len, &pad_bytes);

  if (input_len < packet_size + pad_bytes)
    return 0;

  // We have a complete packet. Read through it.
  int consumed = 0;
  char* cur = input;
  std::vector<char> data(cur, cur + packet_size);
  OnPacket(data);
  consumed += packet_size;
  consumed += pad_bytes;
  return consumed;
}

void P2PSocketHostStunTcp::DoSend(
    const net::IPEndPoint& to,
    const std::vector<char>& data,
    const rtc::PacketOptions& options,
    const net::NetworkTrafficAnnotationTag traffic_annotation) {
  // Each packet is expected to have header (STUN/TURN ChannelData), where
  // header contains message type and and length of message.
  if (data.size() < kPacketHeaderSize + kPacketLengthOffset) {
    NOTREACHED();
    OnError();
    return;
  }

  int pad_bytes;
  size_t expected_len = GetExpectedPacketSize(
      &data[0], data.size(), &pad_bytes);

  // Accepts only complete STUN/TURN packets.
  if (data.size() != expected_len) {
    NOTREACHED();
    OnError();
    return;
  }

  // Add any pad bytes to the total size.
  int size = data.size() + pad_bytes;

  SendBuffer send_buffer(
      options.packet_id,
      new net::DrainableIOBuffer(new net::IOBuffer(size), size),
      traffic_annotation);
  memcpy(send_buffer.buffer->data(), &data[0], data.size());

  cricket::ApplyPacketOptions(
      reinterpret_cast<uint8_t*>(send_buffer.buffer->data()), data.size(),
      options.packet_time_params,
      (base::TimeTicks::Now() - base::TimeTicks()).InMicroseconds());

  if (pad_bytes) {
    char padding[4] = {0};
    DCHECK_LE(pad_bytes, 4);
    memcpy(send_buffer.buffer->data() + data.size(), padding, pad_bytes);
  }
  WriteOrQueue(send_buffer);

  if (dump_outgoing_rtp_packet_)
    DumpRtpPacket(send_buffer.buffer->data(), data.size(), false);
}

int P2PSocketHostStunTcp::GetExpectedPacketSize(
    const char* data, int len, int* pad_bytes) {
  DCHECK_LE(kTurnChannelDataHeaderSize, len);
  // Both stun and turn had length at offset 2.
  int packet_size = base::NetToHost16(
      *reinterpret_cast<const uint16_t*>(data + kPacketLengthOffset));

  // Get packet type (STUN or TURN).
  uint16_t msg_type =
      base::NetToHost16(*reinterpret_cast<const uint16_t*>(data));

  *pad_bytes = 0;
  // Add heder length to packet length.
  if ((msg_type & 0xC000) == 0) {
    packet_size += kStunHeaderSize;
  } else {
    packet_size += kTurnChannelDataHeaderSize;
    // Calculate any padding if present.
    if (packet_size % 4)
      *pad_bytes = 4 - packet_size % 4;
  }
  return packet_size;
}

}  // namespace content
