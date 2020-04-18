// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_WINDOW_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_WINDOW_CLIENT_H_

#include <memory>
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_client.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class ScriptPromiseResolver;
class ScriptState;

class MODULES_EXPORT ServiceWorkerWindowClient final
    : public ServiceWorkerClient {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // To be used by CallbackPromiseAdapter.
  using WebType = std::unique_ptr<WebServiceWorkerClientInfo>;

  static ServiceWorkerWindowClient* Take(
      ScriptPromiseResolver*,
      std::unique_ptr<WebServiceWorkerClientInfo>);

  static ServiceWorkerWindowClient* Create(const WebServiceWorkerClientInfo&);
  ~ServiceWorkerWindowClient() override;

  // WindowClient.idl
  String visibilityState() const;
  bool focused() const { return is_focused_; }
  ScriptPromise focus(ScriptState*);
  ScriptPromise navigate(ScriptState*, const String& url);

  void Trace(blink::Visitor*) override;

 private:
  explicit ServiceWorkerWindowClient(const WebServiceWorkerClientInfo&);

  mojom::PageVisibilityState page_visibility_state_;
  bool is_focused_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_WINDOW_CLIENT_H_
