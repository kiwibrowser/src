// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_REGISTRATION_IMPL_H_
#define CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_REGISTRATION_IMPL_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_error_type.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_registration.h"

namespace blink {
class WebServiceWorkerRegistrationProxy;
}

namespace content {

class WebServiceWorkerImpl;
class ServiceWorkerProviderContext;

// WebServiceWorkerRegistrationImpl represents a ServiceWorkerRegistration
// object in JavaScript.
// https://w3c.github.io/ServiceWorker/#serviceworkerregistration-interface
//
// Only one WebServiceWorkerRegistrationImpl can exist at a time to represent a
// given service worker registration in a given execution context. This is
// because the standard requires JavaScript equality between
// ServiceWorkerRegistration objects in the same execution context that
// represent the same service worker registration.
//
// This class is ref counted and owned by WebServiceWorkerRegistration::Handle,
// which is passed to Blink. Generally, Blink keeps only one Handle to an
// instance of this class. However, since //content can't know if Blink already
// has a Handle, it passes a new Handle whenever passing this class to Blink.
// Blink discards the new Handle if it already has one.
//
// When a blink::mojom::ServiceWorkerRegistrationObjectInfo arrives at the
// renderer, there are two ways to handle it.
// 1) If there is no WebServiceWorkerRegistrationImpl which represents the
// ServiceWorkerRegistration, a new WebServiceWorkerRegistrationImpl is
// created using the blink::mojom::ServiceWorkerRegistrationObjectInfo.
// 2) If there is a WebServiceWorkerRegistrationImpl which represents the
// ServiceWorkerRegistration, the WebServiceWorkerRegistrationImpl starts to use
// the new browser->renderer connection
// (blink::mojom::ServiceWorkerRegistrationObject interface) and the information
// about the registration.
//
// WebServiceWorkerRegistrationImpl holds a Mojo connection (|host_|). The
// connection keeps the ServiceWorkerRegistrationObjectHost in the browser
// process alive, which in turn keeps the relevant
// content::ServiceWorkerRegistration alive.
class CONTENT_EXPORT WebServiceWorkerRegistrationImpl
    : public blink::mojom::ServiceWorkerRegistrationObject,
      public blink::WebServiceWorkerRegistration,
      public base::RefCounted<WebServiceWorkerRegistrationImpl> {
 public:
  static scoped_refptr<WebServiceWorkerRegistrationImpl>
  CreateForServiceWorkerGlobalScope(
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info);
  static scoped_refptr<WebServiceWorkerRegistrationImpl>
  CreateForServiceWorkerClient(
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info,
      base::WeakPtr<ServiceWorkerProviderContext> provider_context);

  // Called when the browser process sends a new
  // blink::mojom::ServiceWorkerRegistrationObjectInfo and |this| already exists
  // for the described ServiceWorkerRegistration (see the class comment).
  void AttachForServiceWorkerClient(
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info);

  // blink::WebServiceWorkerRegistration overrides.
  void SetProxy(blink::WebServiceWorkerRegistrationProxy* proxy) override;
  blink::WebServiceWorkerRegistrationProxy* Proxy() override;
  blink::WebURL Scope() const override;
  blink::mojom::ServiceWorkerUpdateViaCache UpdateViaCache() const override;
  void Update(
      std::unique_ptr<WebServiceWorkerUpdateCallbacks> callbacks) override;
  void Unregister(std::unique_ptr<WebServiceWorkerUnregistrationCallbacks>
                      callbacks) override;
  void EnableNavigationPreload(
      bool enable,
      std::unique_ptr<WebEnableNavigationPreloadCallbacks> callbacks) override;
  void GetNavigationPreloadState(
      std::unique_ptr<WebGetNavigationPreloadStateCallbacks> callbacks)
      override;
  void SetNavigationPreloadHeader(
      const blink::WebString& value,
      std::unique_ptr<WebSetNavigationPreloadHeaderCallbacks> callbacks)
      override;
  int64_t RegistrationId() const override;

  // Creates blink::WebServiceWorkerRegistration::Handle object that owns a
  // reference to the given WebServiceWorkerRegistrationImpl object.
  static std::unique_ptr<blink::WebServiceWorkerRegistration::Handle>
  CreateHandle(scoped_refptr<WebServiceWorkerRegistrationImpl> registration);

 private:
  friend class base::RefCounted<WebServiceWorkerRegistrationImpl>;
  WebServiceWorkerRegistrationImpl(
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info,
      base::WeakPtr<ServiceWorkerProviderContext> provider_context);
  ~WebServiceWorkerRegistrationImpl() override;

  void SetInstalling(blink::mojom::ServiceWorkerObjectInfoPtr info);
  void SetWaiting(blink::mojom::ServiceWorkerObjectInfoPtr info);
  void SetActive(blink::mojom::ServiceWorkerObjectInfoPtr info);
  // Refreshes the JavaScript ServiceWorkerRegistration object (|proxy_|) with
  // the {installing,waiting,active} service worker object infos from |info_|.
  void RefreshVersionAttributes();

  scoped_refptr<WebServiceWorkerImpl> GetOrCreateServiceWorkerObject(
      blink::mojom::ServiceWorkerObjectInfoPtr info);

  // Implements blink::mojom::ServiceWorkerRegistrationObject.
  void SetVersionAttributes(
      int changed_mask,
      blink::mojom::ServiceWorkerObjectInfoPtr installing,
      blink::mojom::ServiceWorkerObjectInfoPtr waiting,
      blink::mojom::ServiceWorkerObjectInfoPtr active) override;
  void SetUpdateViaCache(
      blink::mojom::ServiceWorkerUpdateViaCache update_via_cache) override;
  void UpdateFound() override;

  enum QueuedTaskType {
    INSTALLING,
    WAITING,
    ACTIVE,
    UPDATE_FOUND,
  };

  struct QueuedTask {
    QueuedTask(QueuedTaskType type,
               const scoped_refptr<WebServiceWorkerImpl>& worker);
    QueuedTask(const QueuedTask& other);
    ~QueuedTask();
    QueuedTaskType type;
    scoped_refptr<WebServiceWorkerImpl> worker;
  };

  void RunQueuedTasks();
  void Attach(blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info);
  void DetachAndMaybeDestroy();
  void OnConnectionError();

  // |info_| is initialized by the contructor with |info| passed from the remote
  // content::ServiceWorkerRegistrationObjectHost just created in the browser
  // process, and may be updated by AttachForServiceWorkerClient().
  // |info_->host_ptr_info| is taken/bound by |host_|.
  // |info_->request| is bound on |binding_|.
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info_;
  blink::WebServiceWorkerRegistrationProxy* proxy_;

  // |host_| keeps the Mojo connection to the
  // browser-side ServiceWorkerRegistrationObjectHost, whose lifetime is bound
  // to the Mojo connection. It is bound on the
  // main thread for service worker clients (document), and is bound on the
  // service worker thread for service worker execution contexts.
  blink::mojom::ServiceWorkerRegistrationObjectHostAssociatedPtr host_;
  // |binding_| keeps the Mojo binding to serve its other Mojo endpoint (i.e.
  // the caller end) held by the ServiceWorkerRegistrationObjectHost in
  // the browser process.
  // It is bound on the main thread for service
  // worker clients (document), and is bound on the service worker thread for
  // service worker execution contexts.
  mojo::AssociatedBinding<blink::mojom::ServiceWorkerRegistrationObject>
      binding_;

  std::vector<QueuedTask> queued_tasks_;

  // True means |this| is for service worker client contexts, otherwise |this|
  // is for service worker execution contexts.
  const bool is_for_client_;
  // For service worker client contexts, |this| is tracked (not owned) in
  // |provider_context_for_client_->controllee_state_->registrations_|.
  // For service worker execution contexts, |provider_context_for_client_| is
  // null.
  base::WeakPtr<ServiceWorkerProviderContext> provider_context_for_client_;

  DISALLOW_COPY_AND_ASSIGN(WebServiceWorkerRegistrationImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_WEB_SERVICE_WORKER_REGISTRATION_IMPL_H_
