// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_data_pipe_reader.h"

#include "base/trace_event/trace_event.h"
#include "content/browser/service_worker/service_worker_url_request_job.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "net/base/io_buffer.h"

namespace content {

ServiceWorkerDataPipeReader::ServiceWorkerDataPipeReader(
    ServiceWorkerURLRequestJob* owner,
    scoped_refptr<ServiceWorkerVersion> streaming_version,
    blink::mojom::ServiceWorkerStreamHandlePtr stream_handle)
    : owner_(owner),
      streaming_version_(streaming_version),
      stream_pending_buffer_size_(0),
      handle_watcher_(FROM_HERE,
                      mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                      base::SequencedTaskRunnerHandle::Get()),
      stream_(std::move(stream_handle->stream)),
      binding_(this, std::move(stream_handle->callback_request)),
      producer_state_(State::kStreaming) {
  TRACE_EVENT_ASYNC_BEGIN1("ServiceWorker", "ServiceWorkerDataPipeReader", this,
                           "Url", owner->request()->url().spec());
  streaming_version_->OnStreamResponseStarted();
  binding_.set_connection_error_handler(base::BindOnce(
      &ServiceWorkerDataPipeReader::OnAborted, base::Unretained(this)));
}

ServiceWorkerDataPipeReader::~ServiceWorkerDataPipeReader() {
  DCHECK(streaming_version_);
  streaming_version_->OnStreamResponseFinished();
  streaming_version_ = nullptr;

  TRACE_EVENT_ASYNC_END0("ServiceWorker", "ServiceWorkerDataPipeReader", this);
}

void ServiceWorkerDataPipeReader::Start() {
  TRACE_EVENT_ASYNC_STEP_INTO0("ServiceWorker", "ServiceWorkerDataPipeReader",
                               this, "Start");
  handle_watcher_.Watch(
      stream_.get(),
      MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED,
      base::Bind(&ServiceWorkerDataPipeReader::OnHandleGotSignal,
                 base::Unretained(this)));
  owner_->OnResponseStarted();
}

void ServiceWorkerDataPipeReader::OnHandleGotSignal(MojoResult) {
  TRACE_EVENT_ASYNC_STEP_INTO0("ServiceWorker", "ServiceWorkerDataPipeReader",
                               this, "OnHandleGotSignal");
  DCHECK(stream_pending_buffer_);

  // If state() is not STREAMING, it means the data pipe was disconnected and
  // OnCompleted/OnAborted has already been called.
  if (state() != State::kStreaming) {
    handle_watcher_.Cancel();
    AsyncComplete();
  }

  // |stream_pending_buffer_| is set to the IOBuffer instance provided to
  // ReadRawData() by URLRequestJob.
  uint32_t size_to_pass = stream_pending_buffer_size_;
  MojoResult mojo_result = stream_->ReadData(
      stream_pending_buffer_->data(), &size_to_pass, MOJO_READ_DATA_FLAG_NONE);

  switch (mojo_result) {
    case MOJO_RESULT_OK:
      stream_pending_buffer_ = nullptr;
      stream_pending_buffer_size_ = 0;
      owner_->OnReadRawDataComplete(size_to_pass);
      return;
    case MOJO_RESULT_FAILED_PRECONDITION:
      stream_.reset();
      handle_watcher_.Cancel();
      // If OnCompleted/OnAborted has already been called, let this request
      // complete.
      if (state() != State::kStreaming)
        AsyncComplete();
      return;
    case MOJO_RESULT_SHOULD_WAIT:
    // MOJO_RESULT_SHOULD_WAIT should not be returned since
    // OnHandleGotSignal should be called by readable or closed signals.
    case MOJO_RESULT_INVALID_ARGUMENT:
    case MOJO_RESULT_OUT_OF_RANGE:
    case MOJO_RESULT_BUSY:
      break;
  }
  NOTREACHED();
}

int ServiceWorkerDataPipeReader::ReadRawData(net::IOBuffer* buf, int buf_size) {
  TRACE_EVENT_ASYNC_STEP_INTO0("ServiceWorker", "ServiceWorkerDataPipeReader",
                               this, "ReadRawData");
  DCHECK(!stream_pending_buffer_);
  // If state() is not STREAMING, it means the data pipe was disconnected and
  // OnCompleted/OnAborted has already been called.
  if (state() != State::kStreaming)
    return SyncComplete();

  uint32_t size_to_pass = buf_size;
  MojoResult mojo_result =
      stream_->ReadData(buf->data(), &size_to_pass, MOJO_READ_DATA_FLAG_NONE);
  switch (mojo_result) {
    case MOJO_RESULT_OK:
      return size_to_pass;
    case MOJO_RESULT_FAILED_PRECONDITION:
      stream_.reset();
      handle_watcher_.Cancel();
      // Complete/Abort asynchronously if OnCompleted/OnAborted has not been
      // called yet.
      if (state() == State::kStreaming) {
        stream_pending_buffer_ = buf;
        stream_pending_buffer_size_ = buf_size;
        return net::ERR_IO_PENDING;
      }
      return SyncComplete();
    case MOJO_RESULT_SHOULD_WAIT:
      stream_pending_buffer_ = buf;
      stream_pending_buffer_size_ = buf_size;
      handle_watcher_.ArmOrNotify();
      return net::ERR_IO_PENDING;
    case MOJO_RESULT_INVALID_ARGUMENT:
    case MOJO_RESULT_OUT_OF_RANGE:
    case MOJO_RESULT_BUSY:
      break;
  }
  NOTREACHED();
  return net::ERR_FAILED;
}

void ServiceWorkerDataPipeReader::OnCompleted() {
  producer_state_ = State::kCompleted;
  if (stream_pending_buffer_ && state() != State::kStreaming)
    AsyncComplete();
}

void ServiceWorkerDataPipeReader::OnAborted() {
  producer_state_ = State::kAborted;
  if (stream_pending_buffer_ && state() != State::kStreaming)
    AsyncComplete();
}

void ServiceWorkerDataPipeReader::AsyncComplete() {
  // This works only after ReadRawData returns net::ERR_IO_PENDING.
  DCHECK(stream_pending_buffer_);

  switch (state()) {
    case State::kStreaming:
      NOTREACHED();
      break;
    case State::kCompleted:
      stream_pending_buffer_ = nullptr;
      stream_pending_buffer_size_ = 0;
      handle_watcher_.Cancel();
      owner_->RecordResult(ServiceWorkerMetrics::REQUEST_JOB_STREAM_RESPONSE);
      owner_->OnReadRawDataComplete(net::OK);
      return;
    case State::kAborted:
      stream_pending_buffer_ = nullptr;
      stream_pending_buffer_size_ = 0;
      handle_watcher_.Cancel();
      owner_->RecordResult(
          ServiceWorkerMetrics::REQUEST_JOB_ERROR_STREAM_ABORTED);
      owner_->OnReadRawDataComplete(net::ERR_CONNECTION_RESET);
      return;
  }
}

int ServiceWorkerDataPipeReader::SyncComplete() {
  // This works only in ReadRawData.
  DCHECK(!stream_pending_buffer_);

  switch (state()) {
    case State::kStreaming:
      break;
    case State::kCompleted:
      owner_->RecordResult(ServiceWorkerMetrics::REQUEST_JOB_STREAM_RESPONSE);
      return net::OK;
    case State::kAborted:
      owner_->RecordResult(
          ServiceWorkerMetrics::REQUEST_JOB_ERROR_STREAM_ABORTED);
      return net::ERR_CONNECTION_RESET;
  }
  NOTREACHED();
  return net::ERR_FAILED;
}

ServiceWorkerDataPipeReader::State ServiceWorkerDataPipeReader::state() {
  if (!stream_.is_valid())
    return producer_state_;
  return State::kStreaming;
}

}  // namespace content
