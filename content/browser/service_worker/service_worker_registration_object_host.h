// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REGISTRATION_OBJECT_HOST_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REGISTRATION_OBJECT_HOST_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_types.h"
#include "mojo/public/cpp/bindings/associated_binding_set.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

class ServiceWorkerContextCore;
class ServiceWorkerVersion;

// ServiceWorkerRegistrationObjectHost has a 1:1 correspondence to
// WebServiceWorkerRegistration in the renderer process.
// The host stays alive while the WebServiceWorkerRegistration is alive, and
// also initiates destruction of the WebServiceWorkerRegistration once detected
// that it's no longer needed. See the class documentation in
// WebServiceWorkerRegistrationImpl for details.
//
// Has a reference to the corresponding ServiceWorkerRegistration in order to
// ensure that the registration is alive while this object host is around.
class CONTENT_EXPORT ServiceWorkerRegistrationObjectHost
    : public blink::mojom::ServiceWorkerRegistrationObjectHost,
      public ServiceWorkerRegistration::Listener {
 public:
  ServiceWorkerRegistrationObjectHost(
      base::WeakPtr<ServiceWorkerContextCore> context,
      ServiceWorkerProviderHost* provider_host,
      scoped_refptr<ServiceWorkerRegistration> registration);
  ~ServiceWorkerRegistrationObjectHost() override;

  // Establishes a new mojo connection into |bindings_|.
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr CreateObjectInfo();

  ServiceWorkerRegistration* registration() { return registration_.get(); }

 private:
  // ServiceWorkerRegistration::Listener overrides.
  void OnVersionAttributesChanged(
      ServiceWorkerRegistration* registration,
      ChangedVersionAttributesMask changed_mask,
      const ServiceWorkerRegistrationInfo& info) override;
  void OnUpdateViaCacheChanged(
      ServiceWorkerRegistration* registration) override;
  void OnRegistrationFailed(ServiceWorkerRegistration* registration) override;
  void OnUpdateFound(ServiceWorkerRegistration* registration) override;

  // Implements blink::mojom::ServiceWorkerRegistrationObjectHost.
  void Update(UpdateCallback callback) override;
  void Unregister(UnregisterCallback callback) override;
  void EnableNavigationPreload(
      bool enable,
      EnableNavigationPreloadCallback callback) override;
  void GetNavigationPreloadState(
      GetNavigationPreloadStateCallback callback) override;
  void SetNavigationPreloadHeader(
      const std::string& value,
      SetNavigationPreloadHeaderCallback callback) override;

  // Called back from ServiceWorkerContextCore when an update is complete.
  void UpdateComplete(UpdateCallback callback,
                      ServiceWorkerStatusCode status,
                      const std::string& status_message,
                      int64_t registration_id);
  // Called back from ServiceWorkerContextCore when the unregistration is
  // complete.
  void UnregistrationComplete(UnregisterCallback callback,
                              ServiceWorkerStatusCode status);
  // Called back from ServiceWorkerStorage when setting navigation preload is
  // complete.
  void DidUpdateNavigationPreloadEnabled(
      bool enable,
      EnableNavigationPreloadCallback callback,
      ServiceWorkerStatusCode status);
  // Called back from ServiceWorkerStorage when setting navigation preload
  // header is complete.
  void DidUpdateNavigationPreloadHeader(
      const std::string& value,
      SetNavigationPreloadHeaderCallback callback,
      ServiceWorkerStatusCode status);

  // Sets the corresponding version field to the given version or if the given
  // version is nullptr, clears the field.
  void SetVersionAttributes(ChangedVersionAttributesMask changed_mask,
                            ServiceWorkerVersion* installing_version,
                            ServiceWorkerVersion* waiting_version,
                            ServiceWorkerVersion* active_version);

  void OnConnectionError();

  // Perform common checks that need to run before RegistrationObjectHost
  // methods that come from a child process are handled. Returns true if all
  // checks have passed. If anything looks wrong |callback| will run with an
  // error message prefixed by |error_prefix| and |args|, and false is returned.
  template <typename CallbackType, typename... Args>
  bool CanServeRegistrationObjectHostMethods(CallbackType* callback,
                                             const char* error_prefix,
                                             Args... args);

  // |provider_host_| is valid throughout lifetime of |this| because it owns
  // |this|.
  ServiceWorkerProviderHost* provider_host_;
  base::WeakPtr<ServiceWorkerContextCore> context_;
  scoped_refptr<ServiceWorkerRegistration> registration_;

  mojo::AssociatedBindingSet<blink::mojom::ServiceWorkerRegistrationObjectHost>
      bindings_;
  // Mojo connection to the content::WebServiceWorkerRegistrationImpl in the
  // renderer, which corresponds to the ServiceWorkerRegistration JavaScript
  // object.
  blink::mojom::ServiceWorkerRegistrationObjectAssociatedPtr
      remote_registration_;

  base::WeakPtrFactory<ServiceWorkerRegistrationObjectHost> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerRegistrationObjectHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_REGISTRATION_OBJECT_HOST_H_
