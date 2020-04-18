// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_IMPL_H_
#define CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_IMPL_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker.h"
#include "third_party/blink/public/web/web_frame.h"

namespace blink {
class WebServiceWorkerProxy;
}

namespace content {

class ServiceWorkerProviderContext;

// WebServiceWorkerImpl represents a ServiceWorker object in JavaScript.
// https://w3c.github.io/ServiceWorker/#serviceworker-interface
//
// Only one WebServiceWorkerImpl can exist at a time to represent a given
// service worker in a given execution context. This is because the standard
// requires JavaScript equality between ServiceWorker objects in the same
// execution context that represent the same service worker.
//
// This class is ref counted and owned by WebServiceWorker::Handle, which is
// passed to Blink. Generally, Blink keeps only one Handle to an instance of
// this class. However, since //content can't know if Blink already has a
// Handle, it passes a new Handle whenever passing this class to Blink. Blink
// discards the new Handle if it already has one.
// Also, this class is referred but not owned by ServiceWorkerProviderContext
// (for service worker client contexts) and ServiceWorkerContextClient (for
// service worker execution contexts). These are tracking WebServiceWorker to
// ensure the uniqueness of ServiceWorker objects, and this class adds/removes
// the reference in the ctor/dtor.
//
// When a blink::mojom::ServiceWorkerObjectInfo arrives at the renderer,
// if there is no WebServiceWorkerImpl which represents the ServiceWorker, a
// new WebServiceWorkerImpl is created using the
// blink::mojom::ServiceWorkerObjectInfo, otherwise reuse the existing one.
//
// WebServiceWorkerImpl holds a Mojo connection (|host_|). The connection keeps
// the ServiceWorkerHandle in the browser process alive, which in turn keeps the
// relevant ServiceWorkerVersion alive.
class CONTENT_EXPORT WebServiceWorkerImpl
    : public blink::mojom::ServiceWorkerObject,
      public blink::WebServiceWorker,
      public base::RefCounted<WebServiceWorkerImpl> {
 public:
  static scoped_refptr<WebServiceWorkerImpl> CreateForServiceWorkerGlobalScope(
      blink::mojom::ServiceWorkerObjectInfoPtr info);
  static scoped_refptr<WebServiceWorkerImpl> CreateForServiceWorkerClient(
      blink::mojom::ServiceWorkerObjectInfoPtr info,
      base::WeakPtr<ServiceWorkerProviderContext> provider_context);

  // Implements blink::mojom::ServiceWorkerObject.
  void StateChanged(blink::mojom::ServiceWorkerState new_state) override;

  // blink::WebServiceWorker overrides.
  void SetProxy(blink::WebServiceWorkerProxy* proxy) override;
  blink::WebServiceWorkerProxy* Proxy() override;
  blink::WebURL Url() const override;
  blink::mojom::ServiceWorkerState GetState() const override;
  void PostMessageToServiceWorker(blink::TransferableMessage message) override;
  void TerminateForTesting(
      std::unique_ptr<TerminateForTestingCallback> callback) override;

  // Creates WebServiceWorker::Handle object that owns a reference to the given
  // WebServiceWorkerImpl object.
  static std::unique_ptr<blink::WebServiceWorker::Handle> CreateHandle(
      scoped_refptr<WebServiceWorkerImpl> worker);

 private:
  friend class base::RefCounted<WebServiceWorkerImpl>;
  WebServiceWorkerImpl(
      blink::mojom::ServiceWorkerObjectInfoPtr info,
      base::WeakPtr<ServiceWorkerProviderContext> provider_context);
  ~WebServiceWorkerImpl() override;

  // Both |host_| and |binding_| are bound on the main
  // thread for service worker clients (document), and are bound on the service
  // worker thread for service worker execution contexts.
  //
  // |host_| keeps the Mojo connection to the
  // browser-side ServiceWorkerHandle, whose lifetime is bound
  // to |host_| via the Mojo connection.
  blink::mojom::ServiceWorkerObjectHostAssociatedPtr host_;
  // |binding_| keeps the Mojo binding to serve its other Mojo endpoint (i.e.
  // the caller end) held by the content::ServiceWorkerHandle in the browser
  // process.
  mojo::AssociatedBinding<blink::mojom::ServiceWorkerObject> binding_;

  blink::mojom::ServiceWorkerObjectInfoPtr info_;
  blink::mojom::ServiceWorkerState state_;
  blink::WebServiceWorkerProxy* proxy_;

  // True means |this| is for service worker client contexts, otherwise |this|
  // is for service worker execution contexts.
  const bool is_for_client_;
  // For service worker client contexts, |this| is tracked (not owned) in
  // |context_for_client_->controllee_state_->workers_|.
  // For service worker execution contexts, |context_for_client_| is
  // null and |this| is tracked (not owned) in
  // |ServiceWorkerContextClient::ThreadSpecificInstance()->context_->workers_|.
  base::WeakPtr<ServiceWorkerProviderContext> context_for_client_;

  DISALLOW_COPY_AND_ASSIGN(WebServiceWorkerImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_IMPL_H_
