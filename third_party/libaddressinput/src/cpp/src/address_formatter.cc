// Copyright (C) 2014 Google Inc.
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

#include <libaddressinput/address_formatter.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "format_element.h"
#include "language.h"
#include "region_data_constants.h"
#include "rule.h"
#include "util/cctype_tolower_equal.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

namespace {

const char kCommaSeparator[] = ", ";
const char kSpaceSeparator[] = " ";
const char kArabicCommaSeparator[] = u8"، ";

const char* kLanguagesThatUseSpace[] = {
  "th",
  "ko"
};

const char* kLanguagesThatHaveNoSeparator[] = {
  "ja",
  "zh"  // All Chinese variants.
};

// This data is based on CLDR, for languages that are in official use in some
// country, where Arabic is the most likely script tag.
// TODO: Consider supporting variants such as tr-Arab by detecting the script
// code.
const char* kLanguagesThatUseAnArabicComma[] = {
  "ar",
  "az",
  "fa",
  "kk",
  "ku",
  "ky",
  "ps",
  "tg",
  "tk",
  "ur",
  "uz"
};

std::string GetLineSeparatorForLanguage(const std::string& language_tag) {
  Language address_language(language_tag);

  // First deal with explicit script tags.
  if (address_language.has_latin_script) {
    return kCommaSeparator;
  }

  // Now guess something appropriate based on the base language.
  const std::string& base_language = address_language.base;
  if (std::find_if(kLanguagesThatUseSpace,
                   kLanguagesThatUseSpace + size(kLanguagesThatUseSpace),
                   std::bind2nd(EqualToTolowerString(), base_language)) !=
      kLanguagesThatUseSpace + size(kLanguagesThatUseSpace)) {
    return kSpaceSeparator;
  } else if (std::find_if(
                 kLanguagesThatHaveNoSeparator,
                 kLanguagesThatHaveNoSeparator +
                     size(kLanguagesThatHaveNoSeparator),
                 std::bind2nd(EqualToTolowerString(), base_language)) !=
             kLanguagesThatHaveNoSeparator +
                 size(kLanguagesThatHaveNoSeparator)) {
    return "";
  } else if (std::find_if(
                 kLanguagesThatUseAnArabicComma,
                 kLanguagesThatUseAnArabicComma +
                     size(kLanguagesThatUseAnArabicComma),
                 std::bind2nd(EqualToTolowerString(), base_language)) !=
             kLanguagesThatUseAnArabicComma +
                 size(kLanguagesThatUseAnArabicComma)) {
    return kArabicCommaSeparator;
  }
  // Either the language is a Latin-script language, or no language was
  // specified. In the latter case we still return ", " as the most common
  // separator in use. In countries that don't use this, e.g. Thailand,
  // addresses are often written in Latin script where this would still be
  // appropriate, so this is a reasonable default in the absence of information.
  return kCommaSeparator;
}

void CombineLinesForLanguage(const std::vector<std::string>& lines,
                             const std::string& language_tag,
                             std::string* line) {
  line->clear();
  std::string separator = GetLineSeparatorForLanguage(language_tag);
  for (std::vector<std::string>::const_iterator it = lines.begin();
       it != lines.end();
       ++it) {
    if (it != lines.begin()) {
      line->append(separator);
    }
    line->append(*it);
  }
}

}  // namespace

void GetFormattedNationalAddress(
    const AddressData& address_data, std::vector<std::string>* lines) {
  assert(lines != nullptr);
  lines->clear();

  Rule rule;
  rule.CopyFrom(Rule::GetDefault());
  // TODO: Eventually, we should get the best rule for this country and
  // language, rather than just for the country.
  rule.ParseSerializedRule(RegionDataConstants::GetRegionData(
      address_data.region_code));

  Language language(address_data.language_code);

  // If Latin-script rules are available and the |language_code| of this address
  // is explicitly tagged as being Latin, then use the Latin-script formatting
  // rules.
  const std::vector<FormatElement>& format =
      language.has_latin_script && !rule.GetLatinFormat().empty()
          ? rule.GetLatinFormat()
          : rule.GetFormat();

  // Address format without the unnecessary elements (based on which address
  // fields are empty). We assume all literal strings that are not at the start
  // or end of a line are separators, and therefore only relevant if the
  // surrounding fields are filled in. This works with the data we have
  // currently.
  std::vector<FormatElement> pruned_format;
  for (std::vector<FormatElement>::const_iterator
       element_it = format.begin();
       element_it != format.end();
       ++element_it) {
    // Always keep the newlines.
    if (element_it->IsNewline() ||
        // Always keep the non-empty address fields.
        (element_it->IsField() &&
         !address_data.IsFieldEmpty(element_it->GetField())) ||
        // Only keep literals that satisfy these 2 conditions:
        (!element_it->IsField() &&
         // (1) Not preceding an empty field.
         (element_it + 1 == format.end() ||
          !(element_it + 1)->IsField() ||
          !address_data.IsFieldEmpty((element_it + 1)->GetField())) &&
         // (2) Not following a removed field.
         (element_it == format.begin() ||
          !(element_it - 1)->IsField() ||
          (!pruned_format.empty() && pruned_format.back().IsField())))) {
      pruned_format.push_back(*element_it);
    }
  }

  std::string line;
  for (std::vector<FormatElement>::const_iterator
       element_it = pruned_format.begin();
       element_it != pruned_format.end();
       ++element_it) {
    if (element_it->IsNewline()) {
      if (!line.empty()) {
        lines->push_back(line);
        line.clear();
      }
    } else if (element_it->IsField()) {
      AddressField field = element_it->GetField();
      if (field == STREET_ADDRESS) {
        // The field "street address" represents the street address lines of an
        // address, so there can be multiple values.
        if (!address_data.IsFieldEmpty(field)) {
          line.append(address_data.address_line.front());
          if (address_data.address_line.size() > 1U) {
            lines->push_back(line);
            line.clear();
            lines->insert(lines->end(),
                          address_data.address_line.begin() + 1,
                          address_data.address_line.end());
          }
        }
      } else {
        line.append(address_data.GetFieldValue(field));
      }
    } else {
      line.append(element_it->GetLiteral());
    }
  }
  if (!line.empty()) {
    lines->push_back(line);
  }
}

void GetFormattedNationalAddressLine(
    const AddressData& address_data, std::string* line) {
  std::vector<std::string> address_lines;
  GetFormattedNationalAddress(address_data, &address_lines);
  CombineLinesForLanguage(address_lines, address_data.language_code, line);
}

void GetStreetAddressLinesAsSingleLine(
    const AddressData& address_data, std::string* line) {
  CombineLinesForLanguage(
      address_data.address_line, address_data.language_code, line);
}

}  // namespace addressinput
}  // namespace i18n
