// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_TEST_UTILS_H_
#define ASH_LOGIN_UI_LOGIN_TEST_UTILS_H_

#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/login_auth_user_view.h"
#include "ash/login/ui/login_password_view.h"
#include "ash/public/interfaces/login_user_info.mojom.h"

namespace ash {

enum class AuthTarget { kPrimary, kSecondary };

// Helpers for constructing TestApi instances.
LockContentsView::TestApi MakeLockContentsViewTestApi(LockContentsView* view);
LoginAuthUserView::TestApi MakeLoginAuthTestApi(LockContentsView* view,
                                                AuthTarget auth);
LoginPasswordView::TestApi MakeLoginPasswordTestApi(LockContentsView* view,
                                                    AuthTarget auth);

// Utility method to create a new |mojom::LoginUserInfoPtr| instance
// for regular user.
mojom::LoginUserInfoPtr CreateUser(const std::string& email);

// Utility method to create a new |mojom::LoginUserInfoPtr| instance for
// public account user.
mojom::LoginUserInfoPtr CreatePublicAccountUser(const std::string& email);

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_TEST_UTILS_H_
