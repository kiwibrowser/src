// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/mock_login_screen_client.h"

#include <memory>

#include "ash/login/login_screen_controller.h"
#include "ash/shell.h"

namespace ash {

MockLoginScreenClient::MockLoginScreenClient() : binding_(this) {}

MockLoginScreenClient::~MockLoginScreenClient() = default;

mojom::LoginScreenClientPtr MockLoginScreenClient::CreateInterfacePtrAndBind() {
  mojom::LoginScreenClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void MockLoginScreenClient::AuthenticateUser(
    const AccountId& account_id,
    const std::string& password,
    bool authenticated_by_pin,
    AuthenticateUserCallback callback) {
  AuthenticateUser_(account_id, password, authenticated_by_pin, callback);
  if (authenticate_user_callback_storage_)
    *authenticate_user_callback_storage_ = std::move(callback);
  else
    std::move(callback).Run(authenticate_user_callback_result_);
}

std::unique_ptr<MockLoginScreenClient> BindMockLoginScreenClient() {
  auto client = std::make_unique<MockLoginScreenClient>();
  Shell::Get()->login_screen_controller()->SetClient(
      client->CreateInterfacePtrAndBind());
  return client;
}

}  // namespace ash
