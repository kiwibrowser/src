// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_USER_ACTION_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_USER_ACTION_H_

namespace password_manager {

// UserAction - What does the user do with a form?
enum class UserAction {
  // User did nothing, either by accepting what the password manager did, or by
  // simply not typing anything at all.
  kNone = 0,
  // There were multiple choices and the user selected a different one than the
  // default.
  kChoose,
  // The user selected an entry that originated from matching the form against
  // the Public Suffix List.
  kChoosePslMatch,
  // User typed a new value just for the password.
  kOverridePassword,
  // User typed a new value for the username and the password.
  kOverrideUsernameAndPassword,
  kMax
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_USER_ACTION_H_
