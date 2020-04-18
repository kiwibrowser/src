// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_CONTAINER_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_CONTAINER_CLIENT_H_

#include <memory>
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class ExecutionContext;
class WebServiceWorkerProvider;

// This mainly exists to provide access to WebServiceWorkerProvider.
// Owned by Document (or WorkerClients).
class MODULES_EXPORT ServiceWorkerContainerClient final
    : public GarbageCollectedFinalized<ServiceWorkerContainerClient>,
      public Supplement<Document>,
      public Supplement<WorkerClients>,
      public TraceWrapperBase {
  USING_GARBAGE_COLLECTED_MIXIN(ServiceWorkerContainerClient);
  WTF_MAKE_NONCOPYABLE(ServiceWorkerContainerClient);

 public:
  static const char kSupplementName[];

  ServiceWorkerContainerClient(Document&,
                               std::unique_ptr<WebServiceWorkerProvider>);
  ServiceWorkerContainerClient(WorkerClients&,
                               std::unique_ptr<WebServiceWorkerProvider>);
  virtual ~ServiceWorkerContainerClient();

  WebServiceWorkerProvider* Provider() { return provider_.get(); }

  static ServiceWorkerContainerClient* From(ExecutionContext*);

  void Trace(blink::Visitor* visitor) override {
    Supplement<Document>::Trace(visitor);
    Supplement<WorkerClients>::Trace(visitor);
  }

  void TraceWrappers(ScriptWrappableVisitor* visitor) const override {
    Supplement<Document>::TraceWrappers(visitor);
    Supplement<WorkerClients>::TraceWrappers(visitor);
  }

  const char* NameInHeapSnapshot() const override {
    return "ServiceWorkerContainerClient";
  }

 private:
  std::unique_ptr<WebServiceWorkerProvider> provider_;
};

MODULES_EXPORT void ProvideServiceWorkerContainerClientToWorker(
    WorkerClients*,
    std::unique_ptr<WebServiceWorkerProvider>);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_CONTAINER_CLIENT_H_
