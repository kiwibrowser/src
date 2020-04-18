// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_NAVIGATION_PRELOAD_CALLBACKS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_NAVIGATION_PRELOAD_CALLBACKS_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_registration.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"

namespace blink {

class ScriptPromiseResolver;
struct WebNavigationPreloadState;
struct WebServiceWorkerError;

class EnableNavigationPreloadCallbacks final
    : public WebServiceWorkerRegistration::WebEnableNavigationPreloadCallbacks {
 public:
  explicit EnableNavigationPreloadCallbacks(ScriptPromiseResolver*);
  ~EnableNavigationPreloadCallbacks() override;

  // WebEnableNavigationPreloadCallbacks interface.
  void OnSuccess() override;
  void OnError(const WebServiceWorkerError&) override;

 private:
  Persistent<ScriptPromiseResolver> resolver_;
  DISALLOW_COPY_AND_ASSIGN(EnableNavigationPreloadCallbacks);
};

class GetNavigationPreloadStateCallbacks final
    : public WebServiceWorkerRegistration::
          WebGetNavigationPreloadStateCallbacks {
 public:
  explicit GetNavigationPreloadStateCallbacks(ScriptPromiseResolver*);
  ~GetNavigationPreloadStateCallbacks() override;

  // WebGetNavigationPreloadStateCallbacks interface.
  void OnSuccess(const WebNavigationPreloadState&) override;
  void OnError(const WebServiceWorkerError&) override;

 private:
  Persistent<ScriptPromiseResolver> resolver_;
  DISALLOW_COPY_AND_ASSIGN(GetNavigationPreloadStateCallbacks);
};

class SetNavigationPreloadHeaderCallbacks final
    : public WebServiceWorkerRegistration::
          WebSetNavigationPreloadHeaderCallbacks {
 public:
  explicit SetNavigationPreloadHeaderCallbacks(ScriptPromiseResolver*);
  ~SetNavigationPreloadHeaderCallbacks() override;

  // WebSetNavigationPreloadHeaderCallbacks interface.
  void OnSuccess(void) override;
  void OnError(const WebServiceWorkerError&) override;

 private:
  Persistent<ScriptPromiseResolver> resolver_;
  DISALLOW_COPY_AND_ASSIGN(SetNavigationPreloadHeaderCallbacks);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_NAVIGATION_PRELOAD_CALLBACKS_H_
