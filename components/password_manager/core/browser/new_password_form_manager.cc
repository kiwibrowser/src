// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/new_password_form_manager.h"

#include "components/password_manager/core/browser/browser_save_password_progress_logger.h"
#include "components/password_manager/core/browser/form_fetcher_impl.h"
#include "components/password_manager/core/browser/form_parsing/ios_form_parser.h"
#include "components/password_manager/core/browser/password_form_filling.h"
#include "components/password_manager/core/browser/password_form_metrics_recorder.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include "components/password_manager/core/browser/password_manager_util.h"

using autofill::FormData;

using Logger = autofill::SavePasswordProgressLogger;

namespace password_manager {

namespace {

// On iOS, id are used for field identification, whereas on other platforms
// name field is used. Set id equal to name.
// TODO(https://crbug.com/831123): Remove this method when the browser side form
// parsing is ready.
FormData PreprocessFormData(const FormData& form) {
  FormData result_form = form;
  for (auto& field : result_form.fields)
    field.id = field.name;
  return result_form;
}

// Helper function for calling form parsing and logging results if logging is
// active.
std::unique_ptr<autofill::PasswordForm> ParseFormAndMakeLogging(
    PasswordManagerClient* client,
    const FormData& form) {
  // iOS form parsing is a prototype for parsing on all platforms. Call it for
  // developing and experimentation purposes.
  // TODO(https://crbug.com/831123): Call general form parsing instead of iOS
  // one when it is ready.
  std::unique_ptr<autofill::PasswordForm> password_form =
      ParseFormData(PreprocessFormData(form), FormParsingMode::FILLING);

  if (password_manager_util::IsLoggingActive(client)) {
    BrowserSavePasswordProgressLogger logger(client->GetLogManager());
    logger.LogFormData(Logger::STRING_FORM_PARSING_INPUT, form);
    if (password_form)
      logger.LogPasswordForm(Logger::STRING_FORM_PARSING_OUTPUT,
                             *password_form);
  }
  return password_form;
}

}  // namespace

NewPasswordFormManager::NewPasswordFormManager(
    PasswordManagerClient* client,
    const base::WeakPtr<PasswordManagerDriver>& driver,
    const FormData& observed_form,
    FormFetcher* form_fetcher)
    : client_(client),
      driver_(driver),
      observed_form_(observed_form),
      owned_form_fetcher_(
          form_fetcher ? nullptr
                       : std::make_unique<FormFetcherImpl>(
                             PasswordStore::FormDigest(observed_form),
                             client_,
                             true /* should_migrate_http_passwords */,
                             true /* should_query_suppressed_https_forms */)),
      form_fetcher_(form_fetcher ? form_fetcher : owned_form_fetcher_.get()) {
  metrics_recorder_ = base::MakeRefCounted<PasswordFormMetricsRecorder>(
      client_->IsMainFrameSecure(), client_->GetUkmSourceId());
  metrics_recorder_->RecordFormSignature(CalculateFormSignature(observed_form));

  if (owned_form_fetcher_)
    owned_form_fetcher_->Fetch();
  form_fetcher_->AddConsumer(this);

  // The folloing code is for development and debugging purposes.
  // TODO(https://crbug.com/831123): remove it when NewPasswordFormManager will
  // be production ready.
  if (password_manager_util::IsLoggingActive(client_))
    ParseFormAndMakeLogging(client_, observed_form_);
}
NewPasswordFormManager::~NewPasswordFormManager() = default;

bool NewPasswordFormManager::DoesManage(
    const autofill::FormData& form,
    const PasswordManagerDriver* driver) const {
  if (driver != driver_.get())
    return false;
  if (observed_form_.is_form_tag != form.is_form_tag)
    return false;
  // All unowned input elements are considered as one synthetic form.
  if (!observed_form_.is_form_tag && !form.is_form_tag)
    return true;
  return observed_form_.unique_renderer_id == form.unique_renderer_id;
}

FormFetcher* NewPasswordFormManager::GetFormFetcher() {
  return form_fetcher_;
}

const GURL& NewPasswordFormManager::GetOrigin() const {
  return observed_form_.origin;
}

const std::map<base::string16, const autofill::PasswordForm*>&
NewPasswordFormManager::GetBestMatches() const {
  // TODO(https://crbug.com/831123): Implement.
  DCHECK(false);
  static std::map<base::string16, const autofill::PasswordForm*> dummy_map;
  return dummy_map;
}
const autofill::PasswordForm& NewPasswordFormManager::GetPendingCredentials()
    const {
  // TODO(https://crbug.com/831123): Implement.
  DCHECK(false);
  static autofill::PasswordForm dummy_form;
  return dummy_form;
}
metrics_util::CredentialSourceType
NewPasswordFormManager::GetCredentialSource() {
  // TODO(https://crbug.com/831123): Implement.
  return metrics_util::CredentialSourceType::kPasswordManager;
}
PasswordFormMetricsRecorder* NewPasswordFormManager::GetMetricsRecorder() {
  return metrics_recorder_.get();
}
const std::vector<const autofill::PasswordForm*>&
NewPasswordFormManager::GetBlacklistedMatches() const {
  // TODO(https://crbug.com/831123): Implement.
  DCHECK(false);
  static std::vector<const autofill::PasswordForm*> dummy_vector;
  return dummy_vector;
}
bool NewPasswordFormManager::IsBlacklisted() const {
  // TODO(https://crbug.com/831123): Implement.
  return false;
}
bool NewPasswordFormManager::IsPasswordOverridden() const {
  // TODO(https://crbug.com/831123): Implement.
  return false;
}
const autofill::PasswordForm* NewPasswordFormManager::GetPreferredMatch()
    const {
  // TODO(https://crbug.com/831123): Implement or remove from
  // PasswordFormManagerForUI.
  return nullptr;
}

// TODO(https://crbug.com/831123): Implement all methods from
// PasswordFormManagerForUI.
void NewPasswordFormManager::Save() {}
void NewPasswordFormManager::Update(
    const autofill::PasswordForm& credentials_to_update) {}
void NewPasswordFormManager::UpdateUsername(
    const base::string16& new_username) {}
void NewPasswordFormManager::UpdatePasswordValue(
    const base::string16& new_password) {}
void NewPasswordFormManager::OnNopeUpdateClicked() {}
void NewPasswordFormManager::OnNeverClicked() {}
void NewPasswordFormManager::OnNoInteraction(bool is_update) {}
void NewPasswordFormManager::PermanentlyBlacklist() {}
void NewPasswordFormManager::OnPasswordsRevealed() {}

void NewPasswordFormManager::ProcessMatches(
    const std::vector<const autofill::PasswordForm*>& non_federated,
    size_t filtered_count) {
  if (!driver_)
    return;

  // There are additional signals (server-side data) and parse results in
  // filling and saving mode might be different so it is better not to cache
  // parse result, but to parse each time again.
  std::unique_ptr<autofill::PasswordForm> observed_password_form =
      ParseFormAndMakeLogging(client_, observed_form_);
  if (!observed_password_form)
    return;

  // TODO(https://crbug.com/831123). Implement correct treating of blacklisted
  // matches.
  std::map<base::string16, const autofill::PasswordForm*> best_matches;
  std::vector<const autofill::PasswordForm*> not_best_matches;
  const autofill::PasswordForm* preferred_match = nullptr;
  password_manager_util::FindBestMatches(non_federated, &best_matches,
                                         &not_best_matches, &preferred_match);
  if (best_matches.empty())
    return;

  // TODO(https://crbug.com/831123). Implement correct treating of federated
  // matches.
  std::vector<const autofill::PasswordForm*> federated_matches;
  SendFillInformationToRenderer(
      *client_, driver_.get(), false /* is_blaclisted */,
      *observed_password_form.get(), best_matches, federated_matches,
      preferred_match, metrics_recorder_.get());
}

bool NewPasswordFormManager::SetSubmittedFormIfIsManaged(
    const autofill::FormData& submitted_form,
    const PasswordManagerDriver* driver) {
  if (!DoesManage(submitted_form, driver))
    return false;
  submitted_form_ = submitted_form;
  is_submitted_ = true;
  return true;
}

}  // namespace password_manager
