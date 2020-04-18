// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_PROTO_UTIL_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_PROTO_UTIL_H_

#include <string>

#include "components/sync/syncable/syncable_id.h"

namespace sync_pb {
class SyncEntity;
}  // namespace sync_pb

namespace syncer {

// Converts from a specially formatted string field to a syncable::Id.  Used
// when interpreting the fields of protocol buffers received from the server.
syncable::Id SyncableIdFromProto(const std::string& proto_string);

// Converts from a syncable::Id to a formatted std::string.  This is useful for
// populating the fields of a protobuf which will be sent to the server.
std::string SyncableIdToProto(const syncable::Id& syncable_id);

// Helper function to determine if this SyncEntity's properties indicate that it
// is a folder.
bool IsFolder(const sync_pb::SyncEntity& entity);

// Helper function to determine if this SyncEntity's properties indicate that it
// is the root node.
bool IsRoot(const sync_pb::SyncEntity& entity);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_PROTO_UTIL_H_
