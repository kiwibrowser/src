// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_DIRECTORY_COMMIT_CONTRIBUTION_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_DIRECTORY_COMMIT_CONTRIBUTION_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/syncer_error.h"
#include "components/sync/engine_impl/commit_contribution.h"
#include "components/sync/engine_impl/cycle/data_type_debug_info_emitter.h"
#include "components/sync/engine_impl/cycle/status_controller.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

class StatusController;

namespace syncable {
class Directory;
}  // namespace syncable

// This class represents a set of items belonging to a particular data type that
// have been selected from the syncable Directory and prepared for commit.
//
// This class handles the bookkeeping related to the commit of these items,
// including processing the commit response message and setting and unsetting
// the SYNCING bits.
class DirectoryCommitContribution : public CommitContribution {
 public:
  // This destructor will DCHECK if UnsetSyncingBits() has not been called yet.
  ~DirectoryCommitContribution() override;

  // Build a CommitContribution from the IS_UNSYNCED items in |dir| with the
  // given |type|.  The contribution will include at most |max_items| entries.
  //
  // This function may return null if this type has no items ready for and
  // requiring commit.  This function may make model neutral changes to the
  // directory.
  static std::unique_ptr<DirectoryCommitContribution> Build(
      syncable::Directory* dir,
      ModelType type,
      size_t max_items,
      DataTypeDebugInfoEmitter* debug_info_emitter);

  // Serialize this contribution's entries to the given commit request |msg|.
  //
  // This function is not const.  It will update some state in this contribution
  // that will be used when processing the associated commit response.  This
  // function should not be called more than once.
  void AddToCommitMessage(sync_pb::ClientToServerMessage* msg) override;

  // Updates this contribution's contents in accordance with the provided
  // |response|.
  //
  // This function may make model-neutral changes to the directory.  It is not
  // valid to call this function unless AddToCommitMessage() was called earlier.
  // This function should not be called more than once.
  SyncerError ProcessCommitResponse(
      const sync_pb::ClientToServerResponse& response,
      StatusController* status) override;

  // Cleans up any temporary state associated with the commit.  Must be called
  // before destruction.
  void CleanUp() override;

  // Returns the number of entries included in this contribution.
  size_t GetNumEntries() const override;

 private:
  class DirectoryCommitContributionTest;
  FRIEND_TEST_ALL_PREFIXES(DirectoryCommitContributionTest, GatherByTypes);
  FRIEND_TEST_ALL_PREFIXES(DirectoryCommitContributionTest, GatherAndTruncate);

  DirectoryCommitContribution(
      const std::vector<int64_t>& metahandles,
      const google::protobuf::RepeatedPtrField<sync_pb::SyncEntity>& entities,
      const sync_pb::DataTypeContext& context,
      syncable::Directory* directory,
      DataTypeDebugInfoEmitter* debug_info_emitter);

  void UnsetSyncingBits();

  syncable::Directory* dir_;
  const std::vector<int64_t> metahandles_;
  const google::protobuf::RepeatedPtrField<sync_pb::SyncEntity> entities_;
  sync_pb::DataTypeContext context_;
  size_t entries_start_index_;

  // This flag is tracks whether or not the directory entries associated with
  // this commit still have their SYNCING bits set.  These bits will be set when
  // the CommitContribution is created with Build() and unset when CleanUp() is
  // called.  This flag must be unset by the time our destructor is called.
  bool syncing_bits_set_;

  // A pointer to the commit counters of our parent CommitContributor.
  DataTypeDebugInfoEmitter* debug_info_emitter_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryCommitContribution);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_DIRECTORY_COMMIT_CONTRIBUTION_H_
