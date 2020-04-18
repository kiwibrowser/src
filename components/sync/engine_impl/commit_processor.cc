// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/commit_processor.h"

#include <memory>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "components/sync/engine_impl/commit_contribution.h"
#include "components/sync/engine_impl/commit_contributor.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

using TypeToIndexMap = std::map<ModelType, size_t>;

CommitProcessor::CommitProcessor(CommitContributorMap* commit_contributor_map)
    : commit_contributor_map_(commit_contributor_map) {}

CommitProcessor::~CommitProcessor() {}

void CommitProcessor::GatherCommitContributions(
    ModelTypeSet commit_types,
    size_t max_entries,
    bool cookie_jar_mismatch,
    bool cookie_jar_empty,
    Commit::ContributionMap* contributions) {
  size_t num_entries = 0;
  for (ModelTypeSet::Iterator it = commit_types.First(); it.Good(); it.Inc()) {
    CommitContributorMap::iterator cm_it =
        commit_contributor_map_->find(it.Get());
    if (cm_it == commit_contributor_map_->end()) {
      DLOG(ERROR) << "Could not find requested type "
                  << ModelTypeToString(it.Get()) << " in contributor map.";
      continue;
    }
    size_t spaces_remaining = max_entries - num_entries;
    std::unique_ptr<CommitContribution> contribution =
        cm_it->second->GetContribution(spaces_remaining);
    if (contribution) {
      num_entries += contribution->GetNumEntries();
      contributions->insert(std::make_pair(it.Get(), std::move(contribution)));

      if (it.Get() == SESSIONS) {
        UMA_HISTOGRAM_BOOLEAN("Sync.CookieJarMatchOnNavigation",
                              !cookie_jar_mismatch);
        if (cookie_jar_mismatch) {
          UMA_HISTOGRAM_BOOLEAN("Sync.CookieJarEmptyOnMismatch",
                                cookie_jar_empty);
        }
      }
    }
    if (num_entries >= max_entries) {
      DCHECK_EQ(num_entries, max_entries)
          << "Number of commit entries exceeeds maximum";
      break;
    }
  }
}

}  // namespace syncer
