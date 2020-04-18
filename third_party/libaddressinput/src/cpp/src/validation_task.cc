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

#include "validation_task.h"

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>
#include <libaddressinput/address_metadata.h>
#include <libaddressinput/address_problem.h>
#include <libaddressinput/address_validator.h>
#include <libaddressinput/callback.h>
#include <libaddressinput/supplier.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <re2/re2.h>

#include "lookup_key.h"
#include "post_box_matchers.h"
#include "rule.h"
#include "util/re2ptr.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

ValidationTask::ValidationTask(const AddressData& address,
                               bool allow_postal,
                               bool require_name,
                               const FieldProblemMap* filter,
                               FieldProblemMap* problems,
                               const AddressValidator::Callback& validated)
    : address_(address),
      allow_postal_(allow_postal),
      require_name_(require_name),
      filter_(filter),
      problems_(problems),
      validated_(validated),
      supplied_(BuildCallback(this, &ValidationTask::Validate)),
      lookup_key_(new LookupKey) {
  assert(problems_ != nullptr);
  assert(supplied_ != nullptr);
  assert(lookup_key_ != nullptr);
}

ValidationTask::~ValidationTask() {
}

void ValidationTask::Run(Supplier* supplier) const {
  assert(supplier != nullptr);
  problems_->clear();
  lookup_key_->FromAddress(address_);
  supplier->SupplyGlobally(*lookup_key_, *supplied_);
}

void ValidationTask::Validate(bool success,
                              const LookupKey& lookup_key,
                              const Supplier::RuleHierarchy& hierarchy) {
  assert(&lookup_key == lookup_key_.get());  // Sanity check.

  if (success) {
    if (address_.IsFieldEmpty(COUNTRY)) {
      ReportProblemMaybe(COUNTRY, MISSING_REQUIRED_FIELD);
    } else if (hierarchy.rule[0] == nullptr) {
      ReportProblemMaybe(COUNTRY, UNKNOWN_VALUE);
    } else {
      // Checks which use statically linked metadata.
      const std::string& region_code = address_.region_code;
      CheckUnexpectedField(region_code);
      CheckMissingRequiredField(region_code);

      // Checks which use data from the metadata server. Note that
      // CheckPostalCodeFormatAndValue assumes CheckUnexpectedField has already
      // been called.
      CheckUnknownValue(hierarchy);
      CheckPostalCodeFormatAndValue(hierarchy);
      CheckUsesPoBox(hierarchy);
    }
  }

  validated_(success, address_, *problems_);
  delete this;
}

// A field will return an UNEXPECTED_FIELD problem type if the current value of
// that field is not empty and the field should not be used by that region.
void ValidationTask::CheckUnexpectedField(
    const std::string& region_code) const {
  static const AddressField kFields[] = {
    // COUNTRY is never unexpected.
    ADMIN_AREA,
    LOCALITY,
    DEPENDENT_LOCALITY,
    SORTING_CODE,
    POSTAL_CODE,
    STREET_ADDRESS,
    ORGANIZATION,
    RECIPIENT
  };

  for (size_t i = 0; i < size(kFields); ++i) {
    AddressField field = kFields[i];
    if (!address_.IsFieldEmpty(field) && !IsFieldUsed(field, region_code)) {
      ReportProblemMaybe(field, UNEXPECTED_FIELD);
    }
  }
}

// A field will return an MISSING_REQUIRED_FIELD problem type if the current
// value of that field is empty and the field is required by that region.
void ValidationTask::CheckMissingRequiredField(
    const std::string& region_code) const {
  static const AddressField kFields[] = {
    // COUNTRY is assumed to have already been checked.
    ADMIN_AREA,
    LOCALITY,
    DEPENDENT_LOCALITY,
    SORTING_CODE,
    POSTAL_CODE,
    STREET_ADDRESS
    // ORGANIZATION is never required.
    // RECIPIENT is handled separately.
  };

  for (size_t i = 0; i < size(kFields); ++i) {
    AddressField field = kFields[i];
    if (address_.IsFieldEmpty(field) && IsFieldRequired(field, region_code)) {
      ReportProblemMaybe(field, MISSING_REQUIRED_FIELD);
    }
  }

  if (require_name_ && address_.IsFieldEmpty(RECIPIENT)) {
    ReportProblemMaybe(RECIPIENT, MISSING_REQUIRED_FIELD);
  }
}

