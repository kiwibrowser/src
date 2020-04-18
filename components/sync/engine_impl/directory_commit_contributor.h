// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_DIRECTORY_COMMIT_CONTRIBUTOR_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_DIRECTORY_COMMIT_CONTRIBUTOR_H_

#include <stddef.h>

#include <map>
#include <memory>

#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/engine_impl/commit_contributor.h"
#include "components/sync/engine_impl/directory_commit_contribution.h"

namespace syncer {

namespace syncable {
class Directory;
}

class DataTypeDebugInfoEmitter;

// This class represents the syncable::Directory as a source of items to commit
// to the sync server.
//
// Each instance of this class represents a particular type within the
// syncable::Directory.  When asked, it will iterate through the directory, grab
// any items of its type that are ready for commit, and return them in the form
// of a DirectoryCommitContribution.
class DirectoryCommitContributor : public CommitContributor {
 public:
  DirectoryCommitContributor(syncable::Directory* dir,
                             ModelType type,
                             DataTypeDebugInfoEmitter* debug_info_emitter);
  ~DirectoryCommitContributor() override;

  std::unique_ptr<CommitContribution> GetContribution(
      size_t max_entries) override;

 private:
  syncable::Directory* dir_;
  ModelType type_;

  DataTypeDebugInfoEmitter* debug_info_emitter_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryCommitContributor);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_DIRECTORY_COMMIT_CONTRIBUTOR_H_
