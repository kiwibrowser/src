// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_test_utils.h"
#include "ash/login/ui/login_big_user_view.h"
#include "base/strings/string_split.h"

namespace ash {

LockContentsView::TestApi MakeLockContentsViewTestApi(LockContentsView* view) {
  return LockContentsView::TestApi(view);
}

LoginAuthUserView::TestApi MakeLoginAuthTestApi(LockContentsView* view,
                                                AuthTarget target) {
  switch (target) {
    case AuthTarget::kPrimary:
      return LoginAuthUserView::TestApi(
          MakeLockContentsViewTestApi(view).primary_big_view()->auth_user());
    case AuthTarget::kSecondary:
      return LoginAuthUserView::TestApi(MakeLockContentsViewTestApi(view)
                                            .opt_secondary_big_view()
                                            ->auth_user());
  }

  NOTREACHED();
}

LoginPasswordView::TestApi MakeLoginPasswordTestApi(LockContentsView* view,
                                                    AuthTarget target) {
  return LoginPasswordView::TestApi(
      MakeLoginAuthTestApi(view, target).password_view());
}

mojom::LoginUserInfoPtr CreateUser(const std::string& email) {
  auto user = mojom::LoginUserInfo::New();
  user->basic_user_info = mojom::UserInfo::New();
  user->basic_user_info->avatar = mojom::UserAvatar::New();
  user->basic_user_info->account_id = AccountId::FromUserEmail(email);
  user->basic_user_info->display_name = base::SplitString(
      email, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)[0];
  user->basic_user_info->display_email = email;
  return user;
}

mojom::LoginUserInfoPtr CreatePublicAccountUser(const std::string& email) {
  auto user = mojom::LoginUserInfo::New();
  user->basic_user_info = mojom::UserInfo::New();
  std::vector<std::string> email_parts = base::SplitString(
      email, "@", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  user->basic_user_info->avatar = mojom::UserAvatar::New();
  user->basic_user_info->account_id = AccountId::FromUserEmail(email);
  user->basic_user_info->display_name = email_parts[0];
  user->basic_user_info->display_email = email;
  user->basic_user_info->type = user_manager::USER_TYPE_PUBLIC_ACCOUNT;
  user->public_account_info = ash::mojom::PublicAccountInfo::New();
  user->public_account_info->enterprise_domain = email_parts[1];
  return user;
}

}  // namespace ash
