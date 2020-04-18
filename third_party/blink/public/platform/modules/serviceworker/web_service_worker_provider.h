/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_PROVIDER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_SERVICE_WORKER_PROVIDER_H_

#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom-shared.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_registration.h"
#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_vector.h"

#include <memory>

namespace blink {

class WebURL;
class WebServiceWorkerProviderClient;
struct WebServiceWorkerError;

// WebServiceWorkerProvider essentially attaches to a document or worker
// execution context, and "provides" it with service worker functionality. This
// functionality is mostly the functions on ServiceWorkerContainer.idl.
//
// It is currently implemented by the Blink embedder (for Chromium it is
// content::WebServiceWorkerProviderImpl).
//
// Once an execution context has an attached WebServiceWorkerProvider, it's able
// to request the browser process to do things like register/update/get service
// worker registrations, since it'll have a |provider_id| needed to request the
// browser process to do these things. The |provider_id| actually comes from
// WebServiceWorkerNetworkProvider, but content::WebServiceWorkerProviderImpl
// has access to the same id. (This id should hopefully go away once everything
// is mojofied/servicified.)
//
// WebServiceWorkerProvider is created in two places:
// 1) It is created in ServiceWorkerContainerClient::From(), which is used to
// instantiate navigator.serviceWorker for documents.
// 2) It is created for service worker execution contexts during startup.
// The instance should eventually be used for WorkerNavigator.serviceWorker but
// it is not really wired up yet (https://crbug.com/371690). Instead, the
// instance is used to implement ServiceWorkerRegistration#update() and
// ServiceWorkerRegistration#unregister(), just because
// WebServiceWorkerRegistration's corresponding methods need |provider_id| to
// send the requests to the browser process, and this class is used as the
// wrapper object that hides the ID details from Blink.
//
// WebServiceWorkerProvider is owned by ServiceWorkerContainerClient, which is a
// garbage collected Supplement for Document (in case (1) above) or
// WorkerClients (in case (2) above).
//
// Each ServiceWorkerContainer instance has a WebServiceWorkerProvider.
// ServiceWorkerContainer is called the "client" of the
// WebServiceWorkerProvider, and it implements WebServiceWorkerProviderClient.
class WebServiceWorkerProvider {
 public:
  // Sets the "client" for this provider. The client will be notified of
  // controller changes, message events, and feature usages apropos of the
  // document this WebServiceWorkerProvider is for. It's not used when this
  // WebServiceWorkerProvider is for a service worker context.
  virtual void SetClient(WebServiceWorkerProviderClient*) {}

  using WebServiceWorkerRegistrationCallbacks =
      WebCallbacks<std::unique_ptr<WebServiceWorkerRegistration::Handle>,
                   const WebServiceWorkerError&>;
  using WebServiceWorkerGetRegistrationCallbacks =
      WebCallbacks<std::unique_ptr<WebServiceWorkerRegistration::Handle>,
                   const WebServiceWorkerError&>;

  using WebServiceWorkerRegistrationHandles =
      WebVector<std::unique_ptr<WebServiceWorkerRegistration::Handle>>;
  using WebServiceWorkerGetRegistrationsCallbacks =
      WebCallbacks<std::unique_ptr<WebServiceWorkerRegistrationHandles>,
                   const WebServiceWorkerError&>;
  using WebServiceWorkerGetRegistrationForReadyCallbacks =
      WebCallbacks<std::unique_ptr<WebServiceWorkerRegistration::Handle>, void>;

  // For ServiceWorkerContainer#register(). Requests the embedder to register a
  // service worker.
  // TODO(yuryu): Use the blink::mojom::RegistrationOptions type after Onion
  // Soup.
  virtual void RegisterServiceWorker(
      const WebURL& pattern,
      const WebURL& script_url,
      blink::mojom::ServiceWorkerUpdateViaCache update_via_cache,
      std::unique_ptr<WebServiceWorkerRegistrationCallbacks>) {}
  // For ServiceWorkerContainer#getRegistration(). Requests the embedder to
  // return a registration.
  virtual void GetRegistration(
      const WebURL& document_url,
      std::unique_ptr<WebServiceWorkerGetRegistrationCallbacks>) {}
  // For ServiceWorkerContainer#getRegistrations(). Requests the embedder to
  // return matching registrations.
  virtual void GetRegistrations(
      std::unique_ptr<WebServiceWorkerGetRegistrationsCallbacks>) {}
  // For ServiceWorkerContainer#ready. Requests the embedder to return the
  // ready registration.
  virtual void GetRegistrationForReady(
      std::unique_ptr<WebServiceWorkerGetRegistrationForReadyCallbacks>) {}
  // Helper function for checking URLs. The |scope| and |script_url| cannot
  // include escape sequences for "/" or "\" as per spec, as they would break
  // would the path restriction.
  virtual bool ValidateScopeAndScriptURL(const WebURL& scope,
                                         const WebURL& script_url,
                                         WebString* error_message) {
    return false;
  }

  virtual ~WebServiceWorkerProvider() = default;
};

}  // namespace blink

#endif
