// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_message_handler.h"

#include "base/bind.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "remoting/base/compound_buffer.h"

namespace remoting {

FileTransferMessageHandler::FileTransferMessageHandler(
    const std::string& name,
    std::unique_ptr<protocol::MessagePipe> pipe,
    std::unique_ptr<FileProxyWrapper> file_proxy_wrapper)
    : protocol::NamedMessagePipeHandler(name, std::move(pipe)),
      file_proxy_wrapper_(std::move(file_proxy_wrapper)) {
  DCHECK(file_proxy_wrapper_);
}

FileTransferMessageHandler::~FileTransferMessageHandler() = default;

void FileTransferMessageHandler::OnConnected() {
  // base::Unretained is safe here because |file_proxy_wrapper_| is owned by
  // this class, so the callback cannot be run after this class is destroyed.
  file_proxy_wrapper_->Init(base::BindOnce(
      &FileTransferMessageHandler::StatusCallback, base::Unretained(this)));
}

void FileTransferMessageHandler::OnIncomingMessage(
    std::unique_ptr<CompoundBuffer> buffer) {
  FileProxyWrapper::State proxy_state = file_proxy_wrapper_->state();
  if (proxy_state == FileProxyWrapper::kBusy ||
      proxy_state == FileProxyWrapper::kClosed ||
      proxy_state == FileProxyWrapper::kFailed) {
    return;
  }

  if (request_) {
    // File transfer is already in progress, just pass the buffer to
    // FileProxyWrapper to be written.
    SendToFileProxy(std::move(buffer));
  } else {
    // A new file transfer has been started, parse the message into a request
    // protobuf.
    ParseNewRequest(std::move(buffer));
  }
}

void FileTransferMessageHandler::OnDisconnecting() {
  FileProxyWrapper::State proxy_state = file_proxy_wrapper_->state();
  if (proxy_state != FileProxyWrapper::kClosed &&
      proxy_state != FileProxyWrapper::kFailed) {
    // Channel was closed earlier than expected, cancel the transfer.
    file_proxy_wrapper_->Cancel();
  }
}

void FileTransferMessageHandler::StatusCallback(
    FileProxyWrapper::State state,
    base::Optional<protocol::FileTransferResponse_ErrorCode> error) {
  protocol::FileTransferResponse response;
  if (error.has_value()) {
    DCHECK_EQ(state, FileProxyWrapper::kFailed);
    response.set_error(error.value());
  } else {
    DCHECK_EQ(state, FileProxyWrapper::kClosed);
    response.set_state(protocol::FileTransferResponse_TransferState_DONE);
    response.set_total_bytes_written(request_->filesize());
  }
  Send(response, base::Closure());
}

void FileTransferMessageHandler::SendToFileProxy(
    std::unique_ptr<CompoundBuffer> buffer) {
  DCHECK_EQ(file_proxy_wrapper_->state(), FileProxyWrapper::kReady);

  total_bytes_written_ += buffer->total_bytes();
  file_proxy_wrapper_->WriteChunk(std::move(buffer));
  if (total_bytes_written_ >= request_->filesize()) {
    file_proxy_wrapper_->Close();
  }

  if (total_bytes_written_ > request_->filesize()) {
    LOG(ERROR) << "File transfer received " << total_bytes_written_
               << " bytes, but request said there would only be "
               << request_->filesize() << " bytes.";
  }
}

void FileTransferMessageHandler::ParseNewRequest(
    std::unique_ptr<CompoundBuffer> buffer) {
  std::string message;
  message.resize(buffer->total_bytes());
  buffer->CopyTo(base::data(message), message.size());

  request_ = std::make_unique<protocol::FileTransferRequest>();
  if (!request_->ParseFromString(message)) {
    CancelAndSendError("Failed to parse request protobuf");
    return;
  }

  base::FilePath target_directory;
  if (!base::PathService::Get(base::DIR_USER_DESKTOP, &target_directory)) {
    CancelAndSendError(
        "Failed to get DIR_USER_DESKTOP from base::PathService::Get");
    return;
  }

  file_proxy_wrapper_->CreateFile(target_directory, request_->filename());
}

void FileTransferMessageHandler::CancelAndSendError(const std::string& error) {
  LOG(ERROR) << error;
  file_proxy_wrapper_->Cancel();
  protocol::FileTransferResponse response;
  response.set_error(protocol::FileTransferResponse_ErrorCode_UNEXPECTED_ERROR);
  Send(response, base::Closure());
}

}  // namespace remoting
