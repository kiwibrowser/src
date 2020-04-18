// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/parse_info.h"

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "extensions/common/error_utils.h"

namespace extensions {
namespace declarative_net_request {

ParseInfo::ParseInfo(ParseResult result) : result_(result) {}
ParseInfo::ParseInfo(ParseResult result, size_t rule_index)
    : result_(result), rule_index_(rule_index) {}
ParseInfo::ParseInfo(const ParseInfo&) = default;
ParseInfo& ParseInfo::operator=(const ParseInfo&) = default;

std::string ParseInfo::GetErrorDescription(
    const base::StringPiece json_rules_filename) const {
  // Every error except ERROR_PERSISTING_RULESET requires |rule_index_|.
  DCHECK_EQ(!rule_index_.has_value(),
            result_ == ParseResult::ERROR_PERSISTING_RULESET);

  std::string error;
  switch (result_) {
    case ParseResult::SUCCESS:
      NOTREACHED();
      break;
    case ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED:
      error = ErrorUtils::FormatErrorMessage(kErrorResourceTypeDuplicated,
                                             json_rules_filename,
                                             std::to_string(*rule_index_));
      break;
    case ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY:
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyRedirectRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kPriorityKey);
      break;
    case ParseResult::ERROR_EMPTY_REDIRECT_URL:
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyRedirectRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kRedirectUrlKey);
      break;
    case ParseResult::ERROR_INVALID_RULE_ID:
      error = ErrorUtils::FormatErrorMessage(
          kErrorInvalidRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kIDKey, std::to_string(kMinValidID));
      break;
    case ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY:
      error = ErrorUtils::FormatErrorMessage(
          kErrorInvalidRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kPriorityKey,
          std::to_string(kMinValidPriority));
      break;
    case ParseResult::ERROR_NO_APPLICABLE_RESOURCE_TYPES:
      error = ErrorUtils::FormatErrorMessage(kErrorNoApplicableResourceTypes,
                                             json_rules_filename,
                                             std::to_string(*rule_index_));
      break;
    case ParseResult::ERROR_EMPTY_DOMAINS_LIST:
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyList, json_rules_filename, std::to_string(*rule_index_),
          kDomainsKey);
      break;
    case ParseResult::ERROR_EMPTY_RESOURCE_TYPES_LIST:
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyList, json_rules_filename, std::to_string(*rule_index_),
          kResourceTypesKey);
      break;
    case ParseResult::ERROR_EMPTY_URL_FILTER:
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyUrlFilter, json_rules_filename,
          std::to_string(*rule_index_), kUrlFilterKey);
      break;
    case ParseResult::ERROR_INVALID_REDIRECT_URL:
      error = ErrorUtils::FormatErrorMessage(
          kErrorInvalidRedirectUrl, json_rules_filename,
          std::to_string(*rule_index_), kRedirectUrlKey);
      break;
    case ParseResult::ERROR_DUPLICATE_IDS:
      error = ErrorUtils::FormatErrorMessage(kErrorDuplicateIDs,
                                             json_rules_filename,
                                             std::to_string(*rule_index_));
      break;
    case ParseResult::ERROR_PERSISTING_RULESET:
      error =
          ErrorUtils::FormatErrorMessage(kErrorPersisting, json_rules_filename);
      break;
  }
  return error;
}

}  // namespace declarative_net_request
}  // namespace extensions
