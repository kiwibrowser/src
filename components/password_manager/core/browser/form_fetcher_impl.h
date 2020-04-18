// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FORM_FETCHER_IMPL_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FORM_FETCHER_IMPL_H_

#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "components/password_manager/core/browser/form_fetcher.h"
#include "components/password_manager/core/browser/http_password_store_migrator.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"
#include "components/password_manager/core/browser/suppressed_form_fetcher.h"

namespace password_manager {

class PasswordManagerClient;

// Production implementation of FormFetcher. Fetches credentials associated
// with a particular origin.
class FormFetcherImpl : public FormFetcher,
                        public PasswordStoreConsumer,
                        public HttpPasswordStoreMigrator::Consumer,
                        public SuppressedFormFetcher::Consumer {
 public:
  // |form_digest| describes what credentials need to be retrieved and
  // |client| serves the PasswordStore, the logging information etc.
  FormFetcherImpl(PasswordStore::FormDigest form_digest,
                  const PasswordManagerClient* client,
                  bool should_migrate_http_passwords,
                  bool should_query_suppressed_forms);

  ~FormFetcherImpl() override;

  // FormFetcher:
  void AddConsumer(FormFetcher::Consumer* consumer) override;
  void RemoveConsumer(FormFetcher::Consumer* consumer) override;
  State GetState() const override;
  const std::vector<InteractionsStats>& GetInteractionsStats() const override;
  const std::vector<const autofill::PasswordForm*>& GetFederatedMatches()
      const override;
  const std::vector<const autofill::PasswordForm*>& GetSuppressedHTTPSForms()
      const override;
  const std::vector<const autofill::PasswordForm*>&
  GetSuppressedPSLMatchingForms() const override;
  const std::vector<const autofill::PasswordForm*>&
  GetSuppressedSameOrganizationNameForms() const override;
  bool DidCompleteQueryingSuppressedForms() const override;
  void Fetch() override;
  std::unique_ptr<FormFetcher> Clone() override;

  // PasswordStoreConsumer:
  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) override;
  void OnGetSiteStatistics(std::vector<InteractionsStats> stats) override;

  // HttpPasswordStoreMigrator::Consumer:
  void ProcessMigratedForms(
      std::vector<std::unique_ptr<autofill::PasswordForm>> forms) override;

  // SuppressedFormFetcher::Consumer:
  void ProcessSuppressedForms(
      std::vector<std::unique_ptr<autofill::PasswordForm>> forms) override;

 private:
  // Processes password form results and forwards them to the |consumers_|.
  void ProcessPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results);

  // PasswordStore results will be fetched for this description.
  const PasswordStore::FormDigest form_digest_;

  // Results obtained from PasswordStore:
  std::vector<std::unique_ptr<autofill::PasswordForm>> non_federated_;

  // Federated credentials relevant to the observed form. They are neither
  // filled not saved by PasswordFormManager, so they are kept separately from
  // non-federated matches.
  std::vector<std::unique_ptr<autofill::PasswordForm>> federated_;

  // Statistics for the current domain.
  std::vector<InteractionsStats> interactions_stats_;

  std::vector<std::unique_ptr<autofill::PasswordForm>>
      suppressed_same_origin_https_forms_;
  std::vector<std::unique_ptr<autofill::PasswordForm>>
      suppressed_psl_matching_forms_;
  std::vector<std::unique_ptr<autofill::PasswordForm>>
      suppressed_same_organization_name_forms_;

  // Whether querying |suppressed_https_forms_| was attempted and did complete
  // at least once during the lifetime of this instance, regardless of whether
  // there have been any results.
  bool did_complete_querying_suppressed_forms_ = false;

  // Non-owning copies of the vectors above.
  std::vector<const autofill::PasswordForm*> weak_non_federated_;
  std::vector<const autofill::PasswordForm*> weak_federated_;
  std::vector<const autofill::PasswordForm*>
      weak_suppressed_same_origin_https_forms_;
  std::vector<const autofill::PasswordForm*>
      weak_suppressed_psl_matching_forms_;
  std::vector<const autofill::PasswordForm*>
      weak_suppressed_same_organization_name_forms_;

  // Consumers of the fetcher, all are assumed to outlive |this|.
  std::set<FormFetcher::Consumer*> consumers_;

  // Client used to obtain a CredentialFilter.
  const PasswordManagerClient* const client_;

  // The number of non-federated forms which were filtered out by
  // CredentialsFilter and not included in |non_federated_|.
  size_t filtered_count_ = 0;

  // State of the fetcher.
  State state_ = State::NOT_WAITING;

  // False unless FetchDataFromPasswordStore has been called again without the
  // password store returning results in the meantime.
  bool need_to_refetch_ = false;

  // Indicates whether HTTP passwords should be migrated to HTTPS.
  const bool should_migrate_http_passwords_;

  // Indicates whether to query suppressed forms.
  const bool should_query_suppressed_forms_;

  // Does the actual migration.
  std::unique_ptr<HttpPasswordStoreMigrator> http_migrator_;

  // Responsible for looking up `suppressed` credentials. These are stored
  // credentials that were not filled, even though they might be related to the
  // origin that this instance was created for. Look-up happens asynchronously,
  // without blocking Consumer::ProcessMatches.
  std::unique_ptr<SuppressedFormFetcher> suppressed_form_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(FormFetcherImpl);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_FORM_FETCHER_IMPL_H_
