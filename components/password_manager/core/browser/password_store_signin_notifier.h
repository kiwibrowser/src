// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_SIGNIN_NOTIFIER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_SIGNIN_NOTIFIER_H_

#include <string>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"

namespace password_manager {

class PasswordStore;

// Abstract class for notifying PasswordStore about Chrome sign-in events.
// The logic of receiving sign-in events and notifying PasswordStore is split
// in the base abstract class (this class, in components/) and an
// implementation (in the chrome/browser/), because components/ doesn't know
// anything about Chrome sign-in.
class PasswordStoreSigninNotifier {
 public:
  PasswordStoreSigninNotifier();
  virtual ~PasswordStoreSigninNotifier();

  virtual void SubscribeToSigninEvents(PasswordStore* store) = 0;
  virtual void UnsubscribeFromSigninEvents() = 0;

 protected:
  void set_store(PasswordStore* store) { store_ = store; }

  // Passes sign-in to |store_|.
  void NotifySignin(const std::string& username, const std::string& password);

  // Passes signed-out to |store_|.
  void NotifySignedOut(const std::string& username);

 private:
  PasswordStore* store_ = nullptr;  // weak

  DISALLOW_COPY_AND_ASSIGN(PasswordStoreSigninNotifier);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_SIGNIN_NOTIFIER_H_
