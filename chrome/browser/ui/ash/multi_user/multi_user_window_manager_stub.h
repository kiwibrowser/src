// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_MULTI_USER_MULTI_USER_WINDOW_MANAGER_STUB_H_
#define CHROME_BROWSER_UI_ASH_MULTI_USER_MULTI_USER_WINDOW_MANAGER_STUB_H_

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"

class AccountId;

// This is the implementation of MultiUserWindowManager for single user mode.
class MultiUserWindowManagerStub : public MultiUserWindowManager {
 public:
  MultiUserWindowManagerStub();
  ~MultiUserWindowManagerStub() override;

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
  void AddUser(content::BrowserContext* context) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MultiUserWindowManagerStub);
};

#endif  // CHROME_BROWSER_UI_ASH_MULTI_USER_MULTI_USER_WINDOW_MANAGER_STUB_H_
