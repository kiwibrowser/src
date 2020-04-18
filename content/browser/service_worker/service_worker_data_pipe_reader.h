// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_DATA_PIPE_READER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_DATA_PIPE_READER_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_stream_handle.mojom.h"

namespace net {
class IOBuffer;
}  // namespace net

namespace content {

class ServiceWorkerURLRequestJob;
class ServiceWorkerVersion;

// Reads a stream response for ServiceWorkerURLRequestJob passed through
// Mojo's data pipe. Owned by ServiceWorkerURLRequestJob.
class CONTENT_EXPORT ServiceWorkerDataPipeReader
    : public blink::mojom::ServiceWorkerStreamCallback {
 public:
  ServiceWorkerDataPipeReader(
      ServiceWorkerURLRequestJob* owner,
      scoped_refptr<ServiceWorkerVersion> streaming_version,
      blink::mojom::ServiceWorkerStreamHandlePtr stream_handle);
  ~ServiceWorkerDataPipeReader() override;

  // Starts reading the stream. Calls owner_->OnResponseStarted.
  void Start();

  // Same as URLRequestJob::ReadRawData. If ERR_IO_PENDING is returned,
  // owner_->OnReadRawDataComplete will be called when the read completes.
  int ReadRawData(net::IOBuffer* buf, int buf_size);

  // Implements mojom::ServiceWorkerStreamCallback.
  void OnCompleted() override;
  void OnAborted() override;

 private:
  enum class State { kStreaming, kCompleted, kAborted };

  // Callback method for |handle_watcher_|.
  void OnHandleGotSignal(MojoResult);

  // Finalizes the job. These must be called when state() is not
  // State::STREAMING.
  int SyncComplete();
  void AsyncComplete();

  State state();

  ServiceWorkerURLRequestJob* owner_;
  scoped_refptr<ServiceWorkerVersion> streaming_version_;
  scoped_refptr<net::IOBuffer> stream_pending_buffer_;
  int stream_pending_buffer_size_;
  mojo::SimpleWatcher handle_watcher_;
  mojo::ScopedDataPipeConsumerHandle stream_;
  mojo::Binding<blink::mojom::ServiceWorkerStreamCallback> binding_;
  // State notified via ServiceWorkerStreamCallback. |producer_state_| is
  // STREAMING until OnCompleted or OnAborted is called. Note that |stream_|
  // might be closed even if |producer_state_| is STREAMING. In order to see the
  // state of ServiceWorkerDataPipeReader, use state() instead.
  State producer_state_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerDataPipeReader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_DATA_PIPE_READER_H_
