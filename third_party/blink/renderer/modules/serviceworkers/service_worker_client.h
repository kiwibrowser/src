// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_CLIENT_H_

#include <memory>
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_clients_info.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class ScriptPromiseResolver;
class ScriptState;

class MODULES_EXPORT ServiceWorkerClient : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // To be used by CallbackPromiseAdapter.
  using WebType = std::unique_ptr<WebServiceWorkerClientInfo>;

  static ServiceWorkerClient* Take(ScriptPromiseResolver*,
                                   std::unique_ptr<WebServiceWorkerClientInfo>);
  static ServiceWorkerClient* Create(const WebServiceWorkerClientInfo&);

  ~ServiceWorkerClient() override;

  // Client.idl
  String url() const { return url_; }
  String type() const;
  String frameType(ScriptState*) const;
  String id() const { return uuid_; }
  void postMessage(ScriptState*,
                   scoped_refptr<SerializedScriptValue> message,
                   const MessagePortArray&,
                   ExceptionState&);

  static bool CanTransferArrayBuffersAndImageBitmaps() { return false; }

 protected:
  explicit ServiceWorkerClient(const WebServiceWorkerClientInfo&);

  String Uuid() const { return uuid_; }

 private:
  String uuid_;
  String url_;
  mojom::ServiceWorkerClientType type_;
  network::mojom::RequestContextFrameType frame_type_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_CLIENT_H_
