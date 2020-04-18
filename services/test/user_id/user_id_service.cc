// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/test/user_id/user_id_service.h"

#include "services/service_manager/public/cpp/service_context.h"

namespace user_id {

std::unique_ptr<service_manager::Service> CreateUserIdService() {
  return std::make_unique<UserIdService>();
}

UserIdService::UserIdService() {
  registry_.AddInterface<mojom::UserId>(
      base::Bind(&UserIdService::BindUserIdRequest, base::Unretained(this)));
}

UserIdService::~UserIdService() {}

void UserIdService::OnStart() {}

void UserIdService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void UserIdService::BindUserIdRequest(
    mojom::UserIdRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void UserIdService::GetUserId(GetUserIdCallback callback) {
  std::move(callback).Run(context()->identity().user_id());
}

}  // namespace user_id
