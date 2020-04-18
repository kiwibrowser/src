// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_SYNC_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_SYNC_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/password_manager/core/browser/password_store_change.h"

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

// PasswordStore interface for PasswordSyncableService. It provides access to
// synchronous methods of PasswordStore which shouldn't be accessible to other
// classes. These methods are to be called on the PasswordStore background
// thread only.
class PasswordStoreSync {
 public:
  PasswordStoreSync();

  // Overwrites |forms| with all stored non-blacklisted credentials. Returns
  // true on success.
  virtual bool FillAutofillableLogins(
      std::vector<std::unique_ptr<autofill::PasswordForm>>* forms)
      WARN_UNUSED_RESULT = 0;

  // Overwrites |forms| with all stored blacklisted credentials. Returns true on
  // success.
  virtual bool FillBlacklistLogins(
      std::vector<std::unique_ptr<autofill::PasswordForm>>* forms)
      WARN_UNUSED_RESULT = 0;

  // Synchronous implementation to add the given login.
  virtual PasswordStoreChangeList AddLoginSync(
      const autofill::PasswordForm& form) = 0;

  // Synchronous implementation to update the given login.
  virtual PasswordStoreChangeList UpdateLoginSync(
      const autofill::PasswordForm& form) = 0;

  // Synchronous implementation to remove the given login.
  virtual PasswordStoreChangeList RemoveLoginSync(
      const autofill::PasswordForm& form) = 0;

  // Notifies observers that password store data may have been changed.
  virtual void NotifyLoginsChanged(const PasswordStoreChangeList& changes) = 0;

 protected:
  virtual ~PasswordStoreSync();

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordStoreSync);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_SYNC_H_
