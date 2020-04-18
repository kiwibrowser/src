// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_transport.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/format_macros.h"
#include "base/location.h"
#include "base/numerics/safe_conversions.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/cast_channel/cast_framer.h"
#include "components/cast_channel/cast_message_util.h"
#include "components/cast_channel/logger.h"
#include "components/cast_channel/proto/cast_channel.pb.h"
#include "net/base/net_errors.h"
#include "net/socket/socket.h"

#define VLOG_WITH_CONNECTION(level) \
  VLOG(level) << "[" << ip_endpoint_.ToString() << ", auth=SSL_VERIFIED] "

namespace cast_channel {

CastTransportImpl::CastTransportImpl(net::Socket* socket,
                                     int channel_id,
                                     const net::IPEndPoint& ip_endpoint,
                                     scoped_refptr<Logger> logger)
    : started_(false),
      socket_(socket),
      write_state_(WriteState::IDLE),
      read_state_(ReadState::READ),
      error_state_(ChannelError::NONE),
      channel_id_(channel_id),
      ip_endpoint_(ip_endpoint),
      logger_(logger) {
  DCHECK(socket);

  // Buffer is reused across messages to minimize unnecessary buffer
  // [re]allocations.
  read_buffer_ = new net::GrowableIOBuffer();
  read_buffer_->SetCapacity(MessageFramer::MessageHeader::max_message_size());
  framer_.reset(new MessageFramer(read_buffer_));
}

CastTransportImpl::~CastTransportImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  FlushWriteQueue();
}

bool CastTransportImpl::IsTerminalWriteState(WriteState write_state) {
  return write_state == WriteState::WRITE_ERROR ||
         write_state == WriteState::IDLE;
}

bool CastTransportImpl::IsTerminalReadState(ReadState read_state) {
  return read_state == ReadState::READ_ERROR;
}


void CastTransportImpl::SetReadDelegate(std::unique_ptr<Delegate> delegate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(delegate);
  delegate_ = std::move(delegate);
  if (started_) {
    delegate_->Start();
  }
}

void CastTransportImpl::FlushWriteQueue() {
  for (; !write_queue_.empty(); write_queue_.pop()) {
    net::CompletionCallback& callback = write_queue_.front().callback;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, net::ERR_FAILED));
    callback.Reset();
  }
}

void CastTransportImpl::SendMessage(
    const CastMessage& message,
    const net::CompletionCallback& callback,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsCastMessageValid(message));
  std::string serialized_message;
  if (!MessageFramer::Serialize(message, &serialized_message)) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, net::ERR_FAILED));
    return;
  }
  WriteRequest write_request(message.namespace_(), serialized_message, callback,
                             traffic_annotation);

  write_queue_.push(write_request);
  if (write_state_ == WriteState::IDLE) {
    SetWriteState(WriteState::WRITE);
    OnWriteResult(net::OK);
  }
}

