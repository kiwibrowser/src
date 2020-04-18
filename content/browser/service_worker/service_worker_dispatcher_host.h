// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_DISPATCHER_HOST_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "content/browser/service_worker/service_worker_registration_status.h"
#include "content/common/service_worker/service_worker.mojom.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/browser/browser_associated_interface.h"
#include "content/public/browser/browser_message_filter.h"
#include "mojo/public/cpp/bindings/strong_associated_binding_set.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

class ServiceWorkerContextCore;
class ServiceWorkerContextWrapper;

namespace service_worker_dispatcher_host_unittest {
class ServiceWorkerDispatcherHostTest;
class TestingServiceWorkerDispatcherHost;
FORWARD_DECLARE_TEST(ServiceWorkerDispatcherHostTest,
                     ProviderCreatedAndDestroyed);
FORWARD_DECLARE_TEST(ServiceWorkerDispatcherHostTest, CleanupOnRendererCrash);
FORWARD_DECLARE_TEST(BackgroundSyncManagerTest,
                     RegisterWithoutLiveSWRegistration);
}  // namespace service_worker_dispatcher_host_unittest

// ServiceWorkerDispatcherHost is the browser-side endpoint for several IPC
// messages for service workers. There is a 1:1 correspondence between
// renderer processes and ServiceWorkerDispatcherHosts. Currently
// ServiceWorkerDispatcherHost sends the legacy IPC message
// ServiceWorkerMsg_ServiceWorkerStateChanged to its corresponding
// ServiceWorkerDispatcher on the renderer and receives Mojo IPC messages from
// any ServiceWorkerNetworkProvider on the renderer.
//
// ServiceWorkerDispatcherHost is created on the UI thread in
// RenderProcessHostImpl::Init() via CreateMessageFilters(), but initialization,
// destruction, and IPC message handling occur on the IO thread. It lives as
// long as the renderer process lives. Therefore much tracking of renderer
// processes in browser-side service worker code is built on
// ServiceWorkerDispatcherHost lifetime.
//
// This class is bound with mojom::ServiceWorkerDispatcherHost. All
// InterfacePtrs on the same render process are bound to the same
// content::ServiceWorkerDispatcherHost. This can be overridden only for
// testing.
//
// TODO(leonhsl): This class no longer needs to be a BrowserMessageFilter
// because we are already in a pure Mojo world.
class CONTENT_EXPORT ServiceWorkerDispatcherHost
    : public BrowserMessageFilter,
      public BrowserAssociatedInterface<mojom::ServiceWorkerDispatcherHost>,
      public mojom::ServiceWorkerDispatcherHost {
 public:
  explicit ServiceWorkerDispatcherHost(int render_process_id);

  void Init(ServiceWorkerContextWrapper* context_wrapper);

  // BrowserMessageFilter implementation
  void OnFilterRemoved() override;
  void OnDestruct() const override;
  bool OnMessageReceived(const IPC::Message& message) override;

  base::WeakPtr<ServiceWorkerDispatcherHost> AsWeakPtr();

 protected:
  ~ServiceWorkerDispatcherHost() override;

 private:
  friend class BrowserThread;
  friend class base::DeleteHelper<ServiceWorkerDispatcherHost>;
  friend class service_worker_dispatcher_host_unittest::
      ServiceWorkerDispatcherHostTest;
  friend class service_worker_dispatcher_host_unittest::
      TestingServiceWorkerDispatcherHost;
  FRIEND_TEST_ALL_PREFIXES(
      service_worker_dispatcher_host_unittest::ServiceWorkerDispatcherHostTest,
      ProviderCreatedAndDestroyed);
  FRIEND_TEST_ALL_PREFIXES(
      service_worker_dispatcher_host_unittest::ServiceWorkerDispatcherHostTest,
      CleanupOnRendererCrash);
  FRIEND_TEST_ALL_PREFIXES(
      service_worker_dispatcher_host_unittest::BackgroundSyncManagerTest,
      RegisterWithoutLiveSWRegistration);

  enum class ProviderStatus { OK, NO_CONTEXT, DEAD_HOST, NO_HOST, NO_URL };
  // Debugging for https://crbug.com/750267
  enum class Phase { kInitial, kAddedToContext, kRemovedFromContext };

  // mojom::ServiceWorkerDispatcherHost implementation
  void OnProviderCreated(ServiceWorkerProviderHostInfo info) override;

  ServiceWorkerContextCore* GetContext();

  const int render_process_id_;
  // Only accessed on the IO thread.
  Phase phase_ = Phase::kInitial;
  // Only accessed on the IO thread.
  scoped_refptr<ServiceWorkerContextWrapper> context_wrapper_;

  base::WeakPtrFactory<ServiceWorkerDispatcherHost> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_DISPATCHER_HOST_H_
