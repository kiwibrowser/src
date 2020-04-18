// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/glue/chrome_async_socket.h"

#include <stddef.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "jingle/glue/resolving_client_socket_factory.h"
#include "net/base/address_list.h"
#include "net/base/host_port_pair.h"
#include "net/base/io_buffer.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_client_socket.h"
#include "net/ssl/ssl_config_service.h"
#include "third_party/webrtc/rtc_base/socketaddress.h"

namespace jingle_glue {

ChromeAsyncSocket::ChromeAsyncSocket(
    ResolvingClientSocketFactory* resolving_client_socket_factory,
    size_t read_buf_size,
    size_t write_buf_size,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : resolving_client_socket_factory_(resolving_client_socket_factory),
      state_(STATE_CLOSED),
      error_(ERROR_NONE),
      net_error_(net::OK),
      read_state_(IDLE),
      read_buf_(new net::IOBufferWithSize(read_buf_size)),
      read_start_(0U),
      read_end_(0U),
      write_state_(IDLE),
      write_buf_(new net::IOBufferWithSize(write_buf_size)),
      write_end_(0U),
      traffic_annotation_(traffic_annotation),
      weak_ptr_factory_(this) {
  DCHECK(resolving_client_socket_factory_.get());
  DCHECK_GT(read_buf_size, 0U);
  DCHECK_GT(write_buf_size, 0U);
}

ChromeAsyncSocket::~ChromeAsyncSocket() {}

ChromeAsyncSocket::State ChromeAsyncSocket::state() {
  return state_;
}

ChromeAsyncSocket::Error ChromeAsyncSocket::error() {
  return error_;
}

int ChromeAsyncSocket::GetError() {
  return net_error_;
}

bool ChromeAsyncSocket::IsOpen() const {
  return (state_ == STATE_OPEN) || (state_ == STATE_TLS_OPEN);
}

void ChromeAsyncSocket::DoNonNetError(Error error) {
  DCHECK_NE(error, ERROR_NONE);
  DCHECK_NE(error, ERROR_WINSOCK);
  error_ = error;
  net_error_ = net::OK;
}

void ChromeAsyncSocket::DoNetError(net::Error net_error) {
  error_ = ERROR_WINSOCK;
  net_error_ = net_error;
}

void ChromeAsyncSocket::DoNetErrorFromStatus(int status) {
  DCHECK_LT(status, net::OK);
  DoNetError(static_cast<net::Error>(status));
}

// STATE_CLOSED -> STATE_CONNECTING

bool ChromeAsyncSocket::Connect(const rtc::SocketAddress& address) {
  if (state_ != STATE_CLOSED) {
    LOG(DFATAL) << "Connect() called on non-closed socket";
    DoNonNetError(ERROR_WRONGSTATE);
    return false;
  }
  if (address.hostname().empty() || address.port() == 0) {
    DoNonNetError(ERROR_DNS);
    return false;
  }

  DCHECK_EQ(state_, buzz::AsyncSocket::STATE_CLOSED);
  DCHECK_EQ(read_state_, IDLE);
  DCHECK_EQ(write_state_, IDLE);

  state_ = STATE_CONNECTING;

  DCHECK(!weak_ptr_factory_.HasWeakPtrs());
  weak_ptr_factory_.InvalidateWeakPtrs();

  net::HostPortPair dest_host_port_pair(address.hostname(), address.port());

  transport_socket_ =
      resolving_client_socket_factory_->CreateTransportClientSocket(
          dest_host_port_pair);
  int status = transport_socket_->Connect(
      base::Bind(&ChromeAsyncSocket::ProcessConnectDone,
                 weak_ptr_factory_.GetWeakPtr()));
  if (status != net::ERR_IO_PENDING) {
    // We defer execution of ProcessConnectDone instead of calling it
    // directly here as the caller may not expect an error/close to
    // happen here.  This is okay, as from the caller's point of view,
    // the connect always happens asynchronously.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ChromeAsyncSocket::ProcessConnectDone,
                              weak_ptr_factory_.GetWeakPtr(), status));
  }
  return true;
}

