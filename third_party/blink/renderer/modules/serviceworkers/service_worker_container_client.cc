// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/service_worker_container_client.h"

#include <memory>
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_provider.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

namespace blink {

ServiceWorkerContainerClient::ServiceWorkerContainerClient(
    Document& document,
    std::unique_ptr<WebServiceWorkerProvider> provider)
    : Supplement<Document>(document), provider_(std::move(provider)) {}

ServiceWorkerContainerClient::ServiceWorkerContainerClient(
    WorkerClients& clients,
    std::unique_ptr<WebServiceWorkerProvider> provider)
    : Supplement<WorkerClients>(clients), provider_(std::move(provider)) {}

ServiceWorkerContainerClient::~ServiceWorkerContainerClient() = default;

const char ServiceWorkerContainerClient::kSupplementName[] =
    "ServiceWorkerContainerClient";

ServiceWorkerContainerClient* ServiceWorkerContainerClient::From(
    ExecutionContext* context) {
  if (!context)
    return nullptr;
  if (context->IsWorkerGlobalScope()) {
    WorkerClients* worker_clients = ToWorkerGlobalScope(context)->Clients();
    DCHECK(worker_clients);
    ServiceWorkerContainerClient* client =
        Supplement<WorkerClients>::From<ServiceWorkerContainerClient>(
            worker_clients);
    DCHECK(client);
    return client;
  }
  Document* document = ToDocument(context);
  if (!document->GetFrame() || !document->GetFrame()->Client())
    return nullptr;

  ServiceWorkerContainerClient* client =
      Supplement<Document>::From<ServiceWorkerContainerClient>(document);
  if (!client) {
    client = new ServiceWorkerContainerClient(
        *document,
        document->GetFrame()->Client()->CreateServiceWorkerProvider());
    Supplement<Document>::ProvideTo(*document, client);
  }
  return client;
}

void ProvideServiceWorkerContainerClientToWorker(
    WorkerClients* clients,
    std::unique_ptr<WebServiceWorkerProvider> provider) {
  clients->ProvideSupplement(
      new ServiceWorkerContainerClient(*clients, std::move(provider)));
}

}  // namespace blink
