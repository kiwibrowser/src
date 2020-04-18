// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_SERVICE_CONTEXT_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_SERVICE_CONTEXT_H_

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/core.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/export.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/mojom/connector.mojom.h"
#include "services/service_manager/public/mojom/service.mojom.h"
#include "services/service_manager/public/mojom/service_control.mojom.h"

namespace service_manager {

// Encapsulates a connection to the Service Manager in two parts:
//
// - a bound InterfacePtr to mojom::Connector, the primary mechanism
//   by which the instantiating service connects to other services,
//   brokered by the Service Manager.
//
// - a bound InterfaceRequest of mojom::Service, an interface used by the
//   Service Manager to inform this service of lifecycle events and
//   inbound connections brokered by it.
//
// This class owns an instance of a Service implementation, and there should be
// exactly one instance of this class for every logical service instance running
// in the system.
//
// This class is generally used to handle incoming mojom::ServiceRequests from
// the Service Manager. These can either come from ServiceRunner, from the
// command-line (in the form of a pipe token), from a mojom::ServiceFactory
// call, or from some other embedded service-running facility defined by the
// client.
class SERVICE_MANAGER_PUBLIC_CPP_EXPORT ServiceContext : public mojom::Service {
 public:
  // Creates a new ServiceContext bound to |request|. This connection may be
  // used immediately to make outgoing connections via connector().
  //
  // This ServiceContext routes all incoming mojom::Service messages and
  // error signals to |service|, which it owns.
  //
  // If either |connector| or |connector_request| is non-null both must be
  // non-null. If both are null, the context will create its own
  // Connector and request to pass to the Service Manager on initialization.
  //
  // TODO(rockot): Clean up the connector/connector_request junk.
  ServiceContext(std::unique_ptr<service_manager::Service> service,
                 mojom::ServiceRequest request,
                 std::unique_ptr<Connector> connector = nullptr,
                 mojom::ConnectorRequest connector_request = nullptr);

  ~ServiceContext() override;

  Connector* connector() { return connector_.get(); }
  const Identity& identity() const { return identity_; }

  // Specify a closure to be run when the Service calls QuitNow(), typically
  // in response to Service::OnServiceManagerConnectionLost().
  //
  // Note that if the Service has already called QuitNow(), |closure| is run
  // immediately from this method.
  //
  // NOTE: It is acceptable for |closure| to delete this ServiceContext.
  void SetQuitClosure(const base::Closure& closure);

  // Returns a closure that, when run, informs the Service Manager that this
  // instance is ready to terminate.  If it has already terminated, then the
  // closure does nothing.
  //
  // If the Service Manager has any outstanding connection requests for this
  // instance, the request is ignored; the instance will eventually receive
  // the pending request(s) and can then appropriately decide whether or not
  // it still wants to quit.
  //
  // If the request is granted, the Service Manager will soon sever the
  // connection to this ServiceContext, and
  // Service::OnServiceManagerConnectionLost() will be invoked at that time.
  base::RepeatingClosure CreateQuitClosure();

  // Immediately severs the connection to the Service Manager.
  //
  // Note that calling this before the Service receives
  // OnServiceManagerConnectionLost() can lead to unpredictable behavior, as the
  // Service Manager may have already brokered new inbound connections from
  // other services to this Service instance, and those connections will be
  // abruptly terminated as they can no longer result in OnConnect() or
  // OnBindInterface() calls on the Service.
  //
  // To put it another way: unless you want flaky connections to be a normal
  // experience for consumers of your service, avoid calling this before
  // receiving Service::OnServiceManagerConnectionLost().
  void DisconnectFromServiceManager();

  // Immediately severs the connection to the Service Manager and invokes the
  // quit closure (see SetQuitClosure() above) if one has been set.
  //
  // See comments on DisconnectFromServiceManager() regarding abrupt
  // disconnection from the Service Manager.
  void QuitNow();

  // Overrides the interface binder for |interface_name| of |service_name|.
  // This is a process-wide override, meaning that |binder| can intercept
  // requests against only those |service_name| service instances running in the
  // same process with caller of this function.
  static void SetGlobalBinderForTesting(
      const std::string& service_name,
      const std::string& interface_name,
      const BinderRegistryWithArgs<const BindSourceInfo&>::Binder& binder,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner = nullptr);

  // Clears all overridden interface binders for service |service_name| set via
  // SetGlobalBinderForTesting().
  static void ClearGlobalBindersForTesting(const std::string& service_name);

 private:
  friend class service_manager::Service;

  void RequestQuit();

  // mojom::Service:
  void OnStart(const Identity& info, OnStartCallback callback) override;
  void OnBindInterface(const BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe,
                       OnBindInterfaceCallback callback) override;

  void OnConnectionError();

  // A pending Connector request which will eventually be passed to the Service
  // Manager.
  mojom::ConnectorRequest pending_connector_request_;

  std::unique_ptr<service_manager::Service> service_;
  mojo::Binding<mojom::Service> binding_;
  std::unique_ptr<Connector> connector_;
  service_manager::Identity identity_;

  // This instance's control interface to the service manager. Note that this
  // is unbound and therefore invalid until OnStart() is called.
  mojom::ServiceControlAssociatedPtr service_control_;

  // The closure to run when QuitNow() is invoked. May delete |this|.
  base::Closure quit_closure_;

  base::WeakPtrFactory<ServiceContext> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceContext);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_SERVICE_CONTEXT_H_
