// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/public/cpp/connector.h"

#include "services/service_manager/public/cpp/identity.h"

namespace service_manager {

////////////////////////////////////////////////////////////////////////////////
// Connector, public:

Connector::Connector(mojom::ConnectorPtrInfo unbound_state)
    : unbound_state_(std::move(unbound_state)), weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

Connector::Connector(mojom::ConnectorPtr connector)
    : connector_(std::move(connector)), weak_factory_(this) {
  connector_.set_connection_error_handler(
      base::Bind(&Connector::OnConnectionError, base::Unretained(this)));
}

Connector::~Connector() = default;

std::unique_ptr<Connector> Connector::Create(mojom::ConnectorRequest* request) {
  mojom::ConnectorPtr proxy;
  *request = mojo::MakeRequest(&proxy);
  return std::make_unique<Connector>(proxy.PassInterface());
}

void Connector::StartService(const Identity& identity) {
  if (!BindConnectorIfNecessary())
    return;

  connector_->StartService(identity,
                           base::Bind(&Connector::RunStartServiceCallback,
                                      weak_factory_.GetWeakPtr()));
}

void Connector::StartService(const std::string& name) {
  StartService(Identity(name, mojom::kInheritUserID));
}

void Connector::StartService(const Identity& identity,
                             mojom::ServicePtr service,
                             mojom::PIDReceiverRequest pid_receiver_request) {
  if (!BindConnectorIfNecessary())
    return;

  DCHECK(service.is_bound() && pid_receiver_request.is_pending());
  connector_->StartServiceWithProcess(
      identity, service.PassInterface().PassHandle(),
      std::move(pid_receiver_request),
      base::Bind(&Connector::RunStartServiceCallback,
                 weak_factory_.GetWeakPtr()));
}

void Connector::QueryService(const Identity& identity,
                             mojom::Connector::QueryServiceCallback callback) {
  if (!BindConnectorIfNecessary())
    return;

  connector_->QueryService(identity, std::move(callback));
}

void Connector::BindInterface(const Identity& target,
                              const std::string& interface_name,
                              mojo::ScopedMessagePipeHandle interface_pipe) {
  auto service_overrides_iter = local_binder_overrides_.find(target);
  if (service_overrides_iter != local_binder_overrides_.end()) {
    auto override_iter = service_overrides_iter->second.find(interface_name);
    if (override_iter != service_overrides_iter->second.end()) {
      override_iter->second.Run(std::move(interface_pipe));
      return;
    }
  }

  if (!BindConnectorIfNecessary())
    return;

  connector_->BindInterface(target, interface_name, std::move(interface_pipe),
                            base::Bind(&Connector::RunStartServiceCallback,
                                       weak_factory_.GetWeakPtr()));
}

std::unique_ptr<Connector> Connector::Clone() {
  mojom::ConnectorPtrInfo connector;
  auto request = mojo::MakeRequest(&connector);
  if (BindConnectorIfNecessary())
    connector_->Clone(std::move(request));
  return std::make_unique<Connector>(std::move(connector));
}

bool Connector::IsBound() const {
  return connector_.is_bound();
}

void Connector::FilterInterfaces(const std::string& spec,
                                 const Identity& source_identity,
                                 mojom::InterfaceProviderRequest request,
                                 mojom::InterfaceProviderPtr target) {
  if (!BindConnectorIfNecessary())
    return;
  connector_->FilterInterfaces(spec, source_identity, std::move(request),
                               std::move(target));
}

void Connector::BindConnectorRequest(mojom::ConnectorRequest request) {
  if (!BindConnectorIfNecessary())
    return;
  connector_->Clone(std::move(request));
}

base::WeakPtr<Connector> Connector::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

////////////////////////////////////////////////////////////////////////////////
// Connector, private:

void Connector::OnConnectionError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  connector_.reset();
}

void Connector::OverrideBinderForTesting(
    const service_manager::Identity& identity,
    const std::string& interface_name,
    const TestApi::Binder& binder) {
  local_binder_overrides_[identity][interface_name] = binder;
}

bool Connector::HasBinderOverride(const service_manager::Identity& identity,
                                  const std::string& interface_name) {
  auto service_overrides = local_binder_overrides_.find(identity);
  if (service_overrides == local_binder_overrides_.end())
    return false;

  return base::ContainsKey(service_overrides->second, interface_name);
}

void Connector::ClearBinderOverride(const service_manager::Identity& identity,
                                    const std::string& interface_name) {
  auto service_overrides = local_binder_overrides_.find(identity);
  if (service_overrides == local_binder_overrides_.end())
    return;

  service_overrides->second.erase(interface_name);
}

void Connector::ClearBinderOverrides() {
  local_binder_overrides_.clear();
}

void Connector::SetStartServiceCallback(
    const Connector::StartServiceCallback& callback) {
  start_service_callback_ = callback;
}

void Connector::ResetStartServiceCallback() {
  start_service_callback_.Reset();
}

bool Connector::BindConnectorIfNecessary() {
  // Bind the message pipe and SequenceChecker to the current thread the first
  // time it is used to connect.
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!connector_.is_bound()) {
    if (!unbound_state_.is_valid()) {
      // It's possible to get here when the link to the service manager has been
      // severed (and so the connector pipe has been closed) but the app has
      // chosen not to quit.
      return false;
    }

    connector_.Bind(std::move(unbound_state_));
    connector_.set_connection_error_handler(
        base::Bind(&Connector::OnConnectionError, base::Unretained(this)));
  }

  return true;
}

void Connector::RunStartServiceCallback(mojom::ConnectResult result,
                                        const Identity& user_id) {
  if (!start_service_callback_.is_null())
    start_service_callback_.Run(result, user_id);
}

}  // namespace service_manager
