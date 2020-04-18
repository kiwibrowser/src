// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/binding_state.h"

#include "mojo/public/cpp/bindings/lib/task_runner_helper.h"

namespace mojo {
namespace internal {

BindingStateBase::BindingStateBase() : weak_ptr_factory_(this) {}

BindingStateBase::~BindingStateBase() = default;

void BindingStateBase::AddFilter(std::unique_ptr<MessageReceiver> filter) {
  DCHECK(endpoint_client_);
  endpoint_client_->AddFilter(std::move(filter));
}

bool BindingStateBase::HasAssociatedInterfaces() const {
  return router_ ? router_->HasAssociatedEndpoints() : false;
}

void BindingStateBase::PauseIncomingMethodCallProcessing() {
  DCHECK(router_);
  router_->PauseIncomingMethodCallProcessing();
}

void BindingStateBase::ResumeIncomingMethodCallProcessing() {
  DCHECK(router_);
  router_->ResumeIncomingMethodCallProcessing();
}

bool BindingStateBase::WaitForIncomingMethodCall(MojoDeadline deadline) {
  DCHECK(router_);
  return router_->WaitForIncomingMessage(deadline);
}

void BindingStateBase::Close() {
  if (!router_)
    return;

  endpoint_client_.reset();
  router_->CloseMessagePipe();
  router_ = nullptr;
}

void BindingStateBase::CloseWithReason(uint32_t custom_reason,
                                       const std::string& description) {
  if (endpoint_client_)
    endpoint_client_->CloseWithReason(custom_reason, description);

  Close();
}

ReportBadMessageCallback BindingStateBase::GetBadMessageCallback() {
  return base::BindOnce(
      [](ReportBadMessageCallback inner_callback,
         base::WeakPtr<BindingStateBase> binding, const std::string& error) {
        std::move(inner_callback).Run(error);
        if (binding)
          binding->Close();
      },
      mojo::GetBadMessageCallback(), weak_ptr_factory_.GetWeakPtr());
}

void BindingStateBase::FlushForTesting() {
  endpoint_client_->FlushForTesting();
}

void BindingStateBase::EnableTestingMode() {
  DCHECK(is_bound());
  router_->EnableTestingMode();
}

scoped_refptr<internal::MultiplexRouter> BindingStateBase::RouterForTesting() {
  return router_;
}

void BindingStateBase::BindInternal(
    ScopedMessagePipeHandle handle,
    scoped_refptr<base::SingleThreadTaskRunner> runner,
    const char* interface_name,
    std::unique_ptr<MessageReceiver> request_validator,
    bool passes_associated_kinds,
    bool has_sync_methods,
    MessageReceiverWithResponderStatus* stub,
    uint32_t interface_version) {
  DCHECK(!is_bound()) << "Attempting to bind interface that is already bound: "
                      << interface_name;

  auto sequenced_runner =
      GetTaskRunnerToUseFromUserProvidedTaskRunner(std::move(runner));
  MultiplexRouter::Config config =
      passes_associated_kinds
          ? MultiplexRouter::MULTI_INTERFACE
          : (has_sync_methods
                 ? MultiplexRouter::SINGLE_INTERFACE_WITH_SYNC_METHODS
                 : MultiplexRouter::SINGLE_INTERFACE);
  router_ =
      new MultiplexRouter(std::move(handle), config, false, sequenced_runner);
  router_->SetMasterInterfaceName(interface_name);

  endpoint_client_.reset(new InterfaceEndpointClient(
      router_->CreateLocalEndpointHandle(kMasterInterfaceId), stub,
      std::move(request_validator), has_sync_methods,
      std::move(sequenced_runner), interface_version));
}

}  // namesapce internal
}  // namespace mojo
