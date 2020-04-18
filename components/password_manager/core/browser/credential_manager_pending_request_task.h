// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIAL_MANAGER_PENDING_REQUEST_TASK_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIAL_MANAGER_PENDING_REQUEST_TASK_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "components/password_manager/core/browser/http_password_store_migrator.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"
#include "components/password_manager/core/common/credential_manager_types.h"
#include "url/gurl.h"

namespace autofill {
struct PasswordForm;
}  // namespace autofill

namespace password_manager {

struct CredentialInfo;
class PasswordManagerClient;

typedef base::Callback<void(const CredentialInfo& credential)>
    SendCredentialCallback;

// Sends credentials retrieved from the PasswordStore to CredentialManager API
// clients and retrieves embedder-dependent information.
class CredentialManagerPendingRequestTaskDelegate {
 public:
  // Determines whether zero-click sign-in is allowed.
  virtual bool IsZeroClickAllowed() const = 0;

  // Retrieves the current page origin.
  virtual GURL GetOrigin() const = 0;

  // Returns the PasswordManagerClient.
  virtual PasswordManagerClient* client() const = 0;

  // Sends a credential to JavaScript.
  virtual void SendCredential(const SendCredentialCallback& send_callback,
                              const CredentialInfo& credential) = 0;

  // Updates |skip_zero_click| for |form| in the PasswordStore if required.
  // Sends a credential to JavaScript.
  virtual void SendPasswordForm(const SendCredentialCallback& send_callback,
                                CredentialMediationRequirement mediation,
                                const autofill::PasswordForm* form) = 0;
};

// Retrieves credentials from the PasswordStore.
class CredentialManagerPendingRequestTask
    : public PasswordStoreConsumer,
      public HttpPasswordStoreMigrator::Consumer {
 public:
  CredentialManagerPendingRequestTask(
      CredentialManagerPendingRequestTaskDelegate* delegate,
      const SendCredentialCallback& callback,
      CredentialMediationRequirement mediation,
      bool include_passwords,
      const std::vector<GURL>& request_federations);
  ~CredentialManagerPendingRequestTask() override;

  SendCredentialCallback send_callback() const { return send_callback_; }
  const GURL& origin() const { return origin_; }

  // PasswordStoreConsumer:
  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) override;

 private:
  // HttpPasswordStoreMigrator::Consumer:
  void ProcessMigratedForms(
      std::vector<std::unique_ptr<autofill::PasswordForm>> forms) override;

  void ProcessForms(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results);

  CredentialManagerPendingRequestTaskDelegate* delegate_;  // Weak;
  SendCredentialCallback send_callback_;
  const CredentialMediationRequirement mediation_;
  const GURL origin_;
  const bool include_passwords_;
  std::set<std::string> federations_;

  std::unique_ptr<HttpPasswordStoreMigrator> http_migrator_;

  DISALLOW_COPY_AND_ASSIGN(CredentialManagerPendingRequestTask);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_CREDENTIAL_MANAGER_PENDING_REQUEST_TASK_H_
