// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/identity_service.h"

#include "services/identity/identity_manager_impl.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace identity {

IdentityService::IdentityService(AccountTrackerService* account_tracker,
                                 SigninManagerBase* signin_manager,
                                 ProfileOAuth2TokenService* token_service)
    : account_tracker_(account_tracker),
      signin_manager_(signin_manager),
      token_service_(token_service) {
  registry_.AddInterface<mojom::IdentityManager>(
      base::Bind(&IdentityService::Create, base::Unretained(this)));
  signin_manager_shutdown_subscription_ =
      signin_manager_->RegisterOnShutdownCallback(
          base::Bind(&IdentityService::ShutDown, base::Unretained(this)));
}

IdentityService::~IdentityService() {
  ShutDown();
}

void IdentityService::OnStart() {}

void IdentityService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void IdentityService::ShutDown() {
  if (IsShutDown())
    return;

  signin_manager_ = nullptr;
  signin_manager_shutdown_subscription_.reset();
  token_service_ = nullptr;
  account_tracker_ = nullptr;
}

bool IdentityService::IsShutDown() {
  return (signin_manager_ == nullptr);
}

void IdentityService::Create(mojom::IdentityManagerRequest request) {
  // This instance cannot service requests if it has already been shut down.
  if (IsShutDown())
    return;

  IdentityManagerImpl::Create(std::move(request), account_tracker_,
                              signin_manager_, token_service_);
}

}  // namespace identity
