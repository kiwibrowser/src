// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <libaddressinput/address_ui.h>

#include <libaddressinput/address_field.h>
#include <libaddressinput/address_ui_component.h>
#include <libaddressinput/localization.h>

#include <cassert>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "format_element.h"
#include "grit.h"
#include "language.h"
#include "messages.h"
#include "region_data_constants.h"
#include "rule.h"

namespace i18n {
namespace addressinput {

namespace {

std::string GetLabelForField(const Localization& localization,
                             AddressField field,
                             int admin_area_name_message_id,
                             int postal_code_name_message_id,
                             int locality_name_message_id,
                             int sublocality_name_message_id) {
  int message_id;
  switch (field) {
    case SORTING_CODE:
      // This needs no translation as it's used only in one locale.
      return "CEDEX";
    case COUNTRY:
      message_id = IDS_LIBADDRESSINPUT_COUNTRY_OR_REGION_LABEL;
      break;
    case ADMIN_AREA:
      message_id = admin_area_name_message_id;
      break;
    case LOCALITY:
      message_id = locality_name_message_id;
      break;
    case DEPENDENT_LOCALITY:
      message_id = sublocality_name_message_id;
      break;
    case POSTAL_CODE:
      message_id = postal_code_name_message_id;
      break;
    case STREET_ADDRESS:
      message_id = IDS_LIBADDRESSINPUT_ADDRESS_LINE_1_LABEL;
      break;
    case ORGANIZATION:
      message_id = IDS_LIBADDRESSINPUT_ORGANIZATION_LABEL;
      break;
    case RECIPIENT:
      message_id = IDS_LIBADDRESSINPUT_RECIPIENT_LABEL;
      break;
    default:
      message_id = INVALID_MESSAGE_ID;
  }
  return localization.GetString(message_id);
}

}  // namespace

const std::vector<std::string>& GetRegionCodes() {
  return RegionDataConstants::GetRegionCodes();
}

std::vector<AddressUiComponent> BuildComponents(
    const std::string& region_code,
    const Localization& localization,
    const std::string& ui_language_tag,
    std::string* best_address_language_tag) {
  assert(best_address_language_tag != nullptr);
  std::vector<AddressUiComponent> result;

  Rule rule;
  rule.CopyFrom(Rule::GetDefault());
  if (!rule.ParseSerializedRule(
          RegionDataConstants::GetRegionData(region_code))) {
    return result;
  }

  const Language& best_address_language =
      ChooseBestAddressLanguage(rule, Language(ui_language_tag));
  *best_address_language_tag = best_address_language.tag;

  const std::vector<FormatElement>& format =
      !rule.GetLatinFormat().empty() && best_address_language.has_latin_script
          ? rule.GetLatinFormat()
          : rule.GetFormat();

  // For avoiding showing an input field twice, when the field is displayed
  // twice on an envelope.
  std::set<AddressField> fields;

  bool preceded_by_newline = true;
  bool followed_by_newline = true;
  for (std::vector<FormatElement>::const_iterator format_it = format.begin();
       format_it != format.end(); ++format_it) {
    if (format_it->IsNewline()) {
      preceded_by_newline = true;
      continue;
    } else if (!format_it->IsField() ||
               !fields.insert(format_it->GetField()).second) {
      continue;
    }
    AddressUiComponent component;
    std::vector<FormatElement>::const_iterator next_format_it = format_it + 1;
    followed_by_newline =
        next_format_it == format.end() || next_format_it->IsNewline();
    component.length_hint = preceded_by_newline && followed_by_newline
                                ? AddressUiComponent::HINT_LONG
                                : AddressUiComponent::HINT_SHORT;
    preceded_by_newline = false;
    component.field = format_it->GetField();
    component.name = GetLabelForField(localization,
                                      format_it->GetField(),
                                      rule.GetAdminAreaNameMessageId(),
                                      rule.GetPostalCodeNameMessageId(),
                                      rule.GetLocalityNameMessageId(),
                                      rule.GetSublocalityNameMessageId());
    result.push_back(component);
  }

  return result;
}

}  // namespace addressinput
}  // namespace i18n
