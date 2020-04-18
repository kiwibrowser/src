// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_CONNECTOR_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_CONNECTOR_H_

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/sequence_checker.h"
#include "services/service_manager/public/cpp/export.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/mojom/connector.mojom.h"
#include "services/service_manager/public/mojom/service.mojom.h"
#include "services/service_manager/public/mojom/service_manager.mojom.h"

namespace service_manager {

// An interface that encapsulates the Service Manager's brokering interface, by
// which
// connections between services are established. Once either StartService() or
// BindInterface() is called, this class is bound to the thread the call was
// made on and it cannot be passed to another thread without calling Clone().
//
// An instance of this class is created internally by ServiceContext for use
// on the thread ServiceContext is instantiated on.
//
// To use this interface on another thread, call Clone() and pass the new
// instance to the desired thread before calling StartService() or
// BindInterface().
//
// While instances of this object are owned by the caller, the underlying
// connection with the service manager is bound to the lifetime of the instance
// that created it, i.e. when the application is terminated the Connector pipe
// is closed.
class SERVICE_MANAGER_PUBLIC_CPP_EXPORT Connector {
 public:
  using StartServiceCallback =
      base::Callback<void(mojom::ConnectResult, const Identity& identity)>;

  class TestApi {
   public:
    using Binder = base::RepeatingCallback<void(mojo::ScopedMessagePipeHandle)>;
    explicit TestApi(Connector* connector) : connector_(connector) {}
    ~TestApi() { connector_->ResetStartServiceCallback(); }

    // Allows caller to specify a callback to bind requests for |interface_name|
    // from |service_name| locally, rather than passing the request through the
    // Service Manager.
    void OverrideBinderForTesting(const service_manager::Identity& identity,
                                  const std::string& interface_name,
                                  const Binder& binder) {
      connector_->OverrideBinderForTesting(identity, interface_name, binder);
    }
    bool HasBinderOverride(const service_manager::Identity& identity,
                           const std::string& interface_name) {
      return connector_->HasBinderOverride(identity, interface_name);
    }
    void ClearBinderOverride(const service_manager::Identity& identity,
                             const std::string& interface_name) {
      connector_->ClearBinderOverride(identity, interface_name);
    }
    void ClearBinderOverrides() { connector_->ClearBinderOverrides(); }

    // Register a callback to be run with the result of an attempt to start a
    // service. This will be run in response to calls to StartService() or
    // BindInterface().
    void SetStartServiceCallback(const StartServiceCallback& callback) {
      connector_->SetStartServiceCallback(callback);
    }

   private:
    Connector* connector_;
  };

  explicit Connector(mojom::ConnectorPtrInfo unbound_state);
  explicit Connector(mojom::ConnectorPtr connector);
  ~Connector();

  // Creates a new Connector instance and fills in |*request| with a request
  // for the other end the Connector's interface.
  static std::unique_ptr<Connector> Create(mojom::ConnectorRequest* request);

  // Creates an instance of a service for |identity|.
  void StartService(const Identity& identity);

  // Creates an instance of the service |name| inheriting the caller's identity.
  void StartService(const std::string& name);

  // Creates an instance of a service for |identity| in a process started by the
  // client (or someone else). Must be called before BindInterface() may be
  // called to |identity|.
  void StartService(const Identity& identity,
                    mojom::ServicePtr service,
                    mojom::PIDReceiverRequest pid_receiver_request);

  // Determines if the service for |Identity| is known, and returns information
  // about it from the catalog.
  void QueryService(const Identity& identity,
                    mojom::Connector::QueryServiceCallback callback);

  // Connect to |target| & request to bind |Interface|.
  template <typename Interface>
  void BindInterface(const Identity& target,
                     mojo::InterfacePtr<Interface>* ptr) {
    mojo::MessagePipe pipe;
    ptr->Bind(mojo::InterfacePtrInfo<Interface>(std::move(pipe.handle0), 0u));
    BindInterface(target, Interface::Name_, std::move(pipe.handle1));
  }
  template <typename Interface>
  void BindInterface(const std::string& name,
                     mojo::InterfacePtr<Interface>* ptr) {
    return BindInterface(Identity(name, mojom::kInheritUserID), ptr);
  }
  template <typename Interface>
  void BindInterface(const std::string& name,
                     mojo::InterfaceRequest<Interface> request) {
    return BindInterface(Identity(name, mojom::kInheritUserID),
                         Interface::Name_, request.PassMessagePipe());
  }
  void BindInterface(const Identity& target,
                     const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe);

  // Creates a new instance of this class which may be passed to another thread.
  // The returned object may be passed multiple times until StartService() or
  // BindInterface() is called, at which point this method must be called again
  // to pass again.
  std::unique_ptr<Connector> Clone();

  // Returns true if this Connector instance is already bound to a thread.
  bool IsBound() const;

  void FilterInterfaces(const std::string& spec,
                        const Identity& source_identity,
                        mojom::InterfaceProviderRequest request,
                        mojom::InterfaceProviderPtr target);

  // Binds a Connector request to the other end of this Connector.
  void BindConnectorRequest(mojom::ConnectorRequest request);

  base::WeakPtr<Connector> GetWeakPtr();

 private:
  using BinderOverrideMap = std::map<std::string, TestApi::Binder>;

  void OnConnectionError();

  void OverrideBinderForTesting(const service_manager::Identity& identity,
                                const std::string& interface_name,
                                const TestApi::Binder& binder);
  bool HasBinderOverride(const service_manager::Identity& identity,
                         const std::string& interface_name);
  void ClearBinderOverride(const service_manager::Identity& identity,
                           const std::string& interface_name);
  void ClearBinderOverrides();
  void SetStartServiceCallback(const StartServiceCallback& callback);
  void ResetStartServiceCallback();

  bool BindConnectorIfNecessary();

  // Callback passed to mojom methods StartService()/BindInterface().
  void RunStartServiceCallback(mojom::ConnectResult result,
                               const Identity& user_id);

  mojom::ConnectorPtrInfo unbound_state_;
  mojom::ConnectorPtr connector_;

  SEQUENCE_CHECKER(sequence_checker_);

  std::map<service_manager::Identity, BinderOverrideMap>
      local_binder_overrides_;
  StartServiceCallback start_service_callback_;

  base::WeakPtrFactory<Connector> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Connector);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_CONNECTOR_H_
