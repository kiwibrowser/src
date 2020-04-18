// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/streams/stream_url_request_job.h"

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/streams/stream.h"
#include "content/browser/streams/stream_metadata.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"

namespace content {

StreamURLRequestJob::StreamURLRequestJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate,
                                         scoped_refptr<Stream> stream)
    : net::URLRangeRequestJob(request, network_delegate),
      stream_(stream),
      pending_buffer_size_(0),
      total_bytes_read_(0),
      raw_body_bytes_(0),
      max_range_(0),
      request_failed_(false),
      error_code_(net::OK),
      weak_factory_(this) {
  DCHECK(stream_.get());
  stream_->SetReadObserver(this);
}

StreamURLRequestJob::~StreamURLRequestJob() {
  ClearStream();
}

void StreamURLRequestJob::OnDataAvailable(Stream* stream) {
  // Do nothing if pending_buffer_ is empty, i.e. there's no ReadRawData()
  // operation waiting for IO completion.
  if (!pending_buffer_.get())
    return;

  // pending_buffer_ is set to the IOBuffer instance provided to ReadRawData()
  // by URLRequestJob.

  int result = 0;
  switch (stream_->ReadRawData(pending_buffer_.get(), pending_buffer_size_,
                               &result)) {
    case Stream::STREAM_HAS_DATA:
      DCHECK_GT(result, 0);
      break;
    case Stream::STREAM_COMPLETE:
      // Ensure ReadRawData gives net::OK.
      DCHECK_EQ(net::OK, result);
      break;
    case Stream::STREAM_EMPTY:
      NOTREACHED();
      break;
    case Stream::STREAM_ABORTED:
      // Handle this as connection reset.
      result = net::ERR_CONNECTION_RESET;
      break;
  }

  // Clear the buffers before notifying the read is complete, so that it is
  // safe for the observer to read.
  pending_buffer_ = nullptr;
  pending_buffer_size_ = 0;

  if (result > 0)
    total_bytes_read_ += result;
  ReadRawDataComplete(result);
}

// net::URLRequestJob methods.
void StreamURLRequestJob::Start() {
  // Continue asynchronously.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&StreamURLRequestJob::DidStart,
                                weak_factory_.GetWeakPtr()));
}

void StreamURLRequestJob::Kill() {
  net::URLRequestJob::Kill();
  weak_factory_.InvalidateWeakPtrs();
  ClearStream();
}

int StreamURLRequestJob::ReadRawData(net::IOBuffer* buf, int buf_size) {
  if (request_failed_)
    return error_code_;

  DCHECK(buf);
  int to_read = buf_size;
  if (max_range_ && to_read) {
    if (to_read + total_bytes_read_ > max_range_)
      to_read = max_range_ - total_bytes_read_;

    if (to_read == 0)
      return 0;
  }

  int bytes_read = 0;
  switch (stream_->ReadRawData(buf, to_read, &bytes_read)) {
    case Stream::STREAM_HAS_DATA:
      total_bytes_read_ += bytes_read;
      raw_body_bytes_ = stream_->metadata()->raw_body_bytes();
      return bytes_read;
    case Stream::STREAM_COMPLETE:
      return stream_->GetStatus();
    case Stream::STREAM_EMPTY:
      pending_buffer_ = buf;
      pending_buffer_size_ = to_read;
      return net::ERR_IO_PENDING;
    case Stream::STREAM_ABORTED:
      // Handle this as connection reset.
      return net::ERR_CONNECTION_RESET;
  }
  NOTREACHED();
  return net::ERR_FAILED;
}

bool StreamURLRequestJob::GetMimeType(std::string* mime_type) const {
  const StreamMetadata* metadata = stream_->metadata();
  if (!metadata || !metadata->response_info().headers)
    return false;

  // TODO(zork): Support registered MIME types if needed.
  return metadata->response_info().headers->GetMimeType(mime_type);
}

void StreamURLRequestJob::GetResponseInfo(net::HttpResponseInfo* info) {
  if (failed_response_info_) {
    *info = *failed_response_info_;
    return;
  }
  const StreamMetadata* metadata = stream_->metadata();
  if (metadata)
    *info = metadata->response_info();
}

int64_t StreamURLRequestJob::GetTotalReceivedBytes() const {
  if (!stream_)
    return 0;
  const StreamMetadata* metadata = stream_->metadata();
  if (!metadata)
    return 0;
  return metadata->total_received_bytes();
}

int64_t StreamURLRequestJob::prefilter_bytes_read() const {
  return raw_body_bytes_;
}

void StreamURLRequestJob::DidStart() {
  if (range_parse_result() == net::OK && ranges().size() > 0) {
    // Only one range is supported, and it must start at the first byte.
    if (ranges().size() > 1 || ranges()[0].first_byte_position() != 0) {
      NotifyMethodNotSupported();
      return;
    }

    max_range_ = ranges()[0].last_byte_position() + 1;
  }

  // This class only supports GET requests.
  if (request()->method() != "GET") {
    NotifyMethodNotSupported();
    return;
  }
  NotifyHeadersComplete();
}

void StreamURLRequestJob::NotifyMethodNotSupported() {
  const net::HttpStatusCode status_code = net::HTTP_METHOD_NOT_ALLOWED;
  request_failed_ = true;
  error_code_ = net::ERR_METHOD_NOT_SUPPORTED;

  std::string status("HTTP/1.1 ");
  status.append(base::IntToString(status_code));
  status.append(" ");
  status.append(net::GetHttpReasonPhrase(status_code));
  status.append("\0\0", 2);
  net::HttpResponseHeaders* headers = new net::HttpResponseHeaders(status);

  failed_response_info_.reset(new net::HttpResponseInfo());
  failed_response_info_->headers = headers;
  NotifyHeadersComplete();
}

void StreamURLRequestJob::ClearStream() {
  if (stream_.get()) {
    stream_->RemoveReadObserver(this);
    stream_ = nullptr;
  }
}

}  // namespace content
