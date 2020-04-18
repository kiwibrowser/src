// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "ash/test/ash_test_base.h"
#include "base/compiler_specific.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_context_menu.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager_chromeos.h"
#include "components/account_id/account_id.h"
#include "components/user_manager/scoped_user_manager.h"
#include "ui/aura/window.h"
#include "ui/base/models/menu_model.h"

namespace ash {

// A test class for preparing the MultiUserContextMenu.
class MultiUserContextMenuChromeOSTest : public AshTestBase {
 public:
  MultiUserContextMenuChromeOSTest()
      : multi_user_window_manager_(nullptr),
        fake_user_manager_(new chromeos::FakeChromeUserManager),
        user_manager_enabler_(base::WrapUnique(fake_user_manager_)) {}

  void SetUp() override;
  void TearDown() override;

 protected:
  // Set up the test environment for this many windows.
  void SetUpForThisManyWindows(int windows);

  // Ensures there are |n| logged-in users.
  void SetLoggedInUsers(size_t n) {
    DCHECK_LE(fake_user_manager_->GetLoggedInUsers().size(), n);
    while (fake_user_manager_->GetLoggedInUsers().size() < n) {
      AccountId account_id = AccountId::FromUserEmail(
          base::StringPrintf("generated-user-%" PRIuS "@consumer.example.com",
                             fake_user_manager_->GetLoggedInUsers().size()));
      fake_user_manager_->AddUser(account_id);
      fake_user_manager_->LoginUser(account_id);
    }
  }

  aura::Window* window() { return window_; }
  MultiUserWindowManagerChromeOS* multi_user_window_manager() {
    return multi_user_window_manager_;
  }

 private:
  // A window which can be used for testing.
  aura::Window* window_;

  // The instance of the MultiUserWindowManager.
  MultiUserWindowManagerChromeOS* multi_user_window_manager_;
  // Owned by |user_manager_enabler_|.
  chromeos::FakeChromeUserManager* fake_user_manager_ = nullptr;
  user_manager::ScopedUserManager user_manager_enabler_;

  DISALLOW_COPY_AND_ASSIGN(MultiUserContextMenuChromeOSTest);
};

void MultiUserContextMenuChromeOSTest::SetUp() {
  AshTestBase::SetUp();

  window_ = CreateTestWindowInShellWithId(0);
  window_->Show();

  multi_user_window_manager_ =
      new MultiUserWindowManagerChromeOS(AccountId::FromUserEmail("A"));
  multi_user_window_manager_->Init();
  MultiUserWindowManager::SetInstanceForTest(multi_user_window_manager_);
  EXPECT_TRUE(multi_user_window_manager_);
}

void MultiUserContextMenuChromeOSTest::TearDown() {
  delete window_;

  MultiUserWindowManager::DeleteInstance();
  AshTestBase::TearDown();
}

// Check that an unowned window will never create a menu.
TEST_F(MultiUserContextMenuChromeOSTest, UnownedWindow) {
  EXPECT_EQ(nullptr, CreateMultiUserContextMenu(window()).get());

  // Add more users.
  SetLoggedInUsers(2);
  EXPECT_EQ(nullptr, CreateMultiUserContextMenu(window()).get());
}

// Check that an owned window will never create a menu.
TEST_F(MultiUserContextMenuChromeOSTest, OwnedWindow) {
  // Make the window owned and check that there is no menu (since only a single
  // user exists).
  multi_user_window_manager()->SetWindowOwner(window(),
                                              AccountId::FromUserEmail("A"));
  EXPECT_EQ(nullptr, CreateMultiUserContextMenu(window()).get());

  // After adding another user a menu should get created.
  {
    SetLoggedInUsers(2);
    std::unique_ptr<ui::MenuModel> menu = CreateMultiUserContextMenu(window());
    ASSERT_TRUE(menu.get());
    EXPECT_EQ(1, menu.get()->GetItemCount());
  }
  {
    SetLoggedInUsers(3);
    std::unique_ptr<ui::MenuModel> menu = CreateMultiUserContextMenu(window());
    ASSERT_TRUE(menu.get());
    EXPECT_EQ(2, menu.get()->GetItemCount());
  }
}

}  // namespace ash
