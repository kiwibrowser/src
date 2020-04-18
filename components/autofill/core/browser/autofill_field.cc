// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_field.h"

#include <stdint.h>

#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/proto/server.pb.h"

namespace autofill {

AutofillField::AutofillField()
    : server_type_(NO_SERVER_DATA),
      heuristic_type_(UNKNOWN_TYPE),
      overall_type_(AutofillType(NO_SERVER_DATA)),
      html_type_(HTML_TYPE_UNSPECIFIED),
      html_mode_(HTML_MODE_NONE),
      phone_part_(IGNORED),
      credit_card_number_offset_(0),
      previously_autofilled_(false),
      only_fill_when_focused_(false),
      generation_type_(AutofillUploadContents::Field::NO_GENERATION),
      generated_password_changed_(false),
      form_classifier_outcome_(AutofillUploadContents::Field::NO_OUTCOME),
      vote_type_(AutofillUploadContents::Field::NO_INFORMATION) {}

AutofillField::AutofillField(const FormFieldData& field,
                             const base::string16& unique_name)
    : FormFieldData(field),
      unique_name_(unique_name),
      server_type_(NO_SERVER_DATA),
      heuristic_type_(UNKNOWN_TYPE),
      overall_type_(AutofillType(NO_SERVER_DATA)),
      html_type_(HTML_TYPE_UNSPECIFIED),
      html_mode_(HTML_MODE_NONE),
      phone_part_(IGNORED),
      credit_card_number_offset_(0),
      previously_autofilled_(false),
      only_fill_when_focused_(false),
      parseable_name_(field.name),
      generation_type_(AutofillUploadContents::Field::NO_GENERATION),
      generated_password_changed_(false),
      form_classifier_outcome_(AutofillUploadContents::Field::NO_OUTCOME),
      vote_type_(AutofillUploadContents::Field::NO_INFORMATION) {}

AutofillField::~AutofillField() {}

void AutofillField::set_heuristic_type(ServerFieldType type) {
  if (type >= 0 && type < MAX_VALID_FIELD_TYPE &&
      type != FIELD_WITH_DEFAULT_VALUE) {
    heuristic_type_ = type;
  } else {
    NOTREACHED();
    // This case should not be reachable; but since this has potential
    // implications on data uploaded to the server, better safe than sorry.
    heuristic_type_ = UNKNOWN_TYPE;
  }
  overall_type_ = AutofillType(NO_SERVER_DATA);
}

void AutofillField::set_server_type(ServerFieldType type) {
  // Chrome no longer supports fax numbers, but the server still does.
  if (type >= PHONE_FAX_NUMBER && type <= PHONE_FAX_WHOLE_NUMBER)
    return;

  server_type_ = type;
  overall_type_ = AutofillType(NO_SERVER_DATA);
}

void AutofillField::SetHtmlType(HtmlFieldType type, HtmlFieldMode mode) {
  html_type_ = type;
  html_mode_ = mode;
  overall_type_ = AutofillType(NO_SERVER_DATA);

  if (type == HTML_TYPE_TEL_LOCAL_PREFIX)
    phone_part_ = PHONE_PREFIX;
  else if (type == HTML_TYPE_TEL_LOCAL_SUFFIX)
    phone_part_ = PHONE_SUFFIX;
  else
    phone_part_ = IGNORED;
}

void AutofillField::SetTypeTo(const AutofillType& type) {
  DCHECK(type.GetStorableType() != NO_SERVER_DATA);
  overall_type_ = type;
}

AutofillType AutofillField::ComputedType() const {
  // If autocomplete=tel/tel-* and server confirms it really is a phone field,
  // we always user the server prediction as html types are not very reliable.
  if ((GroupTypeOfHtmlFieldType(html_type_, html_mode_) == PHONE_BILLING ||
       GroupTypeOfHtmlFieldType(html_type_, html_mode_) == PHONE_HOME) &&
      (GroupTypeOfServerFieldType(server_type_) == PHONE_BILLING ||
       GroupTypeOfServerFieldType(server_type_) == PHONE_HOME)) {
    return AutofillType(server_type_);
  }

  // Use the html type specified by the website unless it is unrecognized and
  // autofill predicts a credit card type.
  if (html_type_ != HTML_TYPE_UNSPECIFIED &&
      !(html_type_ == HTML_TYPE_UNRECOGNIZED && IsCreditCardPrediction())) {
    return AutofillType(html_type_, html_mode_);
  }

  if (server_type_ != NO_SERVER_DATA) {
    // Sometimes the server and heuristics disagree on whether a name field
    // should be associated with an address or a credit card. There was a
    // decision to prefer the heuristics in these cases, but it looks like
    // it might be better to fix this server-side.
    // See http://crbug.com/429236 for background.
    bool believe_server;
    if (base::FeatureList::IsEnabled(kAutofillPreferServerNamePredictions)) {
      believe_server = true;
    } else {
      believe_server = !(server_type_ == NAME_FULL &&
                         heuristic_type_ == CREDIT_CARD_NAME_FULL) &&
                       !(server_type_ == CREDIT_CARD_NAME_FULL &&
                         heuristic_type_ == NAME_FULL) &&
                       !(server_type_ == NAME_FIRST &&
                         heuristic_type_ == CREDIT_CARD_NAME_FIRST) &&
                       !(server_type_ == NAME_LAST &&
                         heuristic_type_ == CREDIT_CARD_NAME_LAST);
    }

    // Either way, retain a preference for the the CVC heuristic over the
    // server's password predictions (http://crbug.com/469007)
    believe_server = believe_server &&
                     !(AutofillType(server_type_).group() == PASSWORD_FIELD &&
                       heuristic_type_ == CREDIT_CARD_VERIFICATION_CODE);
    if (believe_server)
      return AutofillType(server_type_);
  }

  return AutofillType(heuristic_type_);
}

AutofillType AutofillField::Type() const {
  if (overall_type_.GetStorableType() != NO_SERVER_DATA) {
    return overall_type_;
  }
  return ComputedType();
}

bool AutofillField::IsEmpty() const {
  return value.empty();
}

FieldSignature AutofillField::GetFieldSignature() const {
  return CalculateFieldSignatureByNameAndType(name, form_control_type);
}

std::string AutofillField::FieldSignatureAsStr() const {
  return base::UintToString(GetFieldSignature());
}

bool AutofillField::IsFieldFillable() const {
  return !Type().IsUnknown();
}

bool AutofillField::IsCreditCardPrediction() const {
  return AutofillType(server_type_).group() == CREDIT_CARD ||
         AutofillType(heuristic_type_).group() == CREDIT_CARD;
}

}  // namespace autofill
