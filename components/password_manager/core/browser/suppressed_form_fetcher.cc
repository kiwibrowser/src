// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/suppressed_form_fetcher.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_store.h"
#include "url/gurl.h"

namespace password_manager {

SuppressedFormFetcher::SuppressedFormFetcher(
    const std::string& observed_signon_realm,
    const PasswordManagerClient* client,
    Consumer* consumer)
    : client_(client),
      consumer_(consumer),
      observed_signon_realm_(observed_signon_realm) {
  DCHECK(client_);
  DCHECK(consumer_);
  DCHECK(GURL(observed_signon_realm_).SchemeIsHTTPOrHTTPS());
  client_->GetPasswordStore()->GetLoginsForSameOrganizationName(
      observed_signon_realm_, this);
}

SuppressedFormFetcher::~SuppressedFormFetcher() = default;

void SuppressedFormFetcher::OnGetPasswordStoreResults(
    std::vector<std::unique_ptr<autofill::PasswordForm>> results) {
  base::EraseIf(results,
                [this](const std::unique_ptr<autofill::PasswordForm>& form) {
                  return form->signon_realm == observed_signon_realm_;
                });

  consumer_->ProcessSuppressedForms(std::move(results));
}

}  // namespace password_manager
