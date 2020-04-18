// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/credential_manager_password_form_manager.h"

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/form_saver_impl.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_store.h"

using autofill::PasswordForm;

namespace password_manager {

CredentialManagerPasswordFormManager::CredentialManagerPasswordFormManager(
    PasswordManagerClient* client,
    const PasswordForm& observed_form,
    std::unique_ptr<autofill::PasswordForm> saved_form,
    CredentialManagerPasswordFormManagerDelegate* delegate,
    std::unique_ptr<FormSaver> form_saver,
    std::unique_ptr<FormFetcher> form_fetcher)
    : PasswordFormManager(client->GetPasswordManager(),
                          client,
                          nullptr,
                          observed_form,
                          (form_saver ? std::move(form_saver)
                                      : std::make_unique<FormSaverImpl>(
                                            client->GetPasswordStore())),
                          form_fetcher.get()),
      delegate_(delegate),
      saved_form_(std::move(saved_form)),
      weak_factory_(this) {
  DCHECK(saved_form_);
  // This condition is only false on iOS.
  if (form_fetcher)
    form_fetcher->Fetch();
  GrabFetcher(std::move(form_fetcher));
}

CredentialManagerPasswordFormManager::~CredentialManagerPasswordFormManager() {
}

void CredentialManagerPasswordFormManager::ProcessMatches(
    const std::vector<const PasswordForm*>& non_federated,
    size_t filtered_count) {
  PasswordFormManager::ProcessMatches(non_federated, filtered_count);

  // Mark the form as "preferred", as we've been told by the API that this is
  // indeed the credential set that the user used to sign into the site.
  saved_form_->preferred = true;
  ProvisionallySave(*saved_form_);

  // Notify the delegate. This might result in deleting |this|, while
  // ProcessMatches is being called from FormFetcherImpl, owned by |this|. If
  // done directly, once ProcessMatches returns, the FormFetcherImpl will be
  // used after free. Therefore the call is posted to a separate task.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&CredentialManagerPasswordFormManager::NotifyDelegate,
                 weak_factory_.GetWeakPtr()));
}

metrics_util::CredentialSourceType
CredentialManagerPasswordFormManager::GetCredentialSource() {
  return metrics_util::CredentialSourceType::kCredentialManagementAPI;
}

void CredentialManagerPasswordFormManager::NotifyDelegate() {
  delegate_->OnProvisionalSaveComplete();
}

}  // namespace password_manager