// STATE_CONNECTING -> STATE_OPEN
// read_state_ == IDLE -> read_state_ == POSTED (via PostDoRead())

void ChromeAsyncSocket::ProcessConnectDone(int status) {
  DCHECK_NE(status, net::ERR_IO_PENDING);
  DCHECK_EQ(read_state_, IDLE);
  DCHECK_EQ(write_state_, IDLE);
  DCHECK_EQ(state_, STATE_CONNECTING);
  if (status != net::OK) {
    DoNetErrorFromStatus(status);
    DoClose();
    return;
  }
  state_ = STATE_OPEN;
  PostDoRead();
  // Write buffer should be empty.
  DCHECK_EQ(write_end_, 0U);
  SignalConnected();
}

// read_state_ == IDLE -> read_state_ == POSTED

void ChromeAsyncSocket::PostDoRead() {
  DCHECK(IsOpen());
  DCHECK_EQ(read_state_, IDLE);
  DCHECK_EQ(read_start_, 0U);
  DCHECK_EQ(read_end_, 0U);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&ChromeAsyncSocket::DoRead, weak_ptr_factory_.GetWeakPtr()));
  read_state_ = POSTED;
}

// read_state_ == POSTED -> read_state_ == PENDING

void ChromeAsyncSocket::DoRead() {
  DCHECK(IsOpen());
  DCHECK_EQ(read_state_, POSTED);
  DCHECK_EQ(read_start_, 0U);
  DCHECK_EQ(read_end_, 0U);
  // Once we call Read(), we cannot call StartTls() until the read
  // finishes.  This is okay, as StartTls() is called only from a read
  // handler (i.e., after a read finishes and before another read is
  // done).
  int status =
      transport_socket_->Read(
          read_buf_.get(), read_buf_->size(),
          base::Bind(&ChromeAsyncSocket::ProcessReadDone,
                     weak_ptr_factory_.GetWeakPtr()));
  read_state_ = PENDING;
  if (status != net::ERR_IO_PENDING) {
    ProcessReadDone(status);
  }
}

// read_state_ == PENDING -> read_state_ == IDLE

void ChromeAsyncSocket::ProcessReadDone(int status) {
  DCHECK_NE(status, net::ERR_IO_PENDING);
  DCHECK(IsOpen());
  DCHECK_EQ(read_state_, PENDING);
  DCHECK_EQ(read_start_, 0U);
  DCHECK_EQ(read_end_, 0U);
  read_state_ = IDLE;
  if (status > 0) {
    read_end_ = static_cast<size_t>(status);
    SignalRead();
  } else if (status == 0) {
    // Other side closed the connection.
    error_ = ERROR_NONE;
    net_error_ = net::OK;
    DoClose();
  } else {  // status < 0
    DoNetErrorFromStatus(status);
    DoClose();
  }
}

// (maybe) read_state_ == IDLE -> read_state_ == POSTED (via
// PostDoRead())

bool ChromeAsyncSocket::Read(char* data, size_t len, size_t* len_read) {
  if (!IsOpen() && (state_ != STATE_TLS_CONNECTING)) {
    // Read() may be called on a closed socket if a previous read
    // causes a socket close (e.g., client sends wrong password and
    // server terminates connection).
    //
    // TODO(akalin): Fix handling of this on the libjingle side.
    if (state_ != STATE_CLOSED) {
      LOG(DFATAL) << "Read() called on non-open non-tls-connecting socket";
    }
    DoNonNetError(ERROR_WRONGSTATE);
    return false;
  }
  DCHECK_LE(read_start_, read_end_);
  if ((state_ == STATE_TLS_CONNECTING) || read_end_ == 0U) {
    if (state_ == STATE_TLS_CONNECTING) {
      DCHECK_EQ(read_state_, IDLE);
      DCHECK_EQ(read_end_, 0U);
    } else {
      DCHECK_NE(read_state_, IDLE);
    }
    *len_read = 0;
    return true;
  }
  DCHECK_EQ(read_state_, IDLE);
  *len_read = std::min(len, read_end_ - read_start_);
  DCHECK_GT(*len_read, 0U);
  std::memcpy(data, read_buf_->data() + read_start_, *len_read);
  read_start_ += *len_read;
  if (read_start_ == read_end_) {
    read_start_ = 0U;
    read_end_ = 0U;
    // We defer execution of DoRead() here for similar reasons as
    // ProcessConnectDone().
    PostDoRead();
  }
  return true;
}

