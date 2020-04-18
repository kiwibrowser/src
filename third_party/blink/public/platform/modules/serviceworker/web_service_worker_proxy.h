// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_PROXY_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_PROXY_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

// A proxy interface, passed via WebServiceWorker.setProxy() from blink to
// the embedder, to talk to the ServiceWorker object from embedder.
class WebServiceWorkerProxy {
 public:
  // Notifies the proxy that the service worker state changed. The new state
  // should be accessible via WebServiceWorker.state().
  virtual void DispatchStateChangeEvent() = 0;

 protected:
  virtual ~WebServiceWorkerProxy() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_PROXY_H_
