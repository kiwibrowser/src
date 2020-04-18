// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_WINDOW_CLIENT_CALLBACK_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_WINDOW_CLIENT_CALLBACK_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_clients_info.h"

namespace blink {

class ScriptPromiseResolver;

class NavigateClientCallback : public WebServiceWorkerClientCallbacks {
 public:
  explicit NavigateClientCallback(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}

  void OnSuccess(std::unique_ptr<WebServiceWorkerClientInfo>) override;
  void OnError(const WebServiceWorkerError&) override;

 private:
  Persistent<ScriptPromiseResolver> resolver_;
  DISALLOW_COPY_AND_ASSIGN(NavigateClientCallback);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_WINDOW_CLIENT_CALLBACK_H_
