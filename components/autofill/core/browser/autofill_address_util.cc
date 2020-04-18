// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_address_util.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "components/autofill/core/browser/country_combobox_model.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_ui.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_ui_component.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/localization.h"
#include "ui/base/l10n/l10n_util.h"

using autofill::AutofillCountry;
using autofill::ServerFieldType;
using i18n::addressinput::AddressUiComponent;

namespace autofill {

// Dictionary keys for address components info.
const char kFieldTypeKey[] = "field";
const char kFieldLengthKey[] = "length";
const char kFieldNameKey[] = "name";

// Field names for the address components.
const char kFullNameField[] = "fullName";
const char kCompanyNameField[] = "companyName";
const char kAddressLineField[] = "addrLines";
const char kDependentLocalityField[] = "dependentLocality";
const char kCityField[] = "city";
const char kStateField[] = "state";
const char kPostalCodeField[] = "postalCode";
const char kSortingCodeField[] = "sortingCode";
const char kCountryField[] = "country";

// Address field length values.
const char kShortField[] = "short";
const char kLongField[] = "long";

ServerFieldType GetFieldTypeFromString(const std::string& type) {
  if (type == kFullNameField)
    return NAME_FULL;
  if (type == kCompanyNameField)
    return COMPANY_NAME;
  if (type == kAddressLineField)
    return ADDRESS_HOME_STREET_ADDRESS;
  if (type == kDependentLocalityField)
    return ADDRESS_HOME_DEPENDENT_LOCALITY;
  if (type == kCityField)
    return ADDRESS_HOME_CITY;
  if (type == kStateField)
    return ADDRESS_HOME_STATE;
  if (type == kPostalCodeField)
    return ADDRESS_HOME_ZIP;
  if (type == kSortingCodeField)
    return ADDRESS_HOME_SORTING_CODE;
  if (type == kCountryField)
    return ADDRESS_HOME_COUNTRY;
  NOTREACHED();
  return UNKNOWN_TYPE;
}

// Fills |components| with the address UI components that should be used to
// input an address for |country_code| when UI BCP 47 language code is
// |ui_language_code|. If |components_language_code| is not NULL, then sets it
// to the BCP 47 language code that should be used to format the address for
// display.
void GetAddressComponents(const std::string& country_code,
                          const std::string& ui_language_code,
                          base::ListValue* address_components,
                          std::string* components_language_code) {
  DCHECK(address_components);

  i18n::addressinput::Localization localization;
  localization.SetGetter(l10n_util::GetStringUTF8);
  std::string not_used;
  std::vector<AddressUiComponent> components =
      i18n::addressinput::BuildComponents(
          country_code, localization, ui_language_code,
          components_language_code ? components_language_code : &not_used);
  if (components.empty()) {
    static const char kDefaultCountryCode[] = "US";
    components = i18n::addressinput::BuildComponents(
        kDefaultCountryCode, localization, ui_language_code,
        components_language_code ? components_language_code : &not_used);
  }
  DCHECK(!components.empty());

  base::ListValue* line = nullptr;
  for (size_t i = 0; i < components.size(); ++i) {
    if (i == 0 ||
        components[i - 1].length_hint == AddressUiComponent::HINT_LONG ||
        components[i].length_hint == AddressUiComponent::HINT_LONG) {
      line = new base::ListValue;
      address_components->Append(base::WrapUnique(line));
      // |line| is invalidated at this point, so it needs to be reset.
      address_components->GetList(address_components->GetSize() - 1, &line);
    }

    auto component = std::make_unique<base::DictionaryValue>();
    component->SetString(kFieldNameKey, components[i].name);

    switch (components[i].field) {
      case i18n::addressinput::COUNTRY:
        component->SetString(kFieldTypeKey, kCountryField);
        break;
      case i18n::addressinput::ADMIN_AREA:
        component->SetString(kFieldTypeKey, kStateField);
        break;
      case i18n::addressinput::LOCALITY:
        component->SetString(kFieldTypeKey, kCityField);
        break;
      case i18n::addressinput::DEPENDENT_LOCALITY:
        component->SetString(kFieldTypeKey, kDependentLocalityField);
        break;
      case i18n::addressinput::SORTING_CODE:
        component->SetString(kFieldTypeKey, kSortingCodeField);
        break;
      case i18n::addressinput::POSTAL_CODE:
        component->SetString(kFieldTypeKey, kPostalCodeField);
        break;
      case i18n::addressinput::STREET_ADDRESS:
        component->SetString(kFieldTypeKey, kAddressLineField);
        break;
      case i18n::addressinput::ORGANIZATION:
        component->SetString(kFieldTypeKey, kCompanyNameField);
        break;
      case i18n::addressinput::RECIPIENT:
        component->SetString(kFieldTypeKey, kFullNameField);
        break;
    }

    switch (components[i].length_hint) {
      case AddressUiComponent::HINT_LONG:
        component->SetString(kFieldLengthKey, kLongField);
        break;
      case AddressUiComponent::HINT_SHORT:
        component->SetString(kFieldLengthKey, kShortField);
        break;
    }

    line->Append(std::move(component));
  }
}

// Sets data related to the country <select>.
void SetCountryData(const PersonalDataManager& manager,
                    base::DictionaryValue* localized_strings,
                    const std::string& ui_language_code) {
  autofill::CountryComboboxModel model;
  model.SetCountries(manager, base::Callback<bool(const std::string&)>(),
                     ui_language_code);
  const std::vector<std::unique_ptr<autofill::AutofillCountry>>& countries =
      model.countries();
  localized_strings->SetString("defaultCountryCode",
                               countries.front()->country_code());

  // An ordered list of options to show in the <select>.
  auto country_list = std::make_unique<base::ListValue>();
  for (size_t i = 0; i < countries.size(); ++i) {
    auto option_details = std::make_unique<base::DictionaryValue>();
    option_details->SetString("name", model.GetItemAt(i));
    option_details->SetString(
        "value", countries[i] ? countries[i]->country_code() : "separator");
    country_list->Append(std::move(option_details));
  }
  localized_strings->Set("autofillCountrySelectList", std::move(country_list));

  auto default_country_components = std::make_unique<base::ListValue>();
  std::string default_country_language_code;
  GetAddressComponents(countries.front()->country_code(), ui_language_code,
                       default_country_components.get(),
                       &default_country_language_code);
  localized_strings->Set("autofillDefaultCountryComponents",
                         std::move(default_country_components));
  localized_strings->SetString("autofillDefaultCountryLanguageCode",
                               default_country_language_code);
}

}  // namespace autofill
