// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BOOKMARKS_SYNCED_BOOKMARK_TRACKER_H_
#define COMPONENTS_SYNC_BOOKMARKS_SYNCED_BOOKMARK_TRACKER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "components/sync/protocol/entity_metadata.pb.h"

namespace bookmarks {
class BookmarkNode;
}

namespace syncer {
struct EntityData;
}

namespace sync_bookmarks {

// This class is responsible for keeping the mapping between bookmarks node in
// the local model and the server-side corresponding sync entities. It manages
// the metadata for its entity and caches entity data upon a local change until
// commit confirmation is received.
class SyncedBookmarkTracker {
 public:
  class Entity {
   public:
    // |bookmark_node| must not be null and must outlive this object.
    explicit Entity(const bookmarks::BookmarkNode* bookmark_node);
    ~Entity();

    // Returns true if this data is out of sync with the server.
    // A commit may or may not be in progress at this time.
    bool IsUnsynced() const;

    // Check whether |data| matches the stored specifics hash.
    bool MatchesData(const syncer::EntityData& data) const;

    // It never returns null.
    const bookmarks::BookmarkNode* bookmark_node() const {
      return bookmark_node_;
    }

   private:
    const bookmarks::BookmarkNode* const bookmark_node_;

    DISALLOW_COPY_AND_ASSIGN(Entity);
  };

  SyncedBookmarkTracker();
  ~SyncedBookmarkTracker();

  // Returns null if not entity is found.
  const Entity* GetEntityForSyncId(const std::string& sync_id) const;

  // Associates a server id with the corresponding local bookmark node in
  // |sync_id_to_entities_map_|.
  void Associate(const std::string& sync_id,
                 const bookmarks::BookmarkNode* bookmark_node);

  // Removes the association that corresponds to |sync_id| from
  // |sync_id_to_entities_map_|.
  void Disassociate(const std::string& sync_id);

  // Returns number of tracked entities. Used only in test.
  std::size_t TrackedEntitiesCountForTest() const;

 private:
  // A map of sync server ids to sync entities. This should contain entries and
  // metadata for almost everything. However, since local data are loaded only
  // when needed (e.g. before a commit cycle), the entities may not always
  // contain model type data/specifics.
  std::map<std::string, std::unique_ptr<Entity>> sync_id_to_entities_map_;

  DISALLOW_COPY_AND_ASSIGN(SyncedBookmarkTracker);
};

}  // namespace sync_bookmarks

#endif  // COMPONENTS_SYNC_BOOKMARKS_SYNCED_BOOKMARK_TRACKER_H_
