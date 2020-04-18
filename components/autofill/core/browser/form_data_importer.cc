// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/form_data_importer.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/credit_card_save_manager.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/phone_number.h"
#include "components/autofill/core/browser/phone_number_i18n.h"
#include "components/autofill/core/browser/validation.h"
#include "components/autofill/core/common/autofill_util.h"

namespace autofill {

namespace {

// Return true if the |field_type| and |value| are valid within the context
// of importing a form.
bool IsValidFieldTypeAndValue(const std::set<ServerFieldType>& types_seen,
                              ServerFieldType field_type,
                              const base::string16& value) {
  // Abandon the import if two fields of the same type are encountered.
  // This indicates ambiguous data or miscategorization of types.
  // Make an exception for PHONE_HOME_NUMBER however as both prefix and
  // suffix are stored against this type, and for EMAIL_ADDRESS because it is
  // common to see second 'confirm email address' fields on forms.
  if (types_seen.count(field_type) && field_type != PHONE_HOME_NUMBER &&
      field_type != EMAIL_ADDRESS)
    return false;

  // Abandon the import if an email address value shows up in a field that is
  // not an email address.
  if (field_type != EMAIL_ADDRESS && IsValidEmailAddress(value))
    return false;

  return true;
}

// Returns true if minimum requirements for import of a given |profile| have
// been met.  An address submitted via a form must have at least the fields
// required as determined by its country code.
// No verification of validity of the contents is performed. This is an
// existence check only.
bool IsMinimumAddress(const AutofillProfile& profile,
                      const std::string& app_locale) {
  // All countries require at least one address line.
  if (profile.GetRawInfo(ADDRESS_HOME_LINE1).empty())
    return false;

  std::string country_code =
      base::UTF16ToASCII(profile.GetRawInfo(ADDRESS_HOME_COUNTRY));
  if (country_code.empty())
    country_code = AutofillCountry::CountryCodeForLocale(app_locale);

  AutofillCountry country(country_code, app_locale);

  if (country.requires_city() && profile.GetRawInfo(ADDRESS_HOME_CITY).empty())
    return false;

  if (country.requires_state() &&
      profile.GetRawInfo(ADDRESS_HOME_STATE).empty())
    return false;

  if (country.requires_zip() && profile.GetRawInfo(ADDRESS_HOME_ZIP).empty())
    return false;

  return true;
}

}  // namespace

FormDataImporter::FormDataImporter(AutofillClient* client,
                                   payments::PaymentsClient* payments_client,
                                   PersonalDataManager* personal_data_manager,
                                   const std::string& app_locale)
    : credit_card_save_manager_(
          std::make_unique<CreditCardSaveManager>(client,
                                                  payments_client,
                                                  app_locale,
                                                  personal_data_manager)),
      personal_data_manager_(personal_data_manager),
      app_locale_(app_locale) {}

FormDataImporter::~FormDataImporter() {}

void FormDataImporter::ImportFormData(const FormStructure& submitted_form,
                                      bool credit_card_autofill_enabled) {
  std::unique_ptr<CreditCard> imported_credit_card;
  if (!ImportFormData(submitted_form, credit_card_autofill_enabled,
                      credit_card_save_manager_->IsCreditCardUploadEnabled(),
                      &imported_credit_card))
    return;

  // No card available to offer save or upload.
  if (!imported_credit_card)
    return;

  if (!credit_card_save_manager_->IsCreditCardUploadEnabled()) {
    // Offer local save.
    credit_card_save_manager_->OfferCardLocalSave(*imported_credit_card);
  } else {
    // Attempt to offer upload save. Because we pass IsCreditCardUploadEnabled()
    // to ImportFormData, this block can be reached on observing either a new
    // card or one already stored locally and whose |TypeAndLastFourDigits| do
    // not match a masked server card. We can offer to upload either kind, but
    // note that unless the "send detected values" experiment is enabled, they
    // must pass address/name/CVC validation requirements first.
    credit_card_save_manager_->AttemptToOfferCardUploadSave(
        submitted_form, *imported_credit_card,
        offering_upload_of_local_credit_card_);
  }
}

CreditCard FormDataImporter::ExtractCreditCardFromForm(
    const FormStructure& form) {
  bool has_duplicate_field_type;
  return ExtractCreditCardFromForm(form, &has_duplicate_field_type);
}

// static
bool FormDataImporter::IsValidLearnableProfile(const AutofillProfile& profile,
                                               const std::string& app_locale) {
  if (!IsMinimumAddress(profile, app_locale))
    return false;

  base::string16 email = profile.GetRawInfo(EMAIL_ADDRESS);
  if (!email.empty() && !IsValidEmailAddress(email))
    return false;

  // Reject profiles with invalid US state information.
  if (profile.IsPresentButInvalid(ADDRESS_HOME_STATE))
    return false;

  // Reject profiles with invalid US zip information.
  if (profile.IsPresentButInvalid(ADDRESS_HOME_ZIP))
    return false;

  return true;
}

bool FormDataImporter::ImportFormData(
    const FormStructure& submitted_form,
    bool credit_card_autofill_enabled,
    bool should_return_local_card,
    std::unique_ptr<CreditCard>* imported_credit_card) {
  // We try the same |form| for both credit card and address import/update.
  // - ImportCreditCard may update an existing card, or fill
  //   |imported_credit_card| with an extracted card. See .h for details of
  //   |should_return_local_card|.
  bool cc_import = false;
  if (credit_card_autofill_enabled) {
    cc_import = ImportCreditCard(submitted_form, should_return_local_card,
                                 imported_credit_card);
  }
  // - ImportAddressProfiles may eventually save or update one or more address
  //   profiles.
  bool address_import = ImportAddressProfiles(submitted_form);
  if (cc_import || address_import)
    return true;

  personal_data_manager_->MarkObserversInsufficientFormDataForImport();
  return false;
}

bool FormDataImporter::ImportAddressProfiles(const FormStructure& form) {
  if (!form.field_count())
    return false;

  // Relevant sections for address fields.
  std::set<std::string> sections;
  for (const auto& field : form) {
    if (field->Type().group() != CREDIT_CARD)
      sections.insert(field->section);
  }

  // We save a maximum of 2 profiles per submitted form (e.g. for shipping and
  // billing).
  static const size_t kMaxNumAddressProfilesSaved = 2;
  size_t num_saved_profiles = 0;
  for (const std::string& section : sections) {
    if (num_saved_profiles == kMaxNumAddressProfilesSaved)
      break;

    if (ImportAddressProfileForSection(form, section))
      num_saved_profiles++;
  }

  return num_saved_profiles > 0;
}

bool FormDataImporter::ImportAddressProfileForSection(
    const FormStructure& form,
    const std::string& section) {
  // The candidate for profile import. There are many ways for the candidate to
  // be rejected (see everywhere this function returns false).
  AutofillProfile candidate_profile;
  candidate_profile.set_origin(form.source_url().spec());

  // We only set complete phone, so aggregate phone parts in these vars and set
  // complete at the end.
  PhoneNumber::PhoneCombineHelper combined_phone;

  // Used to detect and discard address forms with multiple fields of the same
  // type.
  std::set<ServerFieldType> types_seen;

  // Go through each |form| field and attempt to constitute a valid profile.
  for (const auto& field : form) {
    // Reject fields that are not within the specified |section|.
    if (field->section != section)
      continue;

    base::string16 value;
    base::TrimWhitespace(field->value, base::TRIM_ALL, &value);

    // If we don't know the type of the field, or the user hasn't entered any
    // information into the field, or the field is non-focusable (hidden), then
    // skip it.
    if (!field->IsFieldFillable() || !field->is_focusable || value.empty())
      continue;

    AutofillType field_type = field->Type();

    // Credit card fields are handled by ImportCreditCard().
    if (field_type.group() == CREDIT_CARD)
      continue;

    // There can be multiple email fields (e.g. in the case of 'confirm email'
    // fields) but they must all contain the same value, else the profile is
    // invalid.
    ServerFieldType server_field_type = field_type.GetStorableType();
    if (server_field_type == EMAIL_ADDRESS &&
        types_seen.count(server_field_type) &&
        candidate_profile.GetRawInfo(EMAIL_ADDRESS) != value)
      return false;

    // If the field type and |value| don't pass basic validity checks then
    // abandon the import.
    if (!IsValidFieldTypeAndValue(types_seen, server_field_type, value))
      return false;
    types_seen.insert(server_field_type);

    // We need to store phone data in the variables, before building the whole
    // number at the end. If |value| is not from a phone field, home.SetInfo()
    // returns false and data is stored directly in |candidate_profile|.
    if (!combined_phone.SetInfo(field_type, value))
      candidate_profile.SetInfo(field_type, value, app_locale_);

    // Reject profiles with invalid country information.
    if (server_field_type == ADDRESS_HOME_COUNTRY &&
        candidate_profile.GetRawInfo(ADDRESS_HOME_COUNTRY).empty())
      return false;
  }

  // Construct the phone number. Reject the whole profile if the number is
  // invalid.
  if (!combined_phone.IsEmpty()) {
    base::string16 constructed_number;
    if (!combined_phone.ParseNumber(candidate_profile, app_locale_,
                                    &constructed_number) ||
        !candidate_profile.SetInfo(AutofillType(PHONE_HOME_WHOLE_NUMBER),
                                   constructed_number, app_locale_)) {
      return false;
    }
  }

  // Reject the profile if minimum address and validation requirements are not
  // met.
  if (!IsValidLearnableProfile(candidate_profile, app_locale_))
    return false;

  personal_data_manager_->SaveImportedProfile(candidate_profile);
  return true;
}

bool FormDataImporter::ImportCreditCard(
    const FormStructure& form,
    bool should_return_local_card,
    std::unique_ptr<CreditCard>* imported_credit_card) {
  DCHECK(!*imported_credit_card);

  // The candidate for credit card import. There are many ways for the candidate
  // to be rejected (see everywhere this function returns false, below).
  bool has_duplicate_field_type;
  CreditCard candidate_credit_card =
      ExtractCreditCardFromForm(form, &has_duplicate_field_type);

  // If we've seen the same credit card field type twice in the same form,
  // abort credit card import/update.
  if (has_duplicate_field_type)
    return false;

  if (!candidate_credit_card.IsValid()) {
    if (candidate_credit_card.HasValidCardNumber()) {
      AutofillMetrics::LogSubmittedCardStateMetric(
          AutofillMetrics::HAS_CARD_NUMBER_ONLY);
    }
    if (candidate_credit_card.HasValidExpirationDate()) {
      AutofillMetrics::LogSubmittedCardStateMetric(
          AutofillMetrics::HAS_EXPIRATION_DATE_ONLY);
    }

    return false;
  }
  AutofillMetrics::LogSubmittedCardStateMetric(
      AutofillMetrics::HAS_CARD_NUMBER_AND_EXPIRATION_DATE);

  // Attempt to merge with an existing credit card. Don't present a prompt if we
  // have already saved this card number, unless |should_return_local_card| is
  // true which indicates that upload is enabled. In this case, it's useful to
  // present the upload prompt to the user to promote the card from a local card
  // to a synced server card, provided we don't have a masked server card with
  // the same |TypeAndLastFourDigits|.
  for (const CreditCard* card : personal_data_manager_->GetLocalCreditCards()) {
    // Make a local copy so that the data in |local_credit_cards_| isn't
    // modified directly by the UpdateFromImportedCard() call.
    CreditCard card_copy(*card);
    if (card_copy.UpdateFromImportedCard(candidate_credit_card, app_locale_)) {
      personal_data_manager_->UpdateCreditCard(card_copy);
      // If we should not return the local card, return that we merged it,
      // without setting |imported_credit_card|.
      if (!should_return_local_card)
        return true;
      // Mark that the credit card potentially being offered to upload is
      // already a local card.
      offering_upload_of_local_credit_card_ = true;

      break;
    }
  }

  // Also don't offer to save if we already have this stored as a server
  // card. We only check the number because if the new card has the same number
  // as the server card, upload is guaranteed to fail. There's no mechanism for
  // entries with the same number but different names or expiration dates as
  // there is for local cards.
  for (const CreditCard* card :
       personal_data_manager_->GetServerCreditCards()) {
    if (candidate_credit_card.HasSameNumberAs(*card)) {
      // Record metric on whether expiration dates matched.
      if (candidate_credit_card.expiration_month() ==
              card->expiration_month() &&
          candidate_credit_card.expiration_year() == card->expiration_year()) {
        AutofillMetrics::LogSubmittedServerCardExpirationStatusMetric(
            card->record_type() == CreditCard::FULL_SERVER_CARD
                ? AutofillMetrics::FULL_SERVER_CARD_EXPIRATION_DATE_MATCHED
                : AutofillMetrics::MASKED_SERVER_CARD_EXPIRATION_DATE_MATCHED);
      } else {
        AutofillMetrics::LogSubmittedServerCardExpirationStatusMetric(
            card->record_type() == CreditCard::FULL_SERVER_CARD
                ? AutofillMetrics::
                      FULL_SERVER_CARD_EXPIRATION_DATE_DID_NOT_MATCH
                : AutofillMetrics::
                      MASKED_SERVER_CARD_EXPIRATION_DATE_DID_NOT_MATCH);
      }
      return false;
    }
  }

  *imported_credit_card = std::make_unique<CreditCard>(candidate_credit_card);
  return true;
}

CreditCard FormDataImporter::ExtractCreditCardFromForm(
    const FormStructure& form,
    bool* has_duplicate_field_type) {
  *has_duplicate_field_type = false;

  CreditCard candidate_credit_card;
  candidate_credit_card.set_origin(form.source_url().spec());

  std::set<ServerFieldType> types_seen;
  for (const auto& field : form) {
    base::string16 value;
    base::TrimWhitespace(field->value, base::TRIM_ALL, &value);

    // If we don't know the type of the field, or the user hasn't entered any
    // information into the field, or the field is non-focusable (hidden), then
    // skip it.
    if (!field->IsFieldFillable() || !field->is_focusable || value.empty())
      continue;

    AutofillType field_type = field->Type();
    // Field was not identified as a credit card field.
    if (field_type.group() != CREDIT_CARD)
      continue;

    // If we've seen the same credit card field type twice in the same form,
    // set |has_duplicate_field_type| to true.
    ServerFieldType server_field_type = field_type.GetStorableType();
    if (types_seen.count(server_field_type)) {
      *has_duplicate_field_type = true;
    } else {
      types_seen.insert(server_field_type);
    }
    // If |field| is an HTML5 month input, handle it as a special case.
    if (base::LowerCaseEqualsASCII(field->form_control_type, "month")) {
      DCHECK_EQ(CREDIT_CARD_EXP_DATE_4_DIGIT_YEAR, server_field_type);
      candidate_credit_card.SetInfoForMonthInputType(value);
      continue;
    }

    // CreditCard handles storing the |value| according to |field_type|.
    bool saved = candidate_credit_card.SetInfo(field_type, value, app_locale_);

    // Saving with the option text (here |value|) may fail for the expiration
    // month. Attempt to save with the option value. First find the index of the
    // option text in the select options and try the corresponding value.
    if (!saved && server_field_type == CREDIT_CARD_EXP_MONTH) {
      for (size_t i = 0; i < field->option_contents.size(); ++i) {
        if (value == field->option_contents[i]) {
          candidate_credit_card.SetInfo(field_type, field->option_values[i],
                                        app_locale_);
          break;
        }
      }
    }
  }

  return candidate_credit_card;
}

}  // namespace autofill
