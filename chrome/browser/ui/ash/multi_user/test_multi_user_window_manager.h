// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_MULTI_USER_TEST_MULTI_USER_WINDOW_MANAGER_H_
#define CHROME_BROWSER_UI_ASH_MULTI_USER_TEST_MULTI_USER_WINDOW_MANAGER_H_

#include "base/macros.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "components/account_id/account_id.h"

class Browser;

// This is a test implementation of a MultiUserWindowManager which allows to
// test a visiting window on another desktop. It will install and remove itself
// from the system upon creation / destruction.
// The creation function gets a |browser| which is shown on |desktop_owner|'s
// desktop.
class TestMultiUserWindowManager : public MultiUserWindowManager {
 public:
  TestMultiUserWindowManager(Browser* visiting_browser,
                             const AccountId& desktop_owner);
  ~TestMultiUserWindowManager() override;

  aura::Window* created_window() { return created_window_; }

  // MultiUserWindowManager overrides:
  void SetWindowOwner(aura::Window* window,
                      const AccountId& account_id) override;
  const AccountId& GetWindowOwner(aura::Window* window) const override;
  void ShowWindowForUser(aura::Window* window,
                         const AccountId& account_id) override;
  bool AreWindowsSharedAmongUsers() const override;
  void GetOwnersOfVisibleWindows(
      std::set<AccountId>* account_ids) const override;
  bool IsWindowOnDesktopOfUser(aura::Window* window,
                               const AccountId& account_id) const override;
  const AccountId& GetUserPresentingWindow(aura::Window* window) const override;
  void AddUser(content::BrowserContext* profile) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

 private:
  // The window of the visiting browser.
  aura::Window* browser_window_;
  // The owner of the visiting browser.
  AccountId browser_owner_;
  // The owner of the currently shown desktop.
  AccountId desktop_owner_;
  // The created window.
  aura::Window* created_window_;
  // The location of the window.
  AccountId created_window_shown_for_;
  // The current selected active user.
  AccountId current_account_id_;

  DISALLOW_COPY_AND_ASSIGN(TestMultiUserWindowManager);
};

#endif  // CHROME_BROWSER_UI_ASH_MULTI_USER_TEST_MULTI_USER_WINDOW_MANAGER_H_
