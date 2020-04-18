// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"

#include "base/logging.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager_chromeos.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager_stub.h"
#include "chrome/browser/ui/ash/session_controller_client.h"
#include "components/account_id/account_id.h"
#include "components/user_manager/user_info.h"
#include "components/user_manager/user_manager.h"

namespace {
MultiUserWindowManager* g_multi_user_window_manager_instance = nullptr;
}  // namespace

// static
MultiUserWindowManager* MultiUserWindowManager::GetInstance() {
  return g_multi_user_window_manager_instance;
}

MultiUserWindowManager* MultiUserWindowManager::CreateInstance() {
  DCHECK(!g_multi_user_window_manager_instance);
  // TODO(crbug.com/557406): Enable this component in Mash. The object itself
  // has direct ash dependencies.
  if (!ash_util::IsRunningInMash() &&
      SessionControllerClient::IsMultiProfileAvailable()) {
    MultiUserWindowManagerChromeOS* manager =
        new MultiUserWindowManagerChromeOS(
            user_manager::UserManager::Get()->GetActiveUser()->GetAccountId());
    g_multi_user_window_manager_instance = manager;
    manager->Init();
  } else {
    // Use a stub when multi-profile is not available.
    g_multi_user_window_manager_instance = new MultiUserWindowManagerStub();
  }

  return g_multi_user_window_manager_instance;
}

// static
bool MultiUserWindowManager::ShouldShowAvatar(aura::Window* window) {
  // Session restore can open a window for the first user before the instance
  // is created.
  if (!g_multi_user_window_manager_instance)
    return false;

  // Show the avatar icon if the window is on a different desktop than the
  // window's owner's desktop. The stub implementation does the right thing
  // for single-user mode.
  return !g_multi_user_window_manager_instance->IsWindowOnDesktopOfUser(
      window, g_multi_user_window_manager_instance->GetWindowOwner(window));
}

// static
void MultiUserWindowManager::DeleteInstance() {
  DCHECK(g_multi_user_window_manager_instance);
  delete g_multi_user_window_manager_instance;
  g_multi_user_window_manager_instance = nullptr;
}

// static
void MultiUserWindowManager::SetInstanceForTest(
    MultiUserWindowManager* instance) {
  if (g_multi_user_window_manager_instance)
    DeleteInstance();
  g_multi_user_window_manager_instance = instance;
}
