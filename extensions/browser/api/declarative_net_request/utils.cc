// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/utils.h"

#include <memory>
#include <set>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/hash.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/elapsed_timer.h"
#include "base/values.h"
#include "extensions/browser/api/declarative_net_request/constants.h"
#include "extensions/browser/api/declarative_net_request/flat/extension_ruleset_generated.h"
#include "extensions/browser/api/declarative_net_request/flat_ruleset_indexer.h"
#include "extensions/browser/api/declarative_net_request/indexed_rule.h"
#include "extensions/browser/api/declarative_net_request/parse_info.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/api/declarative_net_request/dnr_manifest_data.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "extensions/common/file_util.h"
#include "extensions/common/install_warning.h"
#include "extensions/common/manifest_constants.h"
#include "third_party/flatbuffers/src/include/flatbuffers/flatbuffers.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace dnr_api = extensions::api::declarative_net_request;

// Returns the checksum of the given serialized |data|.
int GetChecksum(const FlatRulesetIndexer::SerializedData& data) {
  uint32_t hash = base::PersistentHash(data.first, data.second);

  // Strip off the sign bit since this needs to be persisted in preferences
  // which don't support unsigned ints.
  return static_cast<int>(hash & 0x7fffffff);
}

// Helper function to persist the indexed ruleset |data| for |extension|.
bool PersistRuleset(const Extension& extension,
                    const FlatRulesetIndexer::SerializedData& data,
                    int* ruleset_checksum) {
  DCHECK(ruleset_checksum);

  const base::FilePath path =
      file_util::GetIndexedRulesetPath(extension.path());

  // Create the directory corresponding to |path| if it does not exist and then
  // persist the ruleset.
  const int data_size = base::checked_cast<int>(data.second);
  const bool success =
      base::CreateDirectory(path.DirName()) &&
      base::WriteFile(path, reinterpret_cast<const char*>(data.first),
                      data_size) == data_size;

  if (success)
    *ruleset_checksum = GetChecksum(data);

  return success;
}

// Helper to retrieve the ruleset ExtensionResource for |extension|.
const ExtensionResource* GetRulesetResource(const Extension& extension) {
  return declarative_net_request::DNRManifestData::GetRulesetResource(
      &extension);
}

// Helper to retrieve the filename of the JSON ruleset provided by |extension|.
std::string GetJSONRulesetFilename(const Extension& extension) {
  base::AssertBlockingAllowed();
  return GetRulesetResource(extension)->GetFilePath().BaseName().AsUTF8Unsafe();
}

// Helper function to index |rules| and persist them to the
// |indexed_ruleset_path|.
ParseInfo IndexAndPersistRulesImpl(const base::ListValue& rules,
                                   const Extension& extension,
                                   std::vector<InstallWarning>* warnings,
                                   int* ruleset_checksum) {
  base::AssertBlockingAllowed();

  FlatRulesetIndexer indexer;
  bool all_rules_parsed = true;
  base::ElapsedTimer timer;
  {
    std::set<int> id_set;  // Ensure all ids are distinct.
    std::unique_ptr<dnr_api::Rule> parsed_rule;

    const auto& rules_list = rules.GetList();
    for (size_t i = 0; i < rules_list.size(); i++) {
      parsed_rule = dnr_api::Rule::FromValue(rules_list[i]);

      // Ignore rules which can't be successfully parsed and show an install
      // warning for them.
      if (!parsed_rule) {
        all_rules_parsed = false;
        continue;
      }

      bool inserted = id_set.insert(parsed_rule->id).second;
      if (!inserted)
        return ParseInfo(ParseResult::ERROR_DUPLICATE_IDS, i);

      IndexedRule indexed_rule;
      ParseResult parse_result =
          IndexedRule::CreateIndexedRule(std::move(parsed_rule), &indexed_rule);
      if (parse_result != ParseResult::SUCCESS)
        return ParseInfo(parse_result, i);

      indexer.AddUrlRule(indexed_rule);
    }
  }
  indexer.Finish();
  UMA_HISTOGRAM_TIMES(kIndexRulesTimeHistogram, timer.Elapsed());

  // The actual data buffer is still owned by |indexer|.
  const FlatRulesetIndexer::SerializedData data = indexer.GetData();
  if (!PersistRuleset(extension, data, ruleset_checksum))
    return ParseInfo(ParseResult::ERROR_PERSISTING_RULESET);

  if (!all_rules_parsed && warnings) {
    warnings->push_back(InstallWarning(
        kRulesNotParsedWarning, manifest_keys::kDeclarativeNetRequestKey,
        manifest_keys::kDeclarativeRuleResourcesKey));
  }

  UMA_HISTOGRAM_TIMES(kIndexAndPersistRulesTimeHistogram, timer.Elapsed());
  UMA_HISTOGRAM_COUNTS_100000(kManifestRulesCountHistogram,
                              indexer.indexed_rules_count());

  return ParseInfo(ParseResult::SUCCESS);
}

}  // namespace

bool IndexAndPersistRules(const base::ListValue& rules,
                          const Extension& extension,
                          std::string* error,
                          std::vector<InstallWarning>* warnings,
                          int* ruleset_checksum) {
  DCHECK(IsAPIAvailable());
  DCHECK(GetRulesetResource(extension));
  DCHECK(ruleset_checksum);
  base::AssertBlockingAllowed();

  const ParseInfo info =
      IndexAndPersistRulesImpl(rules, extension, warnings, ruleset_checksum);
  if (info.result() == ParseResult::SUCCESS)
    return true;

  if (error)
    *error = info.GetErrorDescription(GetJSONRulesetFilename(extension));
  return false;
}

bool IsValidRulesetData(const uint8_t* data,
                        size_t size,
                        int expected_checksum) {
  flatbuffers::Verifier verifier(data, size);
  FlatRulesetIndexer::SerializedData serialized_data(data, size);
  return expected_checksum == GetChecksum(serialized_data) &&
         flat::VerifyExtensionIndexedRulesetBuffer(verifier);
}

}  // namespace declarative_net_request
}  // namespace extensions
