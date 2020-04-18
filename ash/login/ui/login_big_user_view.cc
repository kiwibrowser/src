// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_big_user_view.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

namespace {

bool IsPublicAccountUser(const mojom::LoginUserInfoPtr& user) {
  return user->basic_user_info->type == user_manager::USER_TYPE_PUBLIC_ACCOUNT;
}

// Returns true if either a or b have a value, but not both.
bool OnlyOneSet(views::View* a, views::View* b) {
  return !!a ^ !!b;
}

}  // namespace

LoginBigUserView::LoginBigUserView(
    const mojom::LoginUserInfoPtr& user,
    const LoginAuthUserView::Callbacks& auth_user_callbacks,
    const LoginPublicAccountUserView::Callbacks& public_account_callbacks)
    : NonAccessibleView(),
      auth_user_callbacks_(auth_user_callbacks),
      public_account_callbacks_(public_account_callbacks) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  CreateChildView(user);
}

LoginBigUserView::~LoginBigUserView() = default;

void LoginBigUserView::CreateChildView(const mojom::LoginUserInfoPtr& user) {
  if (IsPublicAccountUser(user))
    CreatePublicAccount(user);
  else
    CreateAuthUser(user);
}

void LoginBigUserView::UpdateForUser(const mojom::LoginUserInfoPtr& user) {
  // Rebuild child view for the following swap case:
  // 1. Public Account -> Auth User
  // 2. Auth User      -> Public Account
  if (IsPublicAccountUser(user) != IsPublicAccountUser(GetCurrentUser()))
    CreateChildView(user);

  DCHECK(OnlyOneSet(public_account_, auth_user_));
  if (public_account_)
    public_account_->UpdateForUser(user);
  if (auth_user_)
    auth_user_->UpdateForUser(user);
}

const mojom::LoginUserInfoPtr& LoginBigUserView::GetCurrentUser() const {
  DCHECK(OnlyOneSet(public_account_, auth_user_));
  if (public_account_)
    return public_account_->current_user();
  return auth_user_->current_user();
}

LoginUserView* LoginBigUserView::GetUserView() {
  DCHECK(OnlyOneSet(public_account_, auth_user_));
  if (public_account_)
    return public_account_->user_view();
  return auth_user_->user_view();
}

bool LoginBigUserView::IsAuthEnabled() const {
  DCHECK(OnlyOneSet(public_account_, auth_user_));
  if (public_account_)
    return public_account_->auth_enabled();
  return auth_user_->auth_methods() != LoginAuthUserView::AUTH_NONE;
}

void LoginBigUserView::RequestFocus() {
  DCHECK(OnlyOneSet(public_account_, auth_user_));
  if (public_account_)
    return public_account_->RequestFocus();
  return auth_user_->RequestFocus();
}

void LoginBigUserView::CreateAuthUser(const mojom::LoginUserInfoPtr& user) {
  DCHECK(!IsPublicAccountUser(user));
  DCHECK(!auth_user_);

  auth_user_ = new LoginAuthUserView(user, auth_user_callbacks_);
  delete public_account_;
  public_account_ = nullptr;
  AddChildView(auth_user_);
}

void LoginBigUserView::CreatePublicAccount(
    const mojom::LoginUserInfoPtr& user) {
  DCHECK(IsPublicAccountUser(user));
  DCHECK(!public_account_);

  public_account_ =
      new LoginPublicAccountUserView(user, public_account_callbacks_);
  delete auth_user_;
  auth_user_ = nullptr;
  AddChildView(public_account_);
}

}  // namespace ash
