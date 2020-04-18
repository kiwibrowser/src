// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_IDENTITY_SERVICE_H_
#define SERVICES_IDENTITY_IDENTITY_SERVICE_H_

#include "components/signin/core/browser/signin_manager_base.h"
#include "services/identity/public/mojom/identity_manager.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

class AccountTrackerService;
class ProfileOAuth2TokenService;

namespace identity {

class IdentityService : public service_manager::Service {
 public:
  IdentityService(AccountTrackerService* account_tracker,
                  SigninManagerBase* signin_manager,
                  ProfileOAuth2TokenService* token_service);
  ~IdentityService() override;

 private:
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  void Create(mojom::IdentityManagerRequest request);

  // Shuts down this instance, blocking it from serving any pending or future
  // requests. Safe to call multiple times; will be a no-op after the first
  // call.
  void ShutDown();
  bool IsShutDown();

  AccountTrackerService* account_tracker_;
  SigninManagerBase* signin_manager_;
  ProfileOAuth2TokenService* token_service_;

  std::unique_ptr<base::CallbackList<void()>::Subscription>
      signin_manager_shutdown_subscription_;

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(IdentityService);
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_IDENTITY_SERVICE_H_
