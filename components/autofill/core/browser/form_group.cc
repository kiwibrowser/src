// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/form_group.h"

#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_profile_comparator.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/common/autofill_l10n_util.h"

namespace autofill {

void FormGroup::GetMatchingTypes(const base::string16& text,
                                 const std::string& app_locale,
                                 ServerFieldTypeSet* matching_types) const {
  if (text.empty()) {
    matching_types->insert(EMPTY_TYPE);
    return;
  }

  AutofillProfileComparator comparator(app_locale);
  base::string16 canonicalized_text = comparator.NormalizeForComparison(text);

  if (canonicalized_text.empty())
    return;

  ServerFieldTypeSet types;
  GetSupportedTypes(&types);
  for (const auto& type : types) {
    base::string16 candidate_text = comparator.NormalizeForComparison(
        GetInfo(AutofillType(type), app_locale));
    if (canonicalized_text == candidate_text)
      matching_types->insert(type);
  }
}

void FormGroup::GetNonEmptyTypes(const std::string& app_locale,
                                 ServerFieldTypeSet* non_empty_types) const {
  ServerFieldTypeSet types;
  GetSupportedTypes(&types);
  for (const auto& type : types) {
    if (!GetInfo(AutofillType(type), app_locale).empty())
      non_empty_types->insert(type);
  }
}

base::string16 FormGroup::GetInfo(ServerFieldType type,
                                  const std::string& app_locale) const {
  return GetInfoImpl(AutofillType(type), app_locale);
}

base::string16 FormGroup::GetInfo(const AutofillType& type,
                                  const std::string& app_locale) const {
  return GetInfoImpl(type, app_locale);
}

bool FormGroup::SetInfo(ServerFieldType type,
                        const base::string16& value,
                        const std::string& app_locale) {
  return SetInfoImpl(AutofillType(type), value, app_locale);
}

bool FormGroup::SetInfo(const AutofillType& type,
                        const base::string16& value,
                        const std::string& app_locale) {
  return SetInfoImpl(type, value, app_locale);
}

bool FormGroup::HasInfo(ServerFieldType type) const {
  return HasInfo(AutofillType(type));
}

bool FormGroup::HasInfo(const AutofillType& type) const {
  // Use "en-US" as a placeholder locale. We are only interested in emptiness,
  // not in the presentation of the string.
  return !GetInfo(type, "en-US").empty();
}

base::string16 FormGroup::GetInfoImpl(const AutofillType& type,
                                      const std::string& app_locale) const {
  return GetRawInfo(type.GetStorableType());
}

bool FormGroup::SetInfoImpl(const AutofillType& type,
                            const base::string16& value,
                            const std::string& app_locale) {
  SetRawInfo(type.GetStorableType(), value);
  return true;
}

}  // namespace autofill