// (maybe) write_state_ == IDLE -> write_state_ == POSTED (via
// PostDoWrite())

bool ChromeAsyncSocket::Write(const char* data, size_t len) {
  if (!IsOpen() && (state_ != STATE_TLS_CONNECTING)) {
    LOG(DFATAL) << "Write() called on non-open non-tls-connecting socket";
    DoNonNetError(ERROR_WRONGSTATE);
    return false;
  }
  // TODO(akalin): Avoid this check by modifying the interface to have
  // a "ready for writing" signal.
  if ((static_cast<size_t>(write_buf_->size()) - write_end_) < len) {
    LOG(DFATAL) << "queueing " << len << " bytes would exceed the "
                << "max write buffer size = " << write_buf_->size()
                << " by " << (len - write_buf_->size()) << " bytes";
    DoNetError(net::ERR_INSUFFICIENT_RESOURCES);
    return false;
  }
  std::memcpy(write_buf_->data() + write_end_, data, len);
  write_end_ += len;
  // If we're TLS-connecting, the write buffer will get flushed once
  // the TLS-connect finishes.  Otherwise, start writing if we're not
  // already writing and we have something to write.
  if ((state_ != STATE_TLS_CONNECTING) &&
      (write_state_ == IDLE) && (write_end_ > 0U)) {
    // We defer execution of DoWrite() here for similar reasons as
    // ProcessConnectDone().
    PostDoWrite();
  }
  return true;
}

// write_state_ == IDLE -> write_state_ == POSTED

void ChromeAsyncSocket::PostDoWrite() {
  DCHECK(IsOpen());
  DCHECK_EQ(write_state_, IDLE);
  DCHECK_GT(write_end_, 0U);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&ChromeAsyncSocket::DoWrite, weak_ptr_factory_.GetWeakPtr()));
  write_state_ = POSTED;
}

// write_state_ == POSTED -> write_state_ == PENDING

void ChromeAsyncSocket::DoWrite() {
  DCHECK(IsOpen());
  DCHECK_EQ(write_state_, POSTED);
  DCHECK_GT(write_end_, 0U);
  // Once we call Write(), we cannot call StartTls() until the write
  // finishes.  This is okay, as StartTls() is called only after we
  // have received a reply to a message we sent to the server and
  // before we send the next message.
  int status =
      transport_socket_->Write(write_buf_.get(), write_end_,
                               base::Bind(&ChromeAsyncSocket::ProcessWriteDone,
                                          weak_ptr_factory_.GetWeakPtr()),
                               traffic_annotation_);
  write_state_ = PENDING;
  if (status != net::ERR_IO_PENDING) {
    ProcessWriteDone(status);
  }
}

// write_state_ == PENDING -> write_state_ == IDLE or POSTED (the
// latter via PostDoWrite())

