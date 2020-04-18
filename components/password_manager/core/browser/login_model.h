// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOGIN_MODEL_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOGIN_MODEL_H_

#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/autofill/core/common/password_form.h"

namespace password_manager {

// A base class for the http-auth UI to communicate with the provider of stored
// credentials.
class LoginModelObserver {
 public:
  LoginModelObserver();

  // Called by the model when |credentials| has been identified as a match for
  // the pending login prompt. Checks that the realm matches, and passes
  // |credentials| to OnAutofillDataAvailableInternal.
  void OnAutofillDataAvailable(const autofill::PasswordForm& credentials);

  virtual void OnLoginModelDestroying() = 0;

  // To be called by the model during AddObserverAndDeliverCredentials.
  void set_signon_realm(const std::string& signon_realm) {
    signon_realm_ = signon_realm;
  }

 protected:
  virtual ~LoginModelObserver();

  virtual void OnAutofillDataAvailableInternal(
      const base::string16& username,
      const base::string16& password) = 0;

 private:
  // Signon realm for which this observer wishes to receive credentials.
  std::string signon_realm_;

  DISALLOW_COPY_AND_ASSIGN(LoginModelObserver);
};

// Corresponding Model interface to be inherited by the provider of stored
// passwords.
class LoginModel {
 public:
  LoginModel();

  // Add an observer interested in the data from the model. Also, requests that
  // the model sends a callback to |observer| with stored credentials for
  // |observed_form|.
  virtual void AddObserverAndDeliverCredentials(
      LoginModelObserver* observer,
      const autofill::PasswordForm& observed_form) = 0;

  // Remove an observer from the model.
  virtual void RemoveObserver(LoginModelObserver* observer) = 0;

 protected:
  virtual ~LoginModel();

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginModel);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_LOGIN_MODEL_H_
