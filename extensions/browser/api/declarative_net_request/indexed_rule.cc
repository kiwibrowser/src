// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/indexed_rule.h"

#include <algorithm>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "components/url_pattern_index/url_pattern_index.h"
#include "extensions/browser/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "url/gurl.h"

namespace extensions {
namespace declarative_net_request {

namespace {

namespace flat_rule = url_pattern_index::flat;
namespace dnr_api = extensions::api::declarative_net_request;

// Returns true if bitmask |sub| is a subset of |super|.
constexpr bool IsSubset(unsigned sub, unsigned super) {
  return (super | sub) == super;
}

// Helper class to parse the url filter of a Declarative Net Request API rule.
class UrlFilterParser {
 public:
  // This sets the |url_pattern_type|, |anchor_left|, |anchor_right| and
  // |url_pattern| fields on the |indexed_rule_|.
  static void Parse(std::unique_ptr<std::string> url_filter,
                    IndexedRule* indexed_rule) {
    DCHECK(indexed_rule);
    UrlFilterParser(url_filter ? std::move(*url_filter) : std::string(),
                    indexed_rule)
        .ParseImpl();
  }

 private:
  UrlFilterParser(std::string url_filter, IndexedRule* indexed_rule)
      : url_filter_(std::move(url_filter)),
        url_filter_len_(url_filter_.length()),
        index_(0),
        indexed_rule_(indexed_rule) {}

  void ParseImpl() {
    ParseLeftAnchor();
    DCHECK_LE(index_, 2u);

    ParseFilterString();
    DCHECK(index_ == url_filter_len_ || index_ + 1 == url_filter_len_);

    ParseRightAnchor();
    DCHECK_EQ(url_filter_len_, index_);
  }

  void ParseLeftAnchor() {
    indexed_rule_->anchor_left = flat_rule::AnchorType_NONE;

    if (IsAtAnchor()) {
      ++index_;
      indexed_rule_->anchor_left = flat_rule::AnchorType_BOUNDARY;
      if (IsAtAnchor()) {
        ++index_;
        indexed_rule_->anchor_left = flat_rule::AnchorType_SUBDOMAIN;
      }
    }
  }

  void ParseFilterString() {
    indexed_rule_->url_pattern_type = flat_rule::UrlPatternType_SUBSTRING;
    size_t left_index = index_;
    while (index_ < url_filter_len_ && !IsAtRightAnchor()) {
      if (IsAtSeparatorOrWildcard())
        indexed_rule_->url_pattern_type = flat_rule::UrlPatternType_WILDCARDED;
      ++index_;
    }
    // Note: Empty url patterns are supported.
    indexed_rule_->url_pattern =
        url_filter_.substr(left_index, index_ - left_index);
  }

  void ParseRightAnchor() {
    indexed_rule_->anchor_right = flat_rule::AnchorType_NONE;
    if (IsAtRightAnchor()) {
      ++index_;
      indexed_rule_->anchor_right = flat_rule::AnchorType_BOUNDARY;
    }
  }

  bool IsAtSeparatorOrWildcard() const {
    return IsAtValidIndex() && (url_filter_[index_] == kSeparatorCharacter ||
                                url_filter_[index_] == kWildcardCharacter);
  }

  bool IsAtRightAnchor() const {
    return IsAtAnchor() && index_ > 0 && index_ + 1 == url_filter_len_;
  }

  bool IsAtValidIndex() const { return index_ < url_filter_len_; }

  bool IsAtAnchor() const {
    return IsAtValidIndex() && url_filter_[index_] == kAnchorCharacter;
  }

  static constexpr char kAnchorCharacter = '|';
  static constexpr char kSeparatorCharacter = '^';
  static constexpr char kWildcardCharacter = '*';

  const std::string url_filter_;
  const size_t url_filter_len_;
  size_t index_;
  IndexedRule* indexed_rule_;  // Must outlive this instance.