void ChromeAsyncSocket::ProcessWriteDone(int status) {
  DCHECK_NE(status, net::ERR_IO_PENDING);
  DCHECK(IsOpen());
  DCHECK_EQ(write_state_, PENDING);
  DCHECK_GT(write_end_, 0U);
  write_state_ = IDLE;
  if (status < net::OK) {
    DoNetErrorFromStatus(status);
    DoClose();
    return;
  }
  size_t written = static_cast<size_t>(status);
  if (written > write_end_) {
    LOG(DFATAL) << "bytes written = " << written
                << " exceeds bytes requested = " << write_end_;
    DoNetError(net::ERR_UNEXPECTED);
    DoClose();
    return;
  }
  // TODO(akalin): Figure out a better way to do this; perhaps a queue
  // of DrainableIOBuffers.  This'll also allow us to not have an
  // artificial buffer size limit.
  std::memmove(write_buf_->data(),
               write_buf_->data() + written,
               write_end_ - written);
  write_end_ -= written;
  if (write_end_ > 0U) {
    PostDoWrite();
  }
}

// * -> STATE_CLOSED

bool ChromeAsyncSocket::Close() {
  DoClose();
  return true;
}

// (not STATE_CLOSED) -> STATE_CLOSED

void ChromeAsyncSocket::DoClose() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  if (transport_socket_.get()) {
    transport_socket_->Disconnect();
  }
  transport_socket_.reset();
  read_state_ = IDLE;
  read_start_ = 0U;
  read_end_ = 0U;
  write_state_ = IDLE;
  write_end_ = 0U;
  if (state_ != STATE_CLOSED) {
    state_ = STATE_CLOSED;
    SignalClosed();
  }
  // Reset error variables after SignalClosed() so slots connected
  // to it can read it.
  error_ = ERROR_NONE;
  net_error_ = net::OK;
}

// STATE_OPEN -> STATE_TLS_CONNECTING

bool ChromeAsyncSocket::StartTls(const std::string& domain_name) {
  if ((state_ != STATE_OPEN) || (read_state_ == PENDING) ||
      (write_state_ != IDLE)) {
    LOG(DFATAL) << "StartTls() called in wrong state";
    DoNonNetError(ERROR_WRONGSTATE);
    return false;
  }

  state_ = STATE_TLS_CONNECTING;
  read_state_ = IDLE;
  read_start_ = 0U;
  read_end_ = 0U;
  DCHECK_EQ(write_end_, 0U);

  // Clear out any posted DoRead() tasks.
  weak_ptr_factory_.InvalidateWeakPtrs();

  DCHECK(transport_socket_.get());
  std::unique_ptr<net::ClientSocketHandle> socket_handle(
      new net::ClientSocketHandle());
  socket_handle->SetSocket(std::move(transport_socket_));
  transport_socket_ = resolving_client_socket_factory_->CreateSSLClientSocket(
      std::move(socket_handle), net::HostPortPair(domain_name, 443));
  int status = transport_socket_->Connect(
      base::Bind(&ChromeAsyncSocket::ProcessSSLConnectDone,
                 weak_ptr_factory_.GetWeakPtr()));
  if (status != net::ERR_IO_PENDING) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ChromeAsyncSocket::ProcessSSLConnectDone,
                              weak_ptr_factory_.GetWeakPtr(), status));
  }
  return true;
}

// STATE_TLS_CONNECTING -> STATE_TLS_OPEN
// read_state_ == IDLE -> read_state_ == POSTED (via PostDoRead())
// (maybe) write_state_ == IDLE -> write_state_ == POSTED (via
// PostDoWrite())

void ChromeAsyncSocket::ProcessSSLConnectDone(int status) {
  DCHECK_NE(status, net::ERR_IO_PENDING);
  DCHECK_EQ(state_, STATE_TLS_CONNECTING);
  DCHECK_EQ(read_state_, IDLE);
  DCHECK_EQ(read_start_, 0U);
  DCHECK_EQ(read_end_, 0U);
  DCHECK_EQ(write_state_, IDLE);
  if (status != net::OK) {
    DoNetErrorFromStatus(status);
    DoClose();
    return;
  }
  state_ = STATE_TLS_OPEN;
  PostDoRead();
  if (write_end_ > 0U) {
    PostDoWrite();
  }
  SignalSSLConnected();
}

}  // namespace jingle_glue
