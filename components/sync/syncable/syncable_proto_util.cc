// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/syncable_proto_util.h"

#include "components/sync/protocol/sync.pb.h"

namespace syncer {

syncable::Id SyncableIdFromProto(const std::string& proto_string) {
  return syncable::Id::CreateFromServerId(proto_string);
}

std::string SyncableIdToProto(const syncable::Id& syncable_id) {
  return syncable_id.GetServerId();
}

bool IsFolder(const sync_pb::SyncEntity& entity) {
  // TODO(sync): The checks for has_folder() and has_bookmarkdata() are likely
  // no longer necessary.  We should remove them if we can convince ourselves
  // that doing so won't break anything.
  return (
      (entity.has_folder() && entity.folder()) ||
      (entity.has_bookmarkdata() && entity.bookmarkdata().bookmark_folder()));
}

bool IsRoot(const sync_pb::SyncEntity& entity) {
  return SyncableIdFromProto(entity.id_string()).IsRoot();
}

}  // namespace syncer
