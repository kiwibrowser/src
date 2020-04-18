// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_TEST_ENTRY_FACTORY_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_TEST_ENTRY_FACTORY_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

namespace syncable {
class Directory;
class Id;
class MutableEntry;
}

class TestEntryFactory {
 public:
  explicit TestEntryFactory(syncable::Directory* dir);
  ~TestEntryFactory();

  // Create a new unapplied folder node with a parent.
  int64_t CreateUnappliedNewItemWithParent(
      const std::string& item_id,
      const sync_pb::EntitySpecifics& specifics,
      const std::string& parent_id);

  int64_t CreateUnappliedNewBookmarkItemWithParent(
      const std::string& item_id,
      const sync_pb::EntitySpecifics& specifics,
      const std::string& parent_id);

  // Create a new unapplied update without a parent.
  int64_t CreateUnappliedNewItem(const std::string& item_id,
                                 const sync_pb::EntitySpecifics& specifics,
                                 bool is_unique);

  // Create an unsynced unique_client_tag item in the database.  If item_id is a
  // local ID, it will be treated as a create-new.  Otherwise, if it's a server
  // ID, we'll fake the server data so that it looks like it exists on the
  // server.  Returns the methandle of the created item in |metahandle_out| if
  // not null.
  void CreateUnsyncedItem(const syncable::Id& item_id,
                          const syncable::Id& parent_id,
                          const std::string& name,
                          bool is_folder,
                          ModelType model_type,
                          int64_t* metahandle_out);

  // Creates a bookmark that is both unsynced an an unapplied update.  Returns
  // the metahandle of the created item.
  int64_t CreateUnappliedAndUnsyncedBookmarkItem(const std::string& name);

  // Creates a unique_client_tag item that has neither IS_UNSYNCED or
  // IS_UNAPPLIED_UPDATE. The item is known to both the server and client.
  // Returns the metahandle of the created item. |specifics| is optional.
  int64_t CreateSyncedItem(const std::string& name,
                           ModelType model_type,
                           bool is_folder);
  int64_t CreateSyncedItem(const std::string& name,
                           ModelType model_type,
                           bool is_folder,
                           const sync_pb::EntitySpecifics& specifics);

  // Create a tombstone for |name|; returns the metahandle of the entry.
  int64_t CreateTombstone(const std::string& name, ModelType model_type);

  // Creates a root node for |model_type|.
  int64_t CreateTypeRootNode(ModelType model_type);

  // Creates a root node that IS_UNAPPLIED. Similar to what one would find in
  // the database between the ProcessUpdates of an initial datatype configure
  // cycle and the ApplyUpdates step of the same sync cycle.
  int64_t CreateUnappliedRootNode(ModelType model_type);

  // Looks up the item referenced by |meta_handle|. If successful, overwrites
  // the server specifics with |specifics|, sets
  // IS_UNAPPLIED_UPDATES/IS_UNSYNCED appropriately, and returns true.
  // Else, return false.
  bool SetServerSpecificsForItem(int64_t meta_handle,
                                 const sync_pb::EntitySpecifics specifics);

  // Looks up the item referenced by |meta_handle|. If successful, overwrites
  // the local specifics with |specifics|, sets
  // IS_UNAPPLIED_UPDATES/IS_UNSYNCED appropriately, and returns true.
  // Else, return false.
  bool SetLocalSpecificsForItem(int64_t meta_handle,
                                const sync_pb::EntitySpecifics specifics);

  // Looks up the item referenced by |meta_handle| and returns its server
  // specifics.
  const sync_pb::EntitySpecifics& GetServerSpecificsForItem(
      int64_t meta_handle) const;

  // Looks up the item referenced by |meta_handle| and returns its specifics.
  const sync_pb::EntitySpecifics& GetLocalSpecificsForItem(
      int64_t meta_handle) const;

  // Getters for IS_UNSYNCED and IS_UNAPPLIED_UPDATE bit fields.
  bool GetIsUnsyncedForItem(int64_t meta_handle) const;
  bool GetIsUnappliedForItem(int64_t meta_handle) const;

  int64_t GetNextRevision();

 private:
  // Populate an entry with a bunch of default values.
  void PopulateEntry(const syncable::Id& parent_id,
                     const std::string& name,
                     ModelType model_type,
                     syncable::MutableEntry* entry);

  syncable::Directory* directory_;
  int64_t next_revision_;

  DISALLOW_COPY_AND_ASSIGN(TestEntryFactory);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_TEST_ENTRY_FACTORY_H_
