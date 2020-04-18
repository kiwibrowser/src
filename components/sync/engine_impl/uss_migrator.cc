// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/uss_migrator.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "components/sync/base/time.h"
#include "components/sync/engine_impl/model_type_worker.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/entry.h"
#include "components/sync/syncable/read_node.h"
#include "components/sync/syncable/read_transaction.h"
#include "components/sync/syncable/user_share.h"

namespace syncer {

namespace {

bool ExtractSyncEntity(ReadTransaction* trans,
                       int64_t id,
                       sync_pb::SyncEntity* entity) {
  ReadNode read_node(trans);
  if (read_node.InitByIdLookup(id) != BaseNode::INIT_OK)
    return false;

  const syncable::Entry& entry = *read_node.GetEntry();

  // Copy the fields USS cares about from the server side of the directory so
  // that we don't miss things that haven't been applied yet. See
  // ModelTypeWorker::ProcessGetUpdatesResponse for which fields are used.
  entity->set_id_string(entry.GetId().GetServerId());
  entity->set_version(entry.GetServerVersion());
  entity->set_mtime(TimeToProtoTime(entry.GetServerMtime()));
  entity->set_ctime(TimeToProtoTime(entry.GetServerCtime()));
  entity->set_name(entry.GetServerNonUniqueName());
  entity->set_deleted(entry.GetServerIsDel());
  entity->set_client_defined_unique_tag(entry.GetUniqueClientTag());

  // It looks like there are fancy other ways to get e.g. passwords specifics
  // out of Entry. Do we need to special-case them when we ship those types?
  entity->mutable_specifics()->CopyFrom(entry.GetServerSpecifics());
  return true;
}

}  // namespace

bool MigrateDirectoryData(ModelType type,
                          UserShare* user_share,
                          ModelTypeWorker* worker) {
  return MigrateDirectoryDataWithBatchSize(type, user_share, worker, 64);
}

bool MigrateDirectoryDataWithBatchSize(ModelType type,
                                       UserShare* user_share,
                                       ModelTypeWorker* worker,
                                       int batch_size) {
  DCHECK_NE(BOOKMARKS, type);
  DCHECK_NE(PASSWORDS, type);
  ReadTransaction trans(FROM_HERE, user_share);

  ReadNode root(&trans);
  if (root.InitTypeRoot(type) != BaseNode::INIT_OK) {
    LOG(ERROR) << "Missing root node for " << ModelTypeToString(type);
    // Inform the worker so it can trigger a fallback initial GetUpdates.
    worker->AbortMigration();
    return false;
  }

  // Get the progress marker and context from the directory.
  sync_pb::DataTypeProgressMarker progress;
  sync_pb::DataTypeContext context;
  user_share->directory->GetDownloadProgress(type, &progress);
  user_share->directory->GetDataTypeContext(trans.GetWrappedTrans(), type,
                                            &context);

  std::vector<int64_t> child_ids;
  root.GetChildIds(&child_ids);

  // Process |batch_size| entities at a time to reduce memory usage.
  size_t i = 0;
  while (i < child_ids.size()) {
    // Vector to own the temporary entities.
    std::vector<std::unique_ptr<sync_pb::SyncEntity>> entities;
    // Vector of raw pointers for passing to ProcessGetUpdatesResponse().
    SyncEntityList entity_ptrs;

    const size_t batch_limit = std::min(i + batch_size, child_ids.size());
    for (; i < batch_limit; i++) {
      auto entity = std::make_unique<sync_pb::SyncEntity>();
      if (!ExtractSyncEntity(&trans, child_ids[i], entity.get())) {
        LOG(ERROR) << "Failed to fetch child node for "
                   << ModelTypeToString(type);
        // Inform the worker so it can clear any partial data and trigger a
        // fallback initial GetUpdates.
        worker->AbortMigration();
        return false;
      }
      // Ignore tombstones; they are not included for initial GetUpdates.
      if (!entity->deleted()) {
        entity_ptrs.push_back(entity.get());
        entities.push_back(std::move(entity));
      }
    }

    worker->ProcessGetUpdatesResponse(progress, context, entity_ptrs, nullptr);
  }

  worker->PassiveApplyUpdates(nullptr);
  return true;
}

}  // namespace syncer
