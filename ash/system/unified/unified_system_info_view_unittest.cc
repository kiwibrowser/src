// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/unified_system_info_view.h"

#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/system/model/enterprise_domain_model.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/test/ash_test_base.h"

namespace ash {

class UnifiedSystemInfoViewTest : public AshTestBase {
 public:
  UnifiedSystemInfoViewTest() = default;
  ~UnifiedSystemInfoViewTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();
    info_view_ = std::make_unique<UnifiedSystemInfoView>();
  }

  void TearDown() override {
    info_view_.reset();
    AshTestBase::TearDown();
  }

 protected:
  UnifiedSystemInfoView* info_view() { return info_view_.get(); }

 private:
  std::unique_ptr<UnifiedSystemInfoView> info_view_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemInfoViewTest);
};

TEST_F(UnifiedSystemInfoViewTest, EnterpriseManagedVisible) {
  // By default, EnterpriseManagedView is not shown.
  EXPECT_FALSE(info_view()->enterprise_managed_->visible());

  // Simulate enterprise information becoming available.
  const bool active_directory = false;
  Shell::Get()
      ->system_tray_model()
      ->enterprise_domain()
      ->SetEnterpriseDisplayDomain("example.com", active_directory);

  // EnterpriseManagedView should be shown.
  EXPECT_TRUE(info_view()->enterprise_managed_->visible());
}

TEST_F(UnifiedSystemInfoViewTest, EnterpriseManagedVisibleForActiveDirectory) {
  // Active directory information becoming available.
  const std::string empty_domain;
  const bool active_directory = true;
  Shell::Get()
      ->system_tray_model()
      ->enterprise_domain()
      ->SetEnterpriseDisplayDomain(empty_domain, active_directory);

  // EnterpriseManagedView should be shown.
  EXPECT_TRUE(info_view()->enterprise_managed_->visible());
}

using UnifiedSystemInfoViewNoSessionTest = NoSessionAshTestBase;

TEST_F(UnifiedSystemInfoViewNoSessionTest, SupervisedVisible) {
  std::unique_ptr<UnifiedSystemInfoView> info_view_;

  SessionController* session = Shell::Get()->session_controller();
  ASSERT_FALSE(session->IsActiveUserSessionStarted());

  // Before login the supervised user view is invisible.
  info_view_ = std::make_unique<UnifiedSystemInfoView>();
  EXPECT_FALSE(info_view_->supervised_->visible());
  info_view_.reset();

  // Simulate a supervised user logging in.
  TestSessionControllerClient* client = GetSessionControllerClient();
  client->Reset();
  client->AddUserSession("child@test.com", user_manager::USER_TYPE_SUPERVISED);
  client->SetSessionState(session_manager::SessionState::ACTIVE);
  mojom::UserSessionPtr user_session = session->GetUserSession(0)->Clone();
  user_session->custodian_email = "parent@test.com";
  session->UpdateUserSession(std::move(user_session));

  // Now the supervised user view is visible.
  info_view_ = std::make_unique<UnifiedSystemInfoView>();
  ASSERT_TRUE(info_view_->supervised_->visible());
}

}  // namespace ash
