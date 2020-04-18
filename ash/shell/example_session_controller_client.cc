// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell/example_session_controller_client.h"

#include "ash/shell.h"
#include "ash/shell/example_factory.h"
#include "base/logging.h"

namespace ash {
namespace shell {

namespace {

ExampleSessionControllerClient* instance = nullptr;

}  // namespace

ExampleSessionControllerClient::ExampleSessionControllerClient(
    SessionController* controller)
    : TestSessionControllerClient(controller) {
  DCHECK_EQ(instance, nullptr);
  DCHECK(controller);
  instance = this;
}

ExampleSessionControllerClient::~ExampleSessionControllerClient() {
  DCHECK_EQ(instance, this);
  instance = nullptr;
}

// static
ExampleSessionControllerClient* ExampleSessionControllerClient::Get() {
  return instance;
}

void ExampleSessionControllerClient::Initialize() {
  // Initialize and bind with the session controller.
  InitializeAndBind();

  // ash_shell has 2 users.
  CreatePredefinedUserSessions(2);
}

void ExampleSessionControllerClient::RequestLockScreen() {
  TestSessionControllerClient::RequestLockScreen();
  shell::CreateLockScreen();
  Shell::Get()->UpdateShelfVisibility();
}

void ExampleSessionControllerClient::UnlockScreen() {
  TestSessionControllerClient::UnlockScreen();
  Shell::Get()->UpdateShelfVisibility();
}

}  // namespace shell
}  // namespace ash
