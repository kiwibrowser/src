/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_NETWORK_PROVIDER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_NETWORK_PROVIDER_H_

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_url_loader.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace blink {

class WebURLRequest;

// This interface is implemented by the client and is only called on the main
// thread. HasControllerServiceWorker() and ControllerServiceWorkerID() are to
// be implemented only by Frame and SharedWorker's provider as they are needed
// only for controllee contexts (but not in controller context).
//
// An instance of this class is owned by the associated loading context, e.g.
// DocumentLoader.
class WebServiceWorkerNetworkProvider {
 public:
  virtual ~WebServiceWorkerNetworkProvider() = default;

  // A request is about to be sent out, and the client may modify it. Request
  // is writable, and changes to the URL, for example, will change the request
  // made.
  virtual void WillSendRequest(WebURLRequest&) {}

  // Returns an identifier of this provider.
  virtual int ProviderID() const { return -1; }

  // Whether the document associated with WebDocumentLoader is controlled by a
  // service worker.
  virtual bool HasControllerServiceWorker() { return false; }

  // Returns an identifier of the service worker controlling the document
  // associated with the WebDocumentLoader.
  virtual int64_t ControllerServiceWorkerID() { return -1; }

  // S13nServiceWorker:
  // Returns a URLLoader for the associated context. May return nullptr
  // if this doesn't provide a ServiceWorker specific URLLoader.
  virtual std::unique_ptr<WebURLLoader> CreateURLLoader(
      const WebURLRequest& request,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
    return nullptr;
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_NETWORK_PROVIDER_H_
