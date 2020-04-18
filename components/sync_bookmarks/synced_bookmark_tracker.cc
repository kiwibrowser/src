// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_bookmarks/synced_bookmark_tracker.h"

#include <utility>

#include "components/sync/model/entity_data.h"

namespace sync_bookmarks {

SyncedBookmarkTracker::Entity::Entity(
    const bookmarks::BookmarkNode* bookmark_node)
    : bookmark_node_(bookmark_node) {
  DCHECK(bookmark_node);
}

SyncedBookmarkTracker::Entity::~Entity() = default;

bool SyncedBookmarkTracker::Entity::MatchesData(
    const syncer::EntityData& data) const {
  // TODO(crbug.com/516866): Implement properly.
  return false;
}

bool SyncedBookmarkTracker::Entity::IsUnsynced() const {
  // TODO(crbug.com/516866): Implement properly.
  return false;
}

SyncedBookmarkTracker::SyncedBookmarkTracker() = default;
SyncedBookmarkTracker::~SyncedBookmarkTracker() = default;

const SyncedBookmarkTracker::Entity* SyncedBookmarkTracker::GetEntityForSyncId(
    const std::string& sync_id) const {
  auto it = sync_id_to_entities_map_.find(sync_id);
  return it != sync_id_to_entities_map_.end() ? it->second.get() : nullptr;
}

void SyncedBookmarkTracker::Associate(
    const std::string& sync_id,
    const bookmarks::BookmarkNode* bookmark_node) {
  sync_id_to_entities_map_[sync_id] = std::make_unique<Entity>(bookmark_node);
}

void SyncedBookmarkTracker::Disassociate(const std::string& sync_id) {
  sync_id_to_entities_map_.erase(sync_id);
}

std::size_t SyncedBookmarkTracker::TrackedEntitiesCountForTest() const {
  return sync_id_to_entities_map_.size();
}

}  // namespace sync_bookmarks
