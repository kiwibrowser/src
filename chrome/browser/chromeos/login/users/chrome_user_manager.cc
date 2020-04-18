// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"

namespace chromeos {

ChromeUserManager::ChromeUserManager(
    scoped_refptr<base::TaskRunner> task_runner)
    : UserManagerBase(task_runner) {}

ChromeUserManager::~ChromeUserManager() {}

// static
ChromeUserManager* ChromeUserManager::Get() {
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  return user_manager ? static_cast<ChromeUserManager*>(user_manager) : NULL;
}

// static
user_manager::UserList
ChromeUserManager::GetUsersAllowedAsSupervisedUserManagers(
    const user_manager::UserList& user_list) {
  user_manager::UserList result;
  for (user_manager::User* user : user_list) {
    if (user->GetType() == user_manager::USER_TYPE_REGULAR)
      result.push_back(user);
  }
  return result;
}

}  // namespace chromeos
