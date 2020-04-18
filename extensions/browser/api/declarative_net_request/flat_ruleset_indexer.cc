// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/flat_ruleset_indexer.h"

#include <string>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "extensions/browser/api/declarative_net_request/indexed_rule.h"

namespace extensions {
namespace declarative_net_request {

namespace {

namespace dnr_api = extensions::api::declarative_net_request;
namespace flat_rule = url_pattern_index::flat;

template <typename T>
using FlatOffset = flatbuffers::Offset<T>;
using FlatStringOffset = FlatOffset<flatbuffers::String>;
using FlatStringListOffset = FlatOffset<flatbuffers::Vector<FlatStringOffset>>;

// Writes to |builder| a flatbuffer vector of shared strings corresponding to
// |vec| and returns the offset to it. If |vec| is empty, returns an empty
// offset.
FlatStringListOffset BuildVectorOfSharedStrings(
    flatbuffers::FlatBufferBuilder* builder,
    const std::vector<std::string>& vec) {
  if (vec.empty())
    return FlatStringListOffset();

  std::vector<FlatStringOffset> offsets;
  offsets.reserve(vec.size());
  for (const auto& str : vec)
    offsets.push_back(builder->CreateSharedString(str));
  return builder->CreateVector(offsets);
}

// Returns the RuleActionType corresponding to |indexed_rule|.
dnr_api::RuleActionType GetRuleActionType(const IndexedRule& indexed_rule) {
  if (indexed_rule.options & flat_rule::OptionFlag_IS_WHITELIST) {
    DCHECK(indexed_rule.redirect_url.empty());
    return dnr_api::RULE_ACTION_TYPE_WHITELIST;
  }
  if (!indexed_rule.redirect_url.empty())
    return dnr_api::RULE_ACTION_TYPE_REDIRECT;
  return dnr_api::RULE_ACTION_TYPE_BLACKLIST;
}

}  // namespace

FlatRulesetIndexer::FlatRulesetIndexer()
    : blacklist_index_builder_(&builder_),
      whitelist_index_builder_(&builder_),
      redirect_index_builder_(&builder_) {}

FlatRulesetIndexer::~FlatRulesetIndexer() = default;

void FlatRulesetIndexer::AddUrlRule(const IndexedRule& indexed_rule) {
  DCHECK(!finished_);

  ++indexed_rules_count_;

  auto domains_included_offset =
      BuildVectorOfSharedStrings(&builder_, indexed_rule.domains);
  auto domains_excluded_offset =
      BuildVectorOfSharedStrings(&builder_, indexed_rule.excluded_domains);
  auto url_pattern_offset =
      builder_.CreateSharedString(indexed_rule.url_pattern);

  FlatOffset<flat_rule::UrlRule> offset = flat_rule::CreateUrlRule(
      builder_, indexed_rule.options, indexed_rule.element_types,
      indexed_rule.activation_types, indexed_rule.url_pattern_type,
      indexed_rule.anchor_left, indexed_rule.anchor_right,
      domains_included_offset, domains_excluded_offset, url_pattern_offset,
      indexed_rule.id, indexed_rule.priority);
  const dnr_api::RuleActionType type = GetRuleActionType(indexed_rule);
  GetBuilder(type)->IndexUrlRule(offset);

  // Store additional metadata required for a redirect rule.
  if (type == dnr_api::RULE_ACTION_TYPE_REDIRECT) {
    DCHECK(!indexed_rule.redirect_url.empty());
    auto redirect_url_offset =
        builder_.CreateSharedString(indexed_rule.redirect_url);
    metadata_.push_back(flat::CreateUrlRuleMetadata(builder_, indexed_rule.id,
                                                    redirect_url_offset));
  }
}

void FlatRulesetIndexer::Finish() {
  DCHECK(!finished_);
  finished_ = true;

  auto blacklist_index_offset = blacklist_index_builder_.Finish();
  auto whitelist_index_offset = whitelist_index_builder_.Finish();
  auto redirect_index_offset = redirect_index_builder_.Finish();

  // Store the extension metadata sorted by ID to support fast lookup through
  // binary search.
  auto extension_metadata_offset =
      builder_.CreateVectorOfSortedTables(&metadata_);

  auto root_offset = flat::CreateExtensionIndexedRuleset(
      builder_, blacklist_index_offset, whitelist_index_offset,
      redirect_index_offset, extension_metadata_offset);
  flat::FinishExtensionIndexedRulesetBuffer(builder_, root_offset);
}

FlatRulesetIndexer::SerializedData FlatRulesetIndexer::GetData() {
  DCHECK(finished_);
  return SerializedData(builder_.GetBufferPointer(),
                        base::strict_cast<size_t>(builder_.GetSize()));
}

FlatRulesetIndexer::UrlPatternIndexBuilder* FlatRulesetIndexer::GetBuilder(
    dnr_api::RuleActionType type) {
  switch (type) {
    case dnr_api::RULE_ACTION_TYPE_BLACKLIST:
      return &blacklist_index_builder_;
    case dnr_api::RULE_ACTION_TYPE_WHITELIST:
      return &whitelist_index_builder_;
    case dnr_api::RULE_ACTION_TYPE_REDIRECT:
      return &redirect_index_builder_;
    case dnr_api::RULE_ACTION_TYPE_NONE:
      NOTREACHED();
  }
  return nullptr;
}

}  // namespace declarative_net_request
}  // namespace extensions
