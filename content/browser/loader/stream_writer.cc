// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/stream_writer.h"

#include "base/callback_helpers.h"
#include "base/guid.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/streams/stream.h"
#include "content/browser/streams/stream_registry.h"
#include "net/base/io_buffer.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace content {

StreamWriter::StreamWriter() : immediate_mode_(false) {}

StreamWriter::~StreamWriter() {
  if (stream_.get())
    Finalize(0);
}

void StreamWriter::InitializeStream(StreamRegistry* registry,
                                    const GURL& origin,
                                    const base::Closure& cancel_callback) {
  cancel_callback_ = cancel_callback;
  DCHECK(!stream_.get());

  // TODO(tyoshino): Find a way to share this with the blob URL creation in
  // WebKit.
  GURL url(std::string(url::kBlobScheme) + ":" + origin.spec() +
           base::GenerateGUID());
  stream_ = new Stream(registry, this, url);
}

void StreamWriter::OnResponseStarted(
    const net::HttpResponseInfo& response_info) {
  stream_->OnResponseStarted(response_info);
}

void StreamWriter::UpdateNetworkStats(int64_t raw_body_bytes,
                                      int64_t total_bytes) {
  stream_->UpdateNetworkStats(raw_body_bytes, total_bytes);
}

void StreamWriter::OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                              int* buf_size,
                              int min_size) {
  static const int kReadBufSize = 32768;

  DCHECK(buf);
  DCHECK(buf_size);
  DCHECK_LE(min_size, kReadBufSize);
  if (!read_buffer_.get())
    read_buffer_ = new net::IOBuffer(kReadBufSize);
  *buf = read_buffer_.get();
  *buf_size = kReadBufSize;
}

void StreamWriter::OnReadCompleted(
    int bytes_read,
    const base::Closure& need_more_data_callback) {
  DCHECK(!need_more_data_callback_);
  if (!bytes_read) {
    need_more_data_callback.Run();
    return;
  }

  // We have more data to read.
  DCHECK(read_buffer_.get());

  // Release the ownership of the buffer, and store a reference
  // to it. A new one will be allocated in OnWillRead().
  scoped_refptr<net::IOBuffer> buffer;
  read_buffer_.swap(buffer);
  stream_->AddData(buffer, bytes_read);

  if (immediate_mode_)
    stream_->Flush();

  if (!stream_->can_add_data()) {
    need_more_data_callback_ = need_more_data_callback;
    return;
  }
  need_more_data_callback.Run();
}

void StreamWriter::Finalize(int status) {
  DCHECK(stream_.get());
  stream_->Finalize(status);
  stream_->RemoveWriteObserver(this);
  stream_ = nullptr;
}

void StreamWriter::OnSpaceAvailable(Stream* stream) {
  base::ResetAndReturn(&need_more_data_callback_).Run();
}

void StreamWriter::OnClose(Stream* stream) {
  cancel_callback_.Run();
}

}  // namespace content