  DISALLOW_COPY_AND_ASSIGN(UrlFilterParser);
};

// Returns a bitmask of flat_rule::OptionFlag corresponding to |parsed_rule|.
uint8_t GetOptionsMask(const dnr_api::Rule& parsed_rule) {
  uint8_t mask = flat_rule::OptionFlag_NONE;

  if (parsed_rule.action.type == dnr_api::RULE_ACTION_TYPE_WHITELIST)
    mask |= flat_rule::OptionFlag_IS_WHITELIST;
  if (parsed_rule.condition.is_url_filter_case_sensitive &&
      *parsed_rule.condition.is_url_filter_case_sensitive) {
    mask |= flat_rule::OptionFlag_IS_MATCH_CASE;
  }

  switch (parsed_rule.condition.domain_type) {
    case dnr_api::DOMAIN_TYPE_FIRSTPARTY:
      mask |= flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY;
      break;
    case dnr_api::DOMAIN_TYPE_THIRDPARTY:
      mask |= flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY;
      break;
    case dnr_api::DOMAIN_TYPE_NONE:
      mask |= (flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY |
               flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY);
      break;
  }
  return mask;
}

uint8_t GetActivationTypes(const dnr_api::Rule& parsed_rule) {
  // Extensions don't use any activation types currently.
  return flat_rule::ActivationType_NONE;
}

flat_rule::ElementType GetElementType(dnr_api::ResourceType resource_type) {
  switch (resource_type) {
    case dnr_api::RESOURCE_TYPE_NONE:
      return flat_rule::ElementType_NONE;
    case dnr_api::RESOURCE_TYPE_MAIN_FRAME:
      return flat_rule::ElementType_MAIN_FRAME;
    case dnr_api::RESOURCE_TYPE_SUB_FRAME:
      return flat_rule::ElementType_SUBDOCUMENT;
    case dnr_api::RESOURCE_TYPE_STYLESHEET:
      return flat_rule::ElementType_STYLESHEET;
    case dnr_api::RESOURCE_TYPE_SCRIPT:
      return flat_rule::ElementType_SCRIPT;
    case dnr_api::RESOURCE_TYPE_IMAGE:
      return flat_rule::ElementType_IMAGE;
    case dnr_api::RESOURCE_TYPE_FONT:
      return flat_rule::ElementType_FONT;
    case dnr_api::RESOURCE_TYPE_OBJECT:
      return flat_rule::ElementType_OBJECT;
    case dnr_api::RESOURCE_TYPE_XMLHTTPREQUEST:
      return flat_rule::ElementType_XMLHTTPREQUEST;
    case dnr_api::RESOURCE_TYPE_PING:
      return flat_rule::ElementType_PING;
    case dnr_api::RESOURCE_TYPE_CSP_REPORT:
      return flat_rule::ElementType_CSP_REPORT;
    case dnr_api::RESOURCE_TYPE_MEDIA:
      return flat_rule::ElementType_MEDIA;
    case dnr_api::RESOURCE_TYPE_WEBSOCKET:
      return flat_rule::ElementType_WEBSOCKET;
    case dnr_api::RESOURCE_TYPE_OTHER:
      return flat_rule::ElementType_OTHER;
  }
  NOTREACHED();
  return flat_rule::ElementType_NONE;
}

// Returns a bitmask of flat_rule::ElementType corresponding to passed
// |resource_types|.
uint16_t GetResourceTypesMask(
    const std::vector<dnr_api::ResourceType>* resource_types) {
  uint16_t mask = flat_rule::ElementType_NONE;
  if (!resource_types)
    return mask;

  for (const auto resource_type : *resource_types)
    mask |= GetElementType(resource_type);
  return mask;
}

// Computes the bitmask of flat_rule::ElementType taking into consideration
// the included and excluded resource types for |condition|.
ParseResult ComputeElementTypes(const dnr_api::RuleCondition& condition,
                                uint16_t* element_types) {
  uint16_t include_element_type_mask =
      GetResourceTypesMask(condition.resource_types.get());
  uint16_t exclude_element_type_mask =
      GetResourceTypesMask(condition.excluded_resource_types.get());

  // OBJECT_SUBREQUEST is not used by Extensions.
  if (exclude_element_type_mask ==
      (flat_rule::ElementType_ANY &
       ~flat_rule::ElementType_OBJECT_SUBREQUEST)) {
    return ParseResult::ERROR_NO_APPLICABLE_RESOURCE_TYPES;
  }

  if (include_element_type_mask & exclude_element_type_mask)
    return ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED;

  if (include_element_type_mask != flat_rule::ElementType_NONE)
    *element_types = include_element_type_mask;
  else if (exclude_element_type_mask != flat_rule::ElementType_NONE)
    *element_types = flat_rule::ElementType_ANY & ~exclude_element_type_mask;
  else
    *element_types = url_pattern_index::kDefaultFlatElementTypesMask;

  return ParseResult::SUCCESS;
}

// Lower-cases and sorts domains, as required by the url_pattern_index
// component.
std::vector<std::string> CanonicalizeDomains(
    std::unique_ptr<std::vector<std::string>> domains) {
  if (!domains)
    return std::vector<std::string>();

  // Convert to lower case as required by the url_pattern_index component.
  for (size_t i = 0; i < domains->size(); i++)
    (*domains)[i] = base::ToLowerASCII((*domains)[i]);

  std::sort(domains->begin(), domains->end(),
            [](const std::string& left, const std::string& right) {
              return url_pattern_index::CompareDomains(left, right) < 0;
            });

  // Move the vector, because it isn't eligible for return value optimization.
  return std::move(*domains);
}

}  // namespace

IndexedRule::IndexedRule() = default;
IndexedRule::~IndexedRule() = default;
IndexedRule::IndexedRule(IndexedRule&& other) = default;
IndexedRule& IndexedRule::operator=(IndexedRule&& other) = default;

// static
ParseResult IndexedRule::CreateIndexedRule(
    std::unique_ptr<dnr_api::Rule> parsed_rule,
    IndexedRule* indexed_rule) {
  DCHECK(indexed_rule);
  DCHECK(IsAPIAvailable());

  if (parsed_rule->id < kMinValidID)
    return ParseResult::ERROR_INVALID_RULE_ID;

  const bool is_redirect_rule =
      parsed_rule->action.type == dnr_api::RULE_ACTION_TYPE_REDIRECT;
  if (is_redirect_rule) {
    if (!parsed_rule->action.redirect_url ||
        parsed_rule->action.redirect_url->empty()) {
      return ParseResult::ERROR_EMPTY_REDIRECT_URL;
    }
    if (!GURL(*parsed_rule->action.redirect_url).is_valid())
      return ParseResult::ERROR_INVALID_REDIRECT_URL;
    if (!parsed_rule->priority)
      return ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY;
    if (*parsed_rule->priority < kMinValidPriority)
      return ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY;
  }

  if (parsed_rule->condition.domains && parsed_rule->condition.domains->empty())
    return ParseResult::ERROR_EMPTY_DOMAINS_LIST;

  if (parsed_rule->condition.resource_types &&
      parsed_rule->condition.resource_types->empty()) {
    return ParseResult::ERROR_EMPTY_RESOURCE_TYPES_LIST;
  }

  if (parsed_rule->condition.url_filter &&
      parsed_rule->condition.url_filter->empty()) {
    return ParseResult::ERROR_EMPTY_URL_FILTER;
  }

  indexed_rule->id = base::checked_cast<uint32_t>(parsed_rule->id);
  indexed_rule->priority = base::checked_cast<uint32_t>(
      is_redirect_rule ? *parsed_rule->priority : kDefaultPriority);
  indexed_rule->options = GetOptionsMask(*parsed_rule);
  indexed_rule->activation_types = GetActivationTypes(*parsed_rule);

  {
    ParseResult result = ComputeElementTypes(parsed_rule->condition,
                                             &indexed_rule->element_types);
    if (result != ParseResult::SUCCESS)
      return result;
  }

  indexed_rule->domains =
      CanonicalizeDomains(std::move(parsed_rule->condition.domains));
  indexed_rule->excluded_domains =
      CanonicalizeDomains(std::move(parsed_rule->condition.excluded_domains));

  if (is_redirect_rule)
    indexed_rule->redirect_url = std::move(*parsed_rule->action.redirect_url);

  // Parse the |anchor_left|, |anchor_right|, |url_pattern_type| and
  // |url_pattern| fields.
  UrlFilterParser::Parse(std::move(parsed_rule->condition.url_filter),
                         indexed_rule);

  // Some sanity checks to ensure we return a valid IndexedRule.
  DCHECK_GE(indexed_rule->id, static_cast<uint32_t>(kMinValidID));
  DCHECK_GE(indexed_rule->priority, static_cast<uint32_t>(kMinValidPriority));
  DCHECK(IsSubset(indexed_rule->options, flat_rule::OptionFlag_ANY));
  DCHECK(IsSubset(indexed_rule->element_types, flat_rule::ElementType_ANY));
  DCHECK_EQ(flat_rule::ActivationType_NONE, indexed_rule->activation_types);
  DCHECK_NE(flat_rule::UrlPatternType_REGEXP, indexed_rule->url_pattern_type);
  DCHECK_NE(flat_rule::AnchorType_SUBDOMAIN, indexed_rule->anchor_right);

  return ParseResult::SUCCESS;
}

}  // namespace declarative_net_request
}  // namespace extensions
