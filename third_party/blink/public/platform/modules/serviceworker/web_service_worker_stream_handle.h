// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_STREAM_HANDLE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_STREAM_HANDLE_H_

#include <memory>

#include "mojo/public/cpp/system/data_pipe.h"
#include "third_party/blink/public/platform/web_common.h"

namespace blink {

// Contains the info to send back a body to the page over Mojo's data pipe.
class BLINK_PLATFORM_EXPORT WebServiceWorkerStreamHandle {
 public:
  // Listener can observe whether the data pipe is successfully closed at the
  // end of the body or it has accidentally finished.
  class Listener {
   public:
    virtual ~Listener() = default;
    ;
    virtual void OnAborted() = 0;
    virtual void OnCompleted() = 0;
  };

  void SetListener(std::unique_ptr<Listener>);
  mojo::ScopedDataPipeConsumerHandle DrainStreamDataPipe();

#if INSIDE_BLINK
  WebServiceWorkerStreamHandle(mojo::ScopedDataPipeConsumerHandle stream)
      : stream_(std::move(stream)) {
    DCHECK(stream_.is_valid());
  }
  void Aborted();
  void Completed();
#endif

 private:
  mojo::ScopedDataPipeConsumerHandle stream_;
  std::unique_ptr<Listener> listener_;
};
}

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_STREAM_HANDLE_H_
