// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/login_model.h"

namespace password_manager {

LoginModelObserver::LoginModelObserver() {}

LoginModelObserver::~LoginModelObserver() {}

void LoginModelObserver::OnAutofillDataAvailable(
    const autofill::PasswordForm& credentials) {
  DCHECK(!signon_realm_.empty())
      << "The model did not call set_signon_realm properly.";

  if (credentials.signon_realm == signon_realm_) {
    OnAutofillDataAvailableInternal(credentials.username_value,
                                    credentials.password_value);
  }
}

LoginModel::LoginModel() {}
LoginModel::~LoginModel() {}

}  // namespace password_manager