// A field is UNKNOWN_VALUE if the metadata contains a list of possible values
// for the field and the address data server could not match the current value
// of that field to one of those possible values, therefore returning nullptr.
void ValidationTask::CheckUnknownValue(
    const Supplier::RuleHierarchy& hierarchy) const {
  for (size_t depth = 1; depth < size(LookupKey::kHierarchy); ++depth) {
    AddressField field = LookupKey::kHierarchy[depth];
    if (!(address_.IsFieldEmpty(field) ||
          hierarchy.rule[depth - 1] == nullptr ||
          hierarchy.rule[depth - 1]->GetSubKeys().empty() ||
          hierarchy.rule[depth] != nullptr)) {
      ReportProblemMaybe(field, UNKNOWN_VALUE);
    }
  }
}

// Note that it is assumed that CheckUnexpectedField has already been called.
void ValidationTask::CheckPostalCodeFormatAndValue(
    const Supplier::RuleHierarchy& hierarchy) const {
  assert(hierarchy.rule[0] != nullptr);
  const Rule& country_rule = *hierarchy.rule[0];

  if (!(ShouldReport(POSTAL_CODE, INVALID_FORMAT) ||
        ShouldReport(POSTAL_CODE, MISMATCHING_VALUE))) {
    return;
  }

  if (address_.IsFieldEmpty(POSTAL_CODE)) {
    return;
  } else if (std::find(problems_->begin(), problems_->end(),
                       FieldProblemMap::value_type(POSTAL_CODE,
                                                   UNEXPECTED_FIELD))
             != problems_->end()) {
    return;  // Problem already reported.
  }

  // Validate general postal code format. A country-level rule specifies the
  // regular expression for the whole postal code.
  const RE2ptr* format_ptr = country_rule.GetPostalCodeMatcher();
  if (format_ptr != nullptr &&
      !RE2::FullMatch(address_.postal_code, *format_ptr->ptr) &&
      ShouldReport(POSTAL_CODE, INVALID_FORMAT)) {
    ReportProblem(POSTAL_CODE, INVALID_FORMAT);
    return;
  }

  if (!ShouldReport(POSTAL_CODE, MISMATCHING_VALUE)) {
    return;
  }

  for (size_t depth = size(LookupKey::kHierarchy) - 1;
       depth > 0; --depth) {
    if (hierarchy.rule[depth] != nullptr) {
      // Validate sub-region specific postal code format. A sub-region specifies
      // the regular expression for a prefix of the postal code.
      const RE2ptr* prefix_ptr = hierarchy.rule[depth]->GetPostalCodeMatcher();
      if (prefix_ptr != nullptr) {
        if (!RE2::PartialMatch(address_.postal_code, *prefix_ptr->ptr)) {
          ReportProblem(POSTAL_CODE, MISMATCHING_VALUE);
        }
        return;
      }
    }
  }
}

void ValidationTask::CheckUsesPoBox(
    const Supplier::RuleHierarchy& hierarchy) const {
  assert(hierarchy.rule[0] != nullptr);
  const Rule& country_rule = *hierarchy.rule[0];

  if (allow_postal_ ||
      !ShouldReport(STREET_ADDRESS, USES_P_O_BOX) ||
      address_.IsFieldEmpty(STREET_ADDRESS)) {
    return;
  }

  std::vector<const RE2ptr*> matchers =
      PostBoxMatchers::GetMatchers(country_rule);
  for (std::vector<std::string>::const_iterator
       line = address_.address_line.begin();
       line != address_.address_line.end(); ++line) {
    for (std::vector<const RE2ptr*>::const_iterator
         matcher = matchers.begin();
         matcher != matchers.end(); ++matcher) {
      if (RE2::PartialMatch(*line, *(*matcher)->ptr)) {
        ReportProblem(STREET_ADDRESS, USES_P_O_BOX);
        return;
      }
    }
  }
}

void ValidationTask::ReportProblem(AddressField field,
                                   AddressProblem problem) const {
  problems_->insert(std::make_pair(field, problem));
}

void ValidationTask::ReportProblemMaybe(AddressField field,
                                        AddressProblem problem) const {
  if (ShouldReport(field, problem)) {
    ReportProblem(field, problem);
  }
}

bool ValidationTask::ShouldReport(AddressField field,
                                  AddressProblem problem) const {
  return filter_ == nullptr || filter_->empty() ||
         std::find(filter_->begin(),
                   filter_->end(),
                   FieldProblemMap::value_type(field, problem)) !=
             filter_->end();
}

}  // namespace addressinput
}  // namespace i18n
