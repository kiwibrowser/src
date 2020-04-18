// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SESSION_TEST_SESSION_CONTROLLER_CLIENT_H_
#define ASH_SESSION_TEST_SESSION_CONTROLLER_CLIENT_H_

#include <stdint.h>

#include <string>

#include "ash/public/interfaces/session_controller.mojom.h"
#include "base/macros.h"
#include "components/user_manager/user_type.h"
#include "mojo/public/cpp/bindings/binding.h"

class AccountId;

namespace ash {

enum class AddUserSessionPolicy;
class SessionController;

// Implement SessionControllerClient mojo interface to simulate chrome behavior
// in tests. This breaks the ash/chrome dependency to allow testing ash code in
// isolation. Note that tests that have an instance of SessionControllerClient
// should NOT use this, i.e. tests that run BrowserMain to have chrome's
// SessionControllerClient created, e.g. InProcessBrowserTest based tests. On
// the other hand, tests code in chrome can use this class as long as it does
// not run BrowserMain, e.g. testing::Test based test.
class TestSessionControllerClient : public ash::mojom::SessionControllerClient {
 public:
  explicit TestSessionControllerClient(SessionController* controller);
  ~TestSessionControllerClient() override;

  static void DisableAutomaticallyProvideSigninPref();

  // Initialize using existing info in |controller| and bind as its client.
  void InitializeAndBind();

  // Sets up the default state of SessionController.
  void Reset();

  // Helpers to set SessionController state.
  void SetCanLockScreen(bool can_lock);
  void SetShouldLockScreenAutomatically(bool should_lock);
  void SetAddUserSessionPolicy(AddUserSessionPolicy policy);
  void SetSessionState(session_manager::SessionState state);
  void SetIsRunningInAppMode(bool app_mode);

  // Creates the |count| pre-defined user sessions. The users are named by
  // numbers using "user%d@tray" template. The first user is set as active user
  // to be consistent with crash-and-restore scenario.  Note that existing user
  // sessions prior this call will be removed without sending out notifications.
  void CreatePredefinedUserSessions(int count);

  // Adds a user session from a given display email. The display email will be
  // canonicalized and used to construct an AccountId. |enable_settings| sets
  // whether web UI settings are allowed. If |provide_pref_service| is true,
  // eagerly inject a PrefService for this user. |is_new_profile| indicates
  // whether the user has a newly created profile on the device.
  void AddUserSession(
      const std::string& display_email,
      user_manager::UserType user_type = user_manager::USER_TYPE_REGULAR,
      bool enable_settings = true,
      bool provide_pref_service = true,
      bool is_new_profile = false,
      const std::string& service_user_id = std::string());

  // Creates a test PrefService and associates it with the user.
  void ProvidePrefServiceForUser(const AccountId& account_id);

  // Simulates screen unlocking. It is virtual so that test cases can override
  // it. The default implementation sets the session state of SessionController
  // to be ACTIVE.
  virtual void UnlockScreen();

  // ash::mojom::SessionControllerClient:
  void RequestLockScreen() override;
  void RequestSignOut() override;
  void SwitchActiveUser(const AccountId& account_id) override;
  void CycleActiveUser(CycleUserDirection direction) override;
  void ShowMultiProfileLogin() override;

 private:
  SessionController* const controller_;

  // Binds to the client interface.
  mojo::Binding<ash::mojom::SessionControllerClient> binding_;

  int fake_session_id_ = 0;
  mojom::SessionInfoPtr session_info_;

  DISALLOW_COPY_AND_ASSIGN(TestSessionControllerClient);
};

}  // namespace ash

#endif  // ASH_SESSION_TEST_SESSION_CONTROLLER_CLIENT_H_
