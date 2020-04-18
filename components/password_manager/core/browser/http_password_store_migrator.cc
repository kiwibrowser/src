// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/http_password_store_migrator.h"

#include "base/memory/weak_ptr.h"
#include "base/stl_util.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_store.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace password_manager {

namespace {

// Helper method that allows us to pass WeakPtrs to |PasswordStoreConsumer|
// obtained via |GetWeakPtr|. This is not possible otherwise.
void OnHSTSQueryResultHelper(
    const base::WeakPtr<PasswordStoreConsumer>& migrator,
    bool is_hsts) {
  if (migrator) {
    static_cast<HttpPasswordStoreMigrator*>(migrator.get())
        ->OnHSTSQueryResult(is_hsts);
  }
}

}  // namespace

HttpPasswordStoreMigrator::HttpPasswordStoreMigrator(
    const GURL& https_origin,
    const PasswordManagerClient* client,
    Consumer* consumer)
    : client_(client), consumer_(consumer) {
  DCHECK(client_);
  DCHECK(https_origin.is_valid());
  DCHECK(https_origin.SchemeIs(url::kHttpsScheme)) << https_origin;

  GURL::Replacements rep;
  rep.SetSchemeStr(url::kHttpScheme);
  GURL http_origin = https_origin.ReplaceComponents(rep);
  PasswordStore::FormDigest form(autofill::PasswordForm::SCHEME_HTML,
                                 http_origin.GetOrigin().spec(), http_origin);
  http_origin_domain_ = http_origin.GetOrigin();
  client_->GetPasswordStore()->GetLogins(form, this);
  client_->PostHSTSQueryForHost(
      https_origin, base::Bind(&OnHSTSQueryResultHelper, GetWeakPtr()));
}

HttpPasswordStoreMigrator::~HttpPasswordStoreMigrator() = default;

void HttpPasswordStoreMigrator::OnGetPasswordStoreResults(
    std::vector<std::unique_ptr<autofill::PasswordForm>> results) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  results_ = std::move(results);
  got_password_store_results_ = true;

  if (got_hsts_query_result_)
    ProcessPasswordStoreResults();
}

void HttpPasswordStoreMigrator::OnHSTSQueryResult(bool is_hsts) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  mode_ = is_hsts ? MigrationMode::MOVE : MigrationMode::COPY;
  got_hsts_query_result_ = true;

  if (is_hsts)
    client_->GetPasswordStore()->RemoveSiteStats(http_origin_domain_);

  if (got_password_store_results_)
    ProcessPasswordStoreResults();
}

void HttpPasswordStoreMigrator::ProcessPasswordStoreResults() {
  // Android and PSL matches are ignored.
  base::EraseIf(
      results_, [](const std::unique_ptr<autofill::PasswordForm>& form) {
        return form->is_affiliation_based_match || form->is_public_suffix_match;
      });

  // Add the new credentials to the password store. The HTTP forms are
  // removed iff |mode_| == MigrationMode::MOVE.
  for (const auto& form : results_) {
    autofill::PasswordForm new_form = *form;

    GURL::Replacements rep;
    rep.SetSchemeStr(url::kHttpsScheme);
    new_form.origin = form->origin.ReplaceComponents(rep);
    new_form.signon_realm = new_form.origin.GetOrigin().spec();
    // If |action| is not HTTPS then it's most likely obsolete. Otherwise, it
    // may still be valid.
    if (!form->action.SchemeIs(url::kHttpsScheme))
      new_form.action = new_form.origin;
    new_form.form_data = autofill::FormData();
    new_form.generation_upload_status = autofill::PasswordForm::NO_SIGNAL_SENT;
    new_form.skip_zero_click = false;
    client_->GetPasswordStore()->AddLogin(new_form);

    if (mode_ == MigrationMode::MOVE)
      client_->GetPasswordStore()->RemoveLogin(*form);
    *form = std::move(new_form);
  }

  if (!results_.empty()) {
    // Only log data if there was at least one migrated password.
    metrics_util::LogCountHttpMigratedPasswords(results_.size());
    metrics_util::LogHttpPasswordMigrationMode(
        mode_ == MigrationMode::MOVE
            ? metrics_util::HTTP_PASSWORD_MIGRATION_MODE_MOVE
            : metrics_util::HTTP_PASSWORD_MIGRATION_MODE_COPY);
  }

  if (consumer_)
    consumer_->ProcessMigratedForms(std::move(results_));
}

}  // namespace password_manager
