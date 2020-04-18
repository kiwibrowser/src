// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIALS_FILTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIALS_FILTER_H_

#include "base/macros.h"
#include "components/autofill/core/common/password_form.h"

namespace password_manager {

class PasswordFormManager;

// This interface is used to filter credentials during saving, retrieval from
// PasswordStore, etc.
class CredentialsFilter {
 public:
  CredentialsFilter() {}
  virtual ~CredentialsFilter() {}

  // Removes from |results| all forms which should be ignored for any password
  // manager-related purposes, and returns the rest.
  virtual std::vector<std::unique_ptr<autofill::PasswordForm>> FilterResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) const = 0;

  // Should |form| be offered to be saved?
  virtual bool ShouldSave(const autofill::PasswordForm& form) const = 0;

  // Returns true if the hash of |form.password_value| should be saved for
  // password reuse checking.
  virtual bool ShouldSavePasswordHash(
      const autofill::PasswordForm& form) const = 0;

  // Call this if the form associated with |form_manager| was filled, and the
  // subsequent sign-in looked like a success.
  virtual void ReportFormLoginSuccess(
      const PasswordFormManager& form_manager) const {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CredentialsFilter);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIALS_FILTER_H_