CastTransportImpl::WriteRequest::WriteRequest(
    const std::string& namespace_,
    const std::string& payload,
    const net::CompletionCallback& callback,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : message_namespace(namespace_),
      callback(callback),
      traffic_annotation_(traffic_annotation) {
  VLOG(2) << "WriteRequest size: " << payload.size();
  io_buffer = new net::DrainableIOBuffer(new net::StringIOBuffer(payload),
                                         payload.size());
}

CastTransportImpl::WriteRequest::WriteRequest(const WriteRequest& other) =
    default;

CastTransportImpl::WriteRequest::~WriteRequest() {}

void CastTransportImpl::SetReadState(ReadState read_state) {
  if (read_state_ != read_state)
    read_state_ = read_state;
}

void CastTransportImpl::SetWriteState(WriteState write_state) {
  if (write_state_ != write_state)
    write_state_ = write_state;
}

void CastTransportImpl::SetErrorState(ChannelError error_state) {
  VLOG_WITH_CONNECTION(2) << "SetErrorState: "
                          << ::cast_channel::ChannelErrorToString(error_state);
  error_state_ = error_state;
}

void CastTransportImpl::OnWriteResult(int result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_NE(WriteState::IDLE, write_state_);
  if (write_queue_.empty()) {
    SetWriteState(WriteState::IDLE);
    return;
  }

  // Network operations can either finish synchronously or asynchronously.
  // This method executes the state machine transitions in a loop so that
  // write state transitions happen even when network operations finish
  // synchronously.
  int rv = result;
  do {
    VLOG_WITH_CONNECTION(2)
        << "OnWriteResult (state=" << AsInteger(write_state_) << ", "
        << "result=" << rv << ", "
        << "queue size=" << write_queue_.size() << ")";

    WriteState state = write_state_;
    write_state_ = WriteState::UNKNOWN;
    switch (state) {
      case WriteState::WRITE:
        rv = DoWrite();
        break;
      case WriteState::WRITE_COMPLETE:
        rv = DoWriteComplete(rv);
        break;
      case WriteState::DO_CALLBACK:
        rv = DoWriteCallback();
        break;
      case WriteState::HANDLE_ERROR:
        rv = DoWriteHandleError(rv);
        DCHECK_EQ(WriteState::WRITE_ERROR, write_state_);
        break;
      default:
        NOTREACHED() << "Unknown state in write state machine: "
                     << AsInteger(state);
        SetWriteState(WriteState::WRITE_ERROR);
        SetErrorState(ChannelError::UNKNOWN);
        rv = net::ERR_FAILED;
        break;
    }
  } while (rv != net::ERR_IO_PENDING && !IsTerminalWriteState(write_state_));

  if (write_state_ == WriteState::WRITE_ERROR) {
    FlushWriteQueue();
    DCHECK_NE(ChannelError::NONE, error_state_);
    VLOG_WITH_CONNECTION(2) << "Sending OnError().";
    delegate_->OnError(error_state_);
  }
}

int CastTransportImpl::DoWrite() {
  DCHECK(!write_queue_.empty());
  WriteRequest& request = write_queue_.front();

  VLOG_WITH_CONNECTION(2) << "WriteData byte_count = "
                          << request.io_buffer->size() << " bytes_written "
                          << request.io_buffer->BytesConsumed();

  SetWriteState(WriteState::WRITE_COMPLETE);

  int rv = socket_->Write(
      request.io_buffer.get(), request.io_buffer->BytesRemaining(),
      base::Bind(&CastTransportImpl::OnWriteResult, base::Unretained(this)),
      request.traffic_annotation_);
  return rv;
}

int CastTransportImpl::DoWriteComplete(int result) {
  VLOG_WITH_CONNECTION(2) << "DoWriteComplete result=" << result;
  DCHECK(!write_queue_.empty());
  if (result <= 0) {  // NOTE that 0 also indicates an error
    logger_->LogSocketEventWithRv(channel_id_, ChannelEvent::SOCKET_WRITE,
                                  result);
    SetErrorState(ChannelError::CAST_SOCKET_ERROR);
    SetWriteState(WriteState::HANDLE_ERROR);
    return result == 0 ? net::ERR_FAILED : result;
  }

  // Some bytes were successfully written
  WriteRequest& request = write_queue_.front();
  scoped_refptr<net::DrainableIOBuffer> io_buffer = request.io_buffer;
  io_buffer->DidConsume(result);
  if (io_buffer->BytesRemaining() == 0) {  // Message fully sent
    SetWriteState(WriteState::DO_CALLBACK);
  } else {
    SetWriteState(WriteState::WRITE);
  }

  return net::OK;
}

int CastTransportImpl::DoWriteCallback() {
  VLOG_WITH_CONNECTION(2) << "DoWriteCallback";
  DCHECK(!write_queue_.empty());

  WriteRequest& request = write_queue_.front();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(request.callback, net::OK));

  write_queue_.pop();
  if (write_queue_.empty()) {
    SetWriteState(WriteState::IDLE);
  } else {
    SetWriteState(WriteState::WRITE);
  }

  return net::OK;
}

int CastTransportImpl::DoWriteHandleError(int result) {
  VLOG_WITH_CONNECTION(2) << "DoWriteHandleError result=" << result;
  DCHECK_NE(ChannelError::NONE, error_state_);
  DCHECK_LT(result, 0);
  SetWriteState(WriteState::WRITE_ERROR);
  return net::ERR_FAILED;
}

void CastTransportImpl::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!started_);
  DCHECK_EQ(ReadState::READ, read_state_);
  DCHECK(delegate_) << "Read delegate must be set prior to calling Start()";
  started_ = true;
  delegate_->Start();
  SetReadState(ReadState::READ);

  // Start the read state machine.
  OnReadResult(net::OK);
}

