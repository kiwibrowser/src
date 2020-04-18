// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_USER_ID_USER_ID_SERVICE_H_
#define SERVICES_USER_ID_USER_ID_SERVICE_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/test/user_id/public/mojom/user_id.mojom.h"

namespace user_id {

std::unique_ptr<service_manager::Service> CreateUserIdService();

class UserIdService : public service_manager::Service, public mojom::UserId {
 public:
  UserIdService();
  ~UserIdService() override;

 private:
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // mojom::UserId:
  void GetUserId(GetUserIdCallback callback) override;

  void BindUserIdRequest(mojom::UserIdRequest request);

  service_manager::BinderRegistry registry_;
  mojo::BindingSet<mojom::UserId> bindings_;

  DISALLOW_COPY_AND_ASSIGN(UserIdService);
};

}  // namespace user_id

#endif  // SERVICES_USER_ID_USER_ID_SERVICE_H_
