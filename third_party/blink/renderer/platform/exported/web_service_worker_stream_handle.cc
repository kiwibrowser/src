// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_stream_handle.h"

namespace blink {

void WebServiceWorkerStreamHandle::SetListener(
    std::unique_ptr<Listener> listener) {
  DCHECK(!listener_);
  listener_ = std::move(listener);
}

void WebServiceWorkerStreamHandle::Aborted() {
  DCHECK(listener_);
  listener_->OnAborted();
}

void WebServiceWorkerStreamHandle::Completed() {
  DCHECK(listener_);
  listener_->OnCompleted();
}

mojo::ScopedDataPipeConsumerHandle
WebServiceWorkerStreamHandle::DrainStreamDataPipe() {
  DCHECK(stream_.is_valid());
  return std::move(stream_);
}

}  // namespace blink
