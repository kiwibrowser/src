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

#include <libaddressinput/localization.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>
#include <libaddressinput/address_problem.h>

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "messages.h"
#include "region_data_constants.h"
#include "rule.h"
#include "util/string_split.h"
#include "util/string_util.h"

namespace {

void PushBackUrl(const std::string& url, std::vector<std::string>* parameters) {
  assert(parameters != nullptr);
  // TODO: HTML-escape the "url".
  parameters->push_back("<a href=\"" + url + "\">");
  parameters->push_back("</a>");
}

}  // namespace

namespace i18n {
namespace addressinput {

namespace {

#include "en_messages.cc"

std::string GetEnglishString(int message_id) {
  const char* str = GetString(message_id);
  return str != nullptr ? std::string(str) : std::string();
}

}  // namespace

Localization::Localization() : get_string_(&GetEnglishString) {}

Localization::~Localization() {}

std::string Localization::GetString(int message_id) const {
  return get_string_(message_id);
}

std::string Localization::GetErrorMessage(const AddressData& address,
                                          AddressField field,
                                          AddressProblem problem,
                                          bool enable_examples,
                                          bool enable_links) const {
  if (field == POSTAL_CODE) {
    Rule rule;
    rule.CopyFrom(Rule::GetDefault());
    std::string postal_code_example, post_service_url;
    if (rule.ParseSerializedRule(
            RegionDataConstants::GetRegionData(address.region_code))) {
      if (enable_examples) {
        std::vector<std::string> examples_list;
        SplitString(rule.GetPostalCodeExample(), ',', &examples_list);
        if (!examples_list.empty()) {
          postal_code_example = examples_list.front();
        }
      }
      if (enable_links) {
        post_service_url = rule.GetPostServiceUrl();
      }
    } else {
      assert(false);
    }
    // If we can't parse the serialized rule |uses_postal_code_as_label| will be
    // determined from the default rule.
    bool uses_postal_code_as_label =
        rule.GetPostalCodeNameMessageId() ==
        IDS_LIBADDRESSINPUT_POSTAL_CODE_LABEL;
    return GetErrorMessageForPostalCode(problem, uses_postal_code_as_label,
                                        postal_code_example, post_service_url);
  } else {
    if (problem == MISSING_REQUIRED_FIELD) {
      return get_string_(IDS_LIBADDRESSINPUT_MISSING_REQUIRED_FIELD);
    } else if (problem == UNKNOWN_VALUE) {
      std::vector<std::string> parameters;
      if (AddressData::IsRepeatedFieldValue(field)) {
        std::vector<std::string> values = address.GetRepeatedFieldValue(field);
        assert(!values.empty());
        parameters.push_back(values.front());
      } else {
        parameters.push_back(address.GetFieldValue(field));
      }
      return DoReplaceStringPlaceholders(
          get_string_(IDS_LIBADDRESSINPUT_UNKNOWN_VALUE), parameters);
    } else if (problem == USES_P_O_BOX) {
      return get_string_(IDS_LIBADDRESSINPUT_PO_BOX_FORBIDDEN_VALUE);
    } else {
      // Keep the default under "else" so the compiler helps us check that all
      // handled cases return and don't fall through.
      assert(false);
      return "";
    }
  }
}

void Localization::SetGetter(std::string (*getter)(int)) {
  assert(getter != nullptr);
  get_string_ = getter;
}

std::string Localization::GetErrorMessageForPostalCode(
    AddressProblem problem,
    bool uses_postal_code_as_label,
    const std::string& postal_code_example,
    const std::string& post_service_url) const {
  int message_id;
  std::vector<std::string> parameters;
  if (problem == MISSING_REQUIRED_FIELD) {
    if (!postal_code_example.empty() && !post_service_url.empty()) {
      message_id = uses_postal_code_as_label ?
          IDS_LIBADDRESSINPUT_MISSING_REQUIRED_POSTAL_CODE_EXAMPLE_AND_URL :
          IDS_LIBADDRESSINPUT_MISSING_REQUIRED_ZIP_CODE_EXAMPLE_AND_URL;
      parameters.push_back(postal_code_example);
      PushBackUrl(post_service_url, &parameters);
    } else if (!postal_code_example.empty()) {
      message_id = uses_postal_code_as_label ?
          IDS_LIBADDRESSINPUT_MISSING_REQUIRED_POSTAL_CODE_EXAMPLE :
          IDS_LIBADDRESSINPUT_MISSING_REQUIRED_ZIP_CODE_EXAMPLE;
      parameters.push_back(postal_code_example);
    } else {
      message_id = IDS_LIBADDRESSINPUT_MISSING_REQUIRED_FIELD;
    }
    return DoReplaceStringPlaceholders(get_string_(message_id), parameters);
  } else if (problem == INVALID_FORMAT) {
    if (!postal_code_example.empty() && !post_service_url.empty()) {
      message_id = uses_postal_code_as_label ?
          IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_POSTAL_CODE_EXAMPLE_AND_URL :
          IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_ZIP_CODE_EXAMPLE_AND_URL;
      parameters.push_back(postal_code_example);
      PushBackUrl(post_service_url, &parameters);
    } else if (!postal_code_example.empty()) {
      message_id = uses_postal_code_as_label ?
          IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_POSTAL_CODE_EXAMPLE :
          IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_ZIP_CODE_EXAMPLE;
      parameters.push_back(postal_code_example);
    } else {
      message_id = uses_postal_code_as_label ?
          IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_POSTAL_CODE :
          IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_ZIP;
    }
    return DoReplaceStringPlaceholders(get_string_(message_id), parameters);
  } else if (problem == MISMATCHING_VALUE) {
    if (!post_service_url.empty()) {
      message_id = uses_postal_code_as_label ?
          IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_POSTAL_CODE_URL :
          IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_ZIP_URL;
      PushBackUrl(post_service_url, &parameters);
    } else {
      message_id = uses_postal_code_as_label ?
          IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_POSTAL_CODE :
          IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_ZIP;
    }
    return DoReplaceStringPlaceholders(get_string_(message_id), parameters);
  } else {
    // Keep the default under "else" so the compiler helps us check that all
    // handled cases return and don't fall through.
    assert(false);
    return "";
  }
}

}  // namespace addressinput
}  // namespace i18n
