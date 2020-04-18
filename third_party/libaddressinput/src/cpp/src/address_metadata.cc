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

#include <libaddressinput/address_metadata.h>

#include <libaddressinput/address_field.h>

#include <algorithm>
#include <string>

#include "format_element.h"
#include "region_data_constants.h"
#include "rule.h"

namespace i18n {
namespace addressinput {

bool IsFieldRequired(AddressField field, const std::string& region_code) {
  if (field == COUNTRY) {
    return true;
  }

  Rule rule;
  rule.CopyFrom(Rule::GetDefault());
  if (!rule.ParseSerializedRule(
          RegionDataConstants::GetRegionData(region_code))) {
    return false;
  }

  return std::find(rule.GetRequired().begin(),
                   rule.GetRequired().end(),
                   field) != rule.GetRequired().end();
}

bool IsFieldUsed(AddressField field, const std::string& region_code) {
  if (field == COUNTRY) {
    return true;
  }

  Rule rule;
  rule.CopyFrom(Rule::GetDefault());
  if (!rule.ParseSerializedRule(
          RegionDataConstants::GetRegionData(region_code))) {
    return false;
  }

  return std::find(rule.GetFormat().begin(),
                   rule.GetFormat().end(),
                   FormatElement(field)) != rule.GetFormat().end();
}

}  // namespace addressinput
}  // namespace i18n
