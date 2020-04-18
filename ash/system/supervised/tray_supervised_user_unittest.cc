// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/supervised/tray_supervised_user.h"

#include "ash/login_status.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "ash/test/ash_test_base.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/view.h"

using base::UTF16ToUTF8;

namespace ash {

using TraySupervisedUserTest = NoSessionAshTestBase;

// Verifies an item is created for a supervised user.
TEST_F(TraySupervisedUserTest, CreateDefaultView) {
  TraySupervisedUser* tray =
      SystemTrayTestApi(GetPrimarySystemTray()).tray_supervised_user();
  SessionController* session = Shell::Get()->session_controller();
  ASSERT_FALSE(session->IsActiveUserSessionStarted());

  // Before login there is no supervised user item.
  const LoginStatus unused = LoginStatus::NOT_LOGGED_IN;
  EXPECT_FALSE(tray->CreateDefaultView(unused));

  // Simulate a supervised user logging in.
  TestSessionControllerClient* client = GetSessionControllerClient();
  client->Reset();
  client->AddUserSession("child@test.com", user_manager::USER_TYPE_SUPERVISED);
  client->SetSessionState(session_manager::SessionState::ACTIVE);
  mojom::UserSessionPtr user_session = session->GetUserSession(0)->Clone();
  user_session->custodian_email = "parent@test.com";
  session->UpdateUserSession(std::move(user_session));

  // Now there is a supervised user item.
  std::unique_ptr<views::View> view =
      base::WrapUnique(tray->CreateDefaultView(unused));
  ASSERT_TRUE(view);
  EXPECT_EQ(
      "Usage and history of this user can be reviewed by the manager "
      "(parent@test.com) on chrome.com.",
      UTF16ToUTF8(static_cast<LabelTrayView*>(view.get())->message()));
}

}  // namespace ash
