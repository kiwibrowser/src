/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_H_

#include "third_party/blink/public/common/message_port/transferable_message.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_state.mojom-shared.h"
#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

class WebServiceWorkerProxy;

class WebServiceWorker {
 public:
  // The handle interface that retains a reference to the implementation of
  // WebServiceWorker in the embedder and is owned by ServiceWorker object in
  // Blink. The embedder must keep the service worker representation while
  // Blink is owning this handle.
  class Handle {
   public:
    virtual ~Handle() = default;
    virtual WebServiceWorker* ServiceWorker() { return nullptr; }
  };

  virtual ~WebServiceWorker() = default;

  // Sets ServiceWorkerProxy, with which callee can start making upcalls
  // to the ServiceWorker object via the client. This doesn't pass the
  // ownership to the callee, and the proxy's lifetime is same as that of
  // WebServiceWorker.
  virtual void SetProxy(WebServiceWorkerProxy*) {}
  virtual WebServiceWorkerProxy* Proxy() { return nullptr; }

  virtual WebURL Url() const { return WebURL(); }
  virtual mojom::ServiceWorkerState GetState() const {
    return mojom::ServiceWorkerState::kUnknown;
  }

  virtual void PostMessageToServiceWorker(TransferableMessage) = 0;

  using TerminateForTestingCallback = WebCallbacks<void, void>;
  virtual void TerminateForTesting(
      std::unique_ptr<TerminateForTestingCallback>) {}
};
}

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_H_
