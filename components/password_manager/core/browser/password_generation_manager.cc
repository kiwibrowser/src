// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_generation_manager.h"

#include "base/optional.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/common/password_form_generation_data.h"
#include "components/password_manager/core/browser/browser_save_password_progress_logger.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include "components/password_manager/core/browser/password_manager_util.h"

using autofill::AutofillField;
using autofill::FieldSignature;
using autofill::FormSignature;
using autofill::FormStructure;

namespace password_manager {

namespace {
using Logger = autofill::SavePasswordProgressLogger;
}

PasswordGenerationManager::PasswordGenerationManager(
    PasswordManagerClient* client,
    PasswordManagerDriver* driver)
    : client_(client), driver_(driver) {
}

PasswordGenerationManager::~PasswordGenerationManager() {
}

void PasswordGenerationManager::DetectFormsEligibleForGeneration(
    const std::vector<autofill::FormStructure*>& forms) {
  if (!IsGenerationEnabled())
    return;

  std::vector<autofill::PasswordFormGenerationData>
      forms_eligible_for_generation;
  for (const FormStructure* form : forms) {
    const AutofillField* generation_field = nullptr;
    const AutofillField* confirmation_field = nullptr;
    for (const std::unique_ptr<AutofillField>& field : *form) {
      if (field->server_type() == autofill::ACCOUNT_CREATION_PASSWORD ||
          field->server_type() == autofill::NEW_PASSWORD) {
        generation_field = field.get();
      } else if (field->server_type() == autofill::CONFIRMATION_PASSWORD) {
        confirmation_field = field.get();
      }
    }
    if (generation_field) {
      autofill::PasswordFormGenerationData data(
          form->form_signature(), generation_field->GetFieldSignature());
      if (confirmation_field != nullptr) {
        data.confirmation_field_signature.emplace(
            confirmation_field->GetFieldSignature());
      }
      forms_eligible_for_generation.push_back(data);
    }
  }
  if (!forms_eligible_for_generation.empty())
    driver_->FormsEligibleForGenerationFound(forms_eligible_for_generation);
}

// In order for password generation to be enabled, we need to make sure:
// (1) Password sync is enabled, and
// (2) Password saving is enabled.
bool PasswordGenerationManager::IsGenerationEnabled() const {
  std::unique_ptr<Logger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger.reset(
        new BrowserSavePasswordProgressLogger(client_->GetLogManager()));
  }

  if (!client_->IsSavingAndFillingEnabledForCurrentPage()) {
    if (logger)
      logger->LogMessage(Logger::STRING_GENERATION_DISABLED_SAVING_DISABLED);
    return false;
  }

  if (client_->GetPasswordSyncState() != NOT_SYNCING_PASSWORDS)
    return true;
  if (logger)
    logger->LogMessage(Logger::STRING_GENERATION_DISABLED_NO_SYNC);

  return false;
}

void PasswordGenerationManager::CheckIfFormClassifierShouldRun() {
  if (autofill::FormStructure::IsAutofillFieldMetadataEnabled())
    driver_->AllowToRunFormClassifier();
}

}  // namespace password_manager
