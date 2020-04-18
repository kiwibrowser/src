// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_REGISTRATION_PROXY_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_REGISTRATION_PROXY_H_

#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker.h"

#include <memory>

namespace blink {

// A proxy interface, passed via WebServiceWorkerRegistration.setProxy() from
// blink to the embedder, to talk to the ServiceWorkerRegistration object from
// embedder.
class WebServiceWorkerRegistrationProxy {
 public:
  // Notifies that the registration entered the installation process.
  // The installing worker should be accessible via
  // WebServiceWorkerRegistration.installing.
  virtual void DispatchUpdateFoundEvent() = 0;

  virtual void SetInstalling(std::unique_ptr<WebServiceWorker::Handle>) = 0;
  virtual void SetWaiting(std::unique_ptr<WebServiceWorker::Handle>) = 0;
  virtual void SetActive(std::unique_ptr<WebServiceWorker::Handle>) = 0;

 protected:
  virtual ~WebServiceWorkerRegistrationProxy() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_REGISTRATION_PROXY_H_
