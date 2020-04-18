// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/user_adding_screen.h"

#include "base/bind.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_webui.h"
#include "chrome/browser/chromeos/login/ui/user_adding_screen_input_methods_controller.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "components/session_manager/core/session_manager.h"
#include "components/user_manager/user_manager.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace chromeos {

namespace {

class UserAddingScreenImpl : public UserAddingScreen {
 public:
  void Start() override;
  void Cancel() override;
  bool IsRunning() override;

  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  static UserAddingScreenImpl* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<UserAddingScreenImpl>;

  void OnDisplayHostCompletion();

  UserAddingScreenImpl();
  ~UserAddingScreenImpl() override;

  base::ObserverList<Observer> observers_;
  LoginDisplayHost* display_host_;

  UserAddingScreenInputMethodsController im_controller_;
};

void UserAddingScreenImpl::Start() {
  CHECK(!IsRunning());
  display_host_ = new chromeos::LoginDisplayHostWebUI();
  display_host_->StartUserAdding(base::BindOnce(
      &UserAddingScreenImpl::OnDisplayHostCompletion, base::Unretained(this)));

  session_manager::SessionManager::Get()->SetSessionState(
      session_manager::SessionState::LOGIN_SECONDARY);
  for (auto& observer : observers_)
    observer.OnUserAddingStarted();
}

void UserAddingScreenImpl::Cancel() {
  CHECK(IsRunning());

  display_host_->CancelUserAdding();

  // Reset wallpaper if cancel adding user from multiple user sign in page.
  if (user_manager::UserManager::Get()->IsUserLoggedIn()) {
    WallpaperControllerClient::Get()->ShowUserWallpaper(
        user_manager::UserManager::Get()->GetActiveUser()->GetAccountId());
  }
}

bool UserAddingScreenImpl::IsRunning() {
  return display_host_ != NULL;
}

void UserAddingScreenImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void UserAddingScreenImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void UserAddingScreenImpl::OnDisplayHostCompletion() {
  CHECK(IsRunning());
  display_host_ = NULL;

  session_manager::SessionManager::Get()->SetSessionState(
      session_manager::SessionState::ACTIVE);
  for (auto& observer : observers_)
    observer.OnUserAddingFinished();
}

// static
UserAddingScreenImpl* UserAddingScreenImpl::GetInstance() {
  return base::Singleton<UserAddingScreenImpl>::get();
}

UserAddingScreenImpl::UserAddingScreenImpl()
    : display_host_(NULL), im_controller_(this) {}

UserAddingScreenImpl::~UserAddingScreenImpl() {}

}  // anonymous namespace

UserAddingScreen::UserAddingScreen() {}
UserAddingScreen::~UserAddingScreen() {}

UserAddingScreen* UserAddingScreen::Get() {
  return UserAddingScreenImpl::GetInstance();
}

}  // namespace chromeos
