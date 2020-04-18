// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/p2p/socket_host_test_utils.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/sys_byteorder.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_address.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

const int kStunHeaderSize = 20;
const uint16_t kStunBindingRequest = 0x0001;
const uint16_t kStunBindingResponse = 0x0102;
const uint16_t kStunBindingError = 0x0111;
const uint32_t kStunMagicCookie = 0x2112A442;

MockIPCSender::MockIPCSender() { }
MockIPCSender::~MockIPCSender() { }

FakeSocket::FakeSocket(std::string* written_data)
    : read_pending_(false),
      input_pos_(0),
      written_data_(written_data),
      async_write_(false),
      write_pending_(false) {
}

FakeSocket::~FakeSocket() { }

void FakeSocket::AppendInputData(const char* data, int data_size) {
  input_data_.insert(input_data_.end(), data, data + data_size);
  // Complete pending read if any.
  if (read_pending_) {
    read_pending_ = false;
    int result = std::min(read_buffer_size_,
                          static_cast<int>(input_data_.size() - input_pos_));
    CHECK(result > 0);
    memcpy(read_buffer_->data(), &input_data_[0] + input_pos_, result);
    input_pos_ += result;
    read_buffer_ = nullptr;
    std::move(read_callback_).Run(result);
  }
}

void FakeSocket::SetPeerAddress(const net::IPEndPoint& peer_address) {
  peer_address_ = peer_address;
}

void FakeSocket::SetLocalAddress(const net::IPEndPoint& local_address) {
  local_address_ = local_address;
}

int FakeSocket::Read(net::IOBuffer* buf,
                     int buf_len,
                     net::CompletionOnceCallback callback) {
  DCHECK(buf);
  if (input_pos_ < static_cast<int>(input_data_.size())){
    int result = std::min(buf_len,
                          static_cast<int>(input_data_.size()) - input_pos_);
    memcpy(buf->data(), &(*input_data_.begin()) + input_pos_, result);
    input_pos_ += result;
    return result;
  } else {
    read_pending_ = true;
    read_buffer_ = buf;
    read_buffer_size_ = buf_len;
    read_callback_ = std::move(callback);
    return net::ERR_IO_PENDING;
  }
}

int FakeSocket::Write(
    net::IOBuffer* buf,
    int buf_len,
    net::CompletionOnceCallback callback,
    const net::NetworkTrafficAnnotationTag& /*traffic_annotation*/) {
  DCHECK(buf);
  DCHECK(!write_pending_);

  if (async_write_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FakeSocket::DoAsyncWrite, base::Unretained(this),
                       scoped_refptr<net::IOBuffer>(buf), buf_len,
                       std::move(callback)));
    write_pending_ = true;
    return net::ERR_IO_PENDING;
  }

  if (written_data_) {
    written_data_->insert(written_data_->end(),
                          buf->data(), buf->data() + buf_len);
  }
  return buf_len;
}

void FakeSocket::DoAsyncWrite(scoped_refptr<net::IOBuffer> buf,
                              int buf_len,
                              net::CompletionOnceCallback callback) {
  write_pending_ = false;

  if (written_data_) {
    written_data_->insert(written_data_->end(),
                          buf->data(), buf->data() + buf_len);
  }
  std::move(callback).Run(buf_len);
}

int FakeSocket::SetReceiveBufferSize(int32_t size) {
  NOTIMPLEMENTED();
  return net::ERR_NOT_IMPLEMENTED;
}

int FakeSocket::SetSendBufferSize(int32_t size) {
  NOTIMPLEMENTED();
  return net::ERR_NOT_IMPLEMENTED;
}

int FakeSocket::Connect(net::CompletionOnceCallback callback) {
  return 0;
}

void FakeSocket::Disconnect() {
  NOTREACHED();
}

bool FakeSocket::IsConnected() const {
  return true;
}

bool FakeSocket::IsConnectedAndIdle() const {
  return false;
}

int FakeSocket::GetPeerAddress(net::IPEndPoint* address) const {
  *address = peer_address_;
  return net::OK;
}

int FakeSocket::GetLocalAddress(net::IPEndPoint* address) const {
  *address = local_address_;
  return net::OK;
}

const net::NetLogWithSource& FakeSocket::NetLog() const {
  NOTREACHED();
  return net_log_;
}

bool FakeSocket::WasEverUsed() const {
  return true;
}

bool FakeSocket::WasAlpnNegotiated() const {
  return false;
}

net::NextProto FakeSocket::GetNegotiatedProtocol() const {
  return net::kProtoUnknown;
}

bool FakeSocket::GetSSLInfo(net::SSLInfo* ssl_info) {
  return false;
}

void FakeSocket::GetConnectionAttempts(net::ConnectionAttempts* out) const {
  out->clear();
}

int64_t FakeSocket::GetTotalReceivedBytes() const {
  NOTIMPLEMENTED();
  return 0;
}

void CreateRandomPacket(std::vector<char>* packet) {
  size_t size = kStunHeaderSize + rand() % 1000;
  packet->resize(size);
  for (size_t i = 0; i < size; i++) {
    (*packet)[i] = rand() % 256;
  }
  // Always set the first bit to ensure that generated packet is not
  // valid STUN packet.
  (*packet)[0] = (*packet)[0] | 0x80;
}

static void CreateStunPacket(std::vector<char>* packet, uint16_t type) {
  CreateRandomPacket(packet);
  *reinterpret_cast<uint16_t*>(&*packet->begin()) = base::HostToNet16(type);
  *reinterpret_cast<uint16_t*>(&*packet->begin() + 2) =
      base::HostToNet16(packet->size() - kStunHeaderSize);
  *reinterpret_cast<uint32_t*>(&*packet->begin() + 4) =
      base::HostToNet32(kStunMagicCookie);
}

void CreateStunRequest(std::vector<char>* packet) {
  CreateStunPacket(packet, kStunBindingRequest);
}

void CreateStunResponse(std::vector<char>* packet) {
  CreateStunPacket(packet, kStunBindingResponse);
}

void CreateStunError(std::vector<char>* packet) {
  CreateStunPacket(packet, kStunBindingError);
}

net::IPEndPoint ParseAddress(const std::string& ip_str, uint16_t port) {
  net::IPAddress ip;
  EXPECT_TRUE(ip.AssignFromIPLiteral(ip_str));
  return net::IPEndPoint(ip, port);
}
