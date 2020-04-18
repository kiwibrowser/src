// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_shell_content_state.h"

#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "components/account_id/account_id.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_context.h"

content::BrowserContext* ChromeShellContentState::GetBrowserContextByIndex(
    ash::UserIndex index) {
  DCHECK_LT(static_cast<size_t>(index),
            user_manager::UserManager::Get()->GetLoggedInUsers().size());
  user_manager::User* user =
      user_manager::UserManager::Get()->GetLRULoggedInUsers()[index];
  CHECK(user);
  return chromeos::ProfileHelper::Get()->GetProfileByUser(user);
}

content::BrowserContext* ChromeShellContentState::GetBrowserContextForWindow(
    aura::Window* window) {
  DCHECK(window);
  // Speculative fix for multi-profile crash. crbug.com/661821
  if (!MultiUserWindowManager::GetInstance())
    return nullptr;

  const AccountId& account_id =
      MultiUserWindowManager::GetInstance()->GetWindowOwner(window);
  return account_id.is_valid()
             ? multi_user_util::GetProfileFromAccountId(account_id)
             : nullptr;
}

content::BrowserContext*
ChromeShellContentState::GetUserPresentingBrowserContextForWindow(
    aura::Window* window) {
  DCHECK(window);
  // Speculative fix for multi-profile crash. crbug.com/661821
  if (!MultiUserWindowManager::GetInstance())
    return nullptr;

  const AccountId& account_id =
      MultiUserWindowManager::GetInstance()->GetUserPresentingWindow(window);
  return account_id.is_valid()
             ? multi_user_util::GetProfileFromAccountId(account_id)
             : nullptr;
}
