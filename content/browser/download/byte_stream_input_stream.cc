// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/byte_stream_input_stream.h"

#include "components/download/public/common/download_task_runner.h"
#include "content/browser/byte_stream.h"

namespace content {

ByteStreamInputStream::ByteStreamInputStream(
    std::unique_ptr<ByteStreamReader> stream_reader)
    : stream_reader_(
          stream_reader.release(),
          base::OnTaskRunnerDeleter(download::GetDownloadTaskRunner())),
      completion_status_(download::DOWNLOAD_INTERRUPT_REASON_NONE) {}

ByteStreamInputStream::~ByteStreamInputStream() = default;

bool ByteStreamInputStream::IsEmpty() {
  return !stream_reader_;
}

void ByteStreamInputStream::RegisterDataReadyCallback(
    const mojo::SimpleWatcher::ReadyCallback& callback) {
  if (stream_reader_) {
    stream_reader_->RegisterCallback(
        base::BindRepeating(callback, MOJO_RESULT_OK));
  }
}

void ByteStreamInputStream::ClearDataReadyCallback() {
  if (stream_reader_)
    stream_reader_->RegisterCallback(base::RepeatingClosure());
}

download::InputStream::StreamState ByteStreamInputStream::Read(
    scoped_refptr<net::IOBuffer>* data,
    size_t* length) {
  if (!stream_reader_)
    return download::InputStream::EMPTY;

  ByteStreamReader::StreamState state = stream_reader_->Read(data, length);
  switch (state) {
    case ByteStreamReader::STREAM_EMPTY:
      return download::InputStream::EMPTY;
    case ByteStreamReader::STREAM_HAS_DATA:
      return download::InputStream::HAS_DATA;
    case ByteStreamReader::STREAM_COMPLETE:
      completion_status_ = static_cast<download::DownloadInterruptReason>(
          stream_reader_->GetStatus());
      return download::InputStream::COMPLETE;
  }
  return download::InputStream::EMPTY;
}

download::DownloadInterruptReason ByteStreamInputStream::GetCompletionStatus() {
  return completion_status_;
}

}  // namespace content