void CastTransportImpl::OnReadResult(int result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Network operations can either finish synchronously or asynchronously.
  // This method executes the state machine transitions in a loop so that
  // write state transitions happen even when network operations finish
  // synchronously.
  int rv = result;
  do {
    VLOG_WITH_CONNECTION(2) << "OnReadResult(state=" << AsInteger(read_state_)
                            << ", result=" << rv << ")";
    ReadState state = read_state_;
    read_state_ = ReadState::UNKNOWN;

    switch (state) {
      case ReadState::READ:
        rv = DoRead();
        break;
      case ReadState::READ_COMPLETE:
        rv = DoReadComplete(rv);
        break;
      case ReadState::DO_CALLBACK:
        rv = DoReadCallback();
        break;
      case ReadState::HANDLE_ERROR:
        rv = DoReadHandleError(rv);
        DCHECK_EQ(read_state_, ReadState::READ_ERROR);
        break;
      default:
        NOTREACHED() << "Unknown state in read state machine: "
                     << AsInteger(state);
        SetReadState(ReadState::READ_ERROR);
        SetErrorState(ChannelError::UNKNOWN);
        rv = net::ERR_FAILED;
        break;
    }
  } while (rv != net::ERR_IO_PENDING && !IsTerminalReadState(read_state_));

  if (IsTerminalReadState(read_state_)) {
    DCHECK_EQ(ReadState::READ_ERROR, read_state_);
    VLOG_WITH_CONNECTION(2) << "Sending OnError().";
    delegate_->OnError(error_state_);
  }
}

int CastTransportImpl::DoRead() {
  VLOG_WITH_CONNECTION(2) << "DoRead";
  SetReadState(ReadState::READ_COMPLETE);

  // Determine how many bytes need to be read.
  size_t num_bytes_to_read = framer_->BytesRequested();
  DCHECK_GT(num_bytes_to_read, 0u);

  // Read up to num_bytes_to_read into |current_read_buffer_|.
  return socket_->Read(
      read_buffer_.get(), base::checked_cast<uint32_t>(num_bytes_to_read),
      base::Bind(&CastTransportImpl::OnReadResult, base::Unretained(this)));
}

int CastTransportImpl::DoReadComplete(int result) {
  VLOG_WITH_CONNECTION(2) << "DoReadComplete result = " << result;
  if (result <= 0) {
    logger_->LogSocketEventWithRv(channel_id_, ChannelEvent::SOCKET_READ,
                                  result);
    VLOG_WITH_CONNECTION(1) << "Read error, peer closed the socket.";
    SetErrorState(ChannelError::CAST_SOCKET_ERROR);
    SetReadState(ReadState::HANDLE_ERROR);
    return result == 0 ? net::ERR_FAILED : result;
  }

  size_t message_size;
  DCHECK(!current_message_);
  ChannelError framing_error;
  current_message_ = framer_->Ingest(result, &message_size, &framing_error);
  if (current_message_.get() && (framing_error == ChannelError::NONE)) {
    DCHECK_GT(message_size, static_cast<size_t>(0));
    SetReadState(ReadState::DO_CALLBACK);
  } else if (framing_error != ChannelError::NONE) {
    DCHECK(!current_message_);
    SetErrorState(ChannelError::INVALID_MESSAGE);
    SetReadState(ReadState::HANDLE_ERROR);
  } else {
    DCHECK(!current_message_);
    SetReadState(ReadState::READ);
  }
  return net::OK;
}

int CastTransportImpl::DoReadCallback() {
  VLOG_WITH_CONNECTION(2) << "DoReadCallback";
  if (!IsCastMessageValid(*current_message_)) {
    SetReadState(ReadState::HANDLE_ERROR);
    SetErrorState(ChannelError::INVALID_MESSAGE);
    return net::ERR_INVALID_RESPONSE;
  }
  SetReadState(ReadState::READ);
  delegate_->OnMessage(*current_message_);
  current_message_.reset();
  return net::OK;
}

int CastTransportImpl::DoReadHandleError(int result) {
  VLOG_WITH_CONNECTION(2) << "DoReadHandleError";
  DCHECK_NE(ChannelError::NONE, error_state_);
  DCHECK_LE(result, 0);
  SetReadState(ReadState::READ_ERROR);
  return net::ERR_FAILED;
}

}  // namespace cast_channel
