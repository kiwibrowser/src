// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_EXAMPLE_SESSION_CONTROLLER_CLIENT_H_
#define ASH_SHELL_EXAMPLE_SESSION_CONTROLLER_CLIENT_H_

#include "ash/session/test_session_controller_client.h"
#include "base/macros.h"

namespace ash {

class SessionController;

namespace shell {

class ExampleSessionControllerClient : public TestSessionControllerClient {
 public:
  explicit ExampleSessionControllerClient(SessionController* controller);
  ~ExampleSessionControllerClient() override;

  static ExampleSessionControllerClient* Get();

  void Initialize();

  // TestSessionControllerClient
  void RequestLockScreen() override;
  void UnlockScreen() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExampleSessionControllerClient);
};

}  // namespace shell
}  // namespace ash

#endif  // ASH_SHELL_EXAMPLE_SESSION_CONTROLLER_CLIENT_H_
