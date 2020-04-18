// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/in_memory_request_job.h"

#include "base/threading/thread_task_runner_handle.h"
#include "headless/public/util/in_memory_protocol_handler.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"

namespace headless {

InMemoryRequestJob::InMemoryRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const InMemoryProtocolHandler::Response* response)
    : net::URLRequestJob(request, network_delegate),
      response_(response),
      data_offset_(0),
      weak_factory_(this) {}

InMemoryRequestJob::~InMemoryRequestJob() = default;

void InMemoryRequestJob::Start() {
  // Start reading asynchronously so that all error reporting and data
  // callbacks happen as they would for network requests.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&InMemoryRequestJob::StartAsync,
                                weak_factory_.GetWeakPtr()));
}

void InMemoryRequestJob::StartAsync() {
  if (!response_) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           net::ERR_FILE_NOT_FOUND));
    return;
  }
  NotifyHeadersComplete();
}

void InMemoryRequestJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}

int InMemoryRequestJob::ReadRawData(net::IOBuffer* buf, int buf_size) {
  DCHECK(response_);
  DCHECK_LE(data_offset_, response_->data.size());
  int remaining =
      base::checked_cast<int>(response_->data.size() - data_offset_);
  if (buf_size > remaining)
    buf_size = remaining;
  memcpy(buf->data(), response_->data.data() + data_offset_, buf_size);
  data_offset_ += buf_size;
  return buf_size;
}

bool InMemoryRequestJob::GetMimeType(std::string* mime_type) const {
  *mime_type = response_->mime_type;
  return true;
}

}  // namespace headless
