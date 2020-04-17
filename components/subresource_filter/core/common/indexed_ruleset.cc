// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/common/indexed_ruleset.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "components/subresource_filter/core/common/first_party_origin.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace subresource_filter {

namespace {
namespace proto = url_pattern_index::proto;
using FindRuleStrategy =
    url_pattern_index::UrlPatternIndexMatcher::FindRuleStrategy;

// A helper function to get the checksum on a data buffer.
int LocalGetChecksum(const uint8_t* data, size_t size) {
  uint32_t hash = base::PersistentHash(data, size);

  // Strip off the sign bit since this needs to be persisted in preferences
  // which don't support unsigned ints.
  return static_cast<int>(hash & 0x7fffffff);
}

VerifyStatus GetVerifyStatus(const uint8_t* buffer,
                             size_t size,
                             int expected_checksum) {
  // TODO(ericrobinson): Remove the verifier once we've updated the ruleset at
  // least once.  The verifier detects a subset of the errors detected by the
  // checksum, and is unneeded once expected_checksum is consistently nonzero.
  flatbuffers::Verifier verifier(buffer, size);
  if (expected_checksum != 0 &&
      expected_checksum != LocalGetChecksum(buffer, size)) {
    return flat::VerifyIndexedRulesetBuffer(verifier)
               ? VerifyStatus::kChecksumFailVerifierPass
               : VerifyStatus::kChecksumFailVerifierFail;
  }
  if (!flat::VerifyIndexedRulesetBuffer(verifier)) {
    return expected_checksum == 0 ? VerifyStatus::kVerifierFailChecksumZero
                                  : VerifyStatus::kVerifierFailChecksumPass;
  }
  return expected_checksum == 0 ? VerifyStatus::kPassChecksumZero
                                : VerifyStatus::kPassValidChecksum;
}

}  // namespace

// RulesetIndexer --------------------------------------------------------------

// static
// Keep this in sync with the version number in
// tools/perf/core/default_local_state.json.
const int RulesetIndexer::kIndexedFormatVersion = 20;

// This static assert is meant to catch cases where
// url_pattern_index::kUrlPatternIndexFormatVersion is incremented without
// updating RulesetIndexer::kIndexedFormatVersion.
static_assert(url_pattern_index::kUrlPatternIndexFormatVersion == 1,
              "kUrlPatternIndexFormatVersion has changed, make sure you've "
              "also updated RulesetIndexer::kIndexedFormatVersion above.");

RulesetIndexer::RulesetIndexer()
    : blacklist_(&builder_), whitelist_(&builder_), deactivation_(&builder_) {}

RulesetIndexer::~RulesetIndexer() = default;

bool RulesetIndexer::AddUrlRule(const proto::UrlRule& rule) {
  const auto offset = url_pattern_index::SerializeUrlRule(rule, &builder_);
  // Note: A zero offset.o means a "nullptr" offset. It is returned when the
  // rule has not been serialized.
  if (!offset.o)
    return false;

  if (rule.semantics() == proto::RULE_SEMANTICS_BLACKLIST) {
    blacklist_.IndexUrlRule(offset);
  } else {
    const auto* flat_rule = flatbuffers::GetTemporaryPointer(builder_, offset);
    DCHECK(flat_rule);
    if (flat_rule->element_types())
      whitelist_.IndexUrlRule(offset);
    if (flat_rule->activation_types())
      deactivation_.IndexUrlRule(offset);
  }

  return true;
}

void RulesetIndexer::Finish() {
  auto blacklist_offset = blacklist_.Finish();
  auto whitelist_offset = whitelist_.Finish();
  auto deactivation_offset = deactivation_.Finish();

  auto url_rules_index_offset = flat::CreateIndexedRuleset(
      builder_, blacklist_offset, whitelist_offset, deactivation_offset);
  builder_.Finish(url_rules_index_offset);
}

int RulesetIndexer::GetChecksum() const {
  return LocalGetChecksum(data(), size());
}

// IndexedRulesetMatcher -------------------------------------------------------

// static
bool IndexedRulesetMatcher::Verify(const uint8_t* buffer,
                                   size_t size,
                                   int expected_checksum) {
  TRACE_EVENT_BEGIN1(TRACE_DISABLED_BY_DEFAULT("loading"),
                     "IndexedRulesetMatcher::Verify", "size", size);
  SCOPED_UMA_HISTOGRAM_TIMER(
      "SubresourceFilter.IndexRuleset.Verify2.WallDuration");
  VerifyStatus status = GetVerifyStatus(buffer, size, expected_checksum);
  UMA_HISTOGRAM_ENUMERATION("SubresourceFilter.IndexRuleset.Verify.Status",
                            status);
  TRACE_EVENT_END1(TRACE_DISABLED_BY_DEFAULT("loading"),
                   "IndexedRulesetMatcher::Verify", "status",
                   static_cast<int>(status));
  return status == VerifyStatus::kPassValidChecksum ||
         status == VerifyStatus::kPassChecksumZero;
}

IndexedRulesetMatcher::IndexedRulesetMatcher(const uint8_t* buffer, size_t size)
    : root_(flat::GetIndexedRuleset(buffer)),
      blacklist_(root_->blacklist_index()),
      whitelist_(root_->whitelist_index()),
      deactivation_(root_->deactivation_index()) {}

bool IndexedRulesetMatcher::ShouldDisableFilteringForDocument(
    const GURL& document_url,
    const url::Origin& parent_document_origin,
    proto::ActivationType activation_type) const {
  return !!deactivation_.FindMatch(
      document_url, parent_document_origin, proto::ELEMENT_TYPE_UNSPECIFIED,
      activation_type,
      FirstPartyOrigin::IsThirdParty(document_url, parent_document_origin),
      false, FindRuleStrategy::kAny);
}

bool IndexedRulesetMatcher::ShouldDisallowResourceLoad(
    const GURL& url,
    const FirstPartyOrigin& first_party,
    proto::ElementType element_type,
    bool disable_generic_rules) const {
  const url_pattern_index::flat::UrlRule* rule =
      MatchedUrlRule(url, first_party, element_type, disable_generic_rules);
  return rule &&
         !(rule->options() & url_pattern_index::flat::OptionFlag_IS_WHITELIST);
}

const url_pattern_index::flat::UrlRule* IndexedRulesetMatcher::MatchedUrlRule(
    const GURL& url,
    const FirstPartyOrigin& first_party,
    url_pattern_index::proto::ElementType element_type,
    bool disable_generic_rules) const {
  const bool is_third_party = first_party.IsThirdParty(url);

  const url_pattern_index::flat::UrlRule* blacklist_rule =
      blacklist_.FindMatch(url, first_party.origin(), element_type,
                           proto::ACTIVATION_TYPE_UNSPECIFIED, is_third_party,
                           disable_generic_rules, FindRuleStrategy::kAny);
  const url_pattern_index::flat::UrlRule* whitelist_rule = nullptr;
  if (blacklist_rule) {
    whitelist_rule =
        whitelist_.FindMatch(url, first_party.origin(), element_type,
                             proto::ACTIVATION_TYPE_UNSPECIFIED, is_third_party,
                             disable_generic_rules, FindRuleStrategy::kAny);
    return whitelist_rule ? whitelist_rule : blacklist_rule;
  }
  return nullptr;
}

}  // namespace subresource_filter
