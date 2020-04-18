// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_bookmarks/bookmark_model_type_processor.h"

#include <utility>

#include "base/callback.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync/engine/commit_queue.h"
#include "components/undo/bookmark_undo_utils.h"

namespace sync_bookmarks {

namespace {

// The sync protocol identifies top-level entities by means of well-known tags,
// (aka server defined tags) which should not be confused with titles or client
// tags that aren't supported by bookmarks (at the time of writing). Each tag
// corresponds to a singleton instance of a particular top-level node in a
// user's share; the tags are consistent across users. The tags allow us to
// locate the specific folders whose contents we care about synchronizing,
// without having to do a lookup by name or path.  The tags should not be made
// user-visible. For example, the tag "bookmark_bar" represents the permanent
// node for bookmarks bar in Chrome. The tag "other_bookmarks" represents the
// permanent folder Other Bookmarks in Chrome.
//
// It is the responsibility of something upstream (at time of writing, the sync
// server) to create these tagged nodes when initializing sync for the first
// time for a user.  Thus, once the backend finishes initializing, the
// ProfileSyncService can rely on the presence of tagged nodes.
const char kBookmarkBarTag[] = "bookmark_bar";
const char kMobileBookmarksTag[] = "synced_bookmarks";
const char kOtherBookmarksTag[] = "other_bookmarks";

// Id is created by concatenating the specifics field number and the server tag
// similar to LookbackServerEntity::CreateId() that uses
// GetSpecificsFieldNumberFromModelType() to compute the field number.
static const char kBookmarksRootId[] = "32904_google_chrome_bookmarks";

// |sync_entity| must contain a bookmark specifics.
// Metainfo entries must have unique keys.
bookmarks::BookmarkNode::MetaInfoMap GetBookmarkMetaInfo(
    const syncer::EntityData& sync_entity) {
  const sync_pb::BookmarkSpecifics& specifics =
      sync_entity.specifics.bookmark();
  bookmarks::BookmarkNode::MetaInfoMap meta_info_map;
  for (const sync_pb::MetaInfo& meta_info : specifics.meta_info()) {
    meta_info_map[meta_info.key()] = meta_info.value();
  }
  DCHECK_EQ(static_cast<size_t>(specifics.meta_info_size()),
            meta_info_map.size());
  return meta_info_map;
}

// Creates a bookmark node under the given parent node from the given sync node.
// Returns the newly created node. |sync_entity| must contain a bookmark
// specifics with Metainfo entries having unique keys.
const bookmarks::BookmarkNode* CreateBookmarkNode(
    const syncer::EntityData& sync_entity,
    const bookmarks::BookmarkNode* parent,
    bookmarks::BookmarkModel* model,
    syncer::SyncClient* sync_client,
    int index) {
  DCHECK(parent);
  DCHECK(model);
  DCHECK(sync_client);

  const sync_pb::BookmarkSpecifics& specifics =
      sync_entity.specifics.bookmark();
  bookmarks::BookmarkNode::MetaInfoMap metainfo =
      GetBookmarkMetaInfo(sync_entity);
  if (sync_entity.is_folder) {
    return model->AddFolderWithMetaInfo(
        parent, index, base::UTF8ToUTF16(specifics.title()), &metainfo);
  }
  // 'creation_time_us' was added in M24. Assume a time of 0 means now.
  const int64_t create_time_us = specifics.creation_time_us();
  base::Time create_time =
      (create_time_us == 0)
          ? base::Time::Now()
          : base::Time::FromDeltaSinceWindowsEpoch(
                // Use FromDeltaSinceWindowsEpoch because create_time_us has
                // always used the Windows epoch.
                base::TimeDelta::FromMicroseconds(create_time_us));
  return model->AddURLWithCreationTimeAndMetaInfo(
      parent, index, base::UTF8ToUTF16(specifics.title()),
      GURL(specifics.url()), create_time, &metainfo);
  // TODO(crbug.com/516866): Add the favicon related code.
}

class ScopedRemoteUpdateBookmarks {
 public:
  // |bookmark_model| must not be null and must outlive this object.
  explicit ScopedRemoteUpdateBookmarks(syncer::SyncClient* const sync_client)
      : sync_client_(sync_client),
        suspend_undo_(sync_client->GetBookmarkUndoServiceIfExists()) {
    // Notify UI intensive observers of BookmarkModel that we are about to make
    // potentially significant changes to it, so the updates may be batched. For
    // example, on Mac, the bookmarks bar displays animations when bookmark
    // items are added or deleted.
    sync_client_->GetBookmarkModel()->BeginExtensiveChanges();
  }

  ~ScopedRemoteUpdateBookmarks() {
    // Notify UI intensive observers of BookmarkModel that all updates have been
    // applied, and that they may now be consumed. This prevents issues like the
    // one described in https://crbug.com/281562, where old and new items on the
    // bookmarks bar would overlap.
    sync_client_->GetBookmarkModel()->EndExtensiveChanges();
  }

 private:
  syncer::SyncClient* const sync_client_;

  // Changes made to the bookmark model due to sync should not be undoable.
  ScopedSuspendBookmarkUndo suspend_undo_;

  DISALLOW_COPY_AND_ASSIGN(ScopedRemoteUpdateBookmarks);
};
}  // namespace

BookmarkModelTypeProcessor::BookmarkModelTypeProcessor(
    syncer::SyncClient* sync_client)
    : sync_client_(sync_client),
      bookmark_model_(sync_client->GetBookmarkModel()),
      weak_ptr_factory_(this) {}

BookmarkModelTypeProcessor::~BookmarkModelTypeProcessor() = default;

void BookmarkModelTypeProcessor::ConnectSync(
    std::unique_ptr<syncer::CommitQueue> worker) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!worker_);
  worker_ = std::move(worker);
}

void BookmarkModelTypeProcessor::DisconnectSync() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  NOTIMPLEMENTED();
}

void BookmarkModelTypeProcessor::GetLocalChanges(
    size_t max_entries,
    const GetLocalChangesCallback& callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  syncer::CommitRequestDataList local_changes;
  callback.Run(std::move(local_changes));
  NOTIMPLEMENTED();
}

void BookmarkModelTypeProcessor::OnCommitCompleted(
    const sync_pb::ModelTypeState& type_state,
    const syncer::CommitResponseDataList& response_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  NOTIMPLEMENTED();
}

void BookmarkModelTypeProcessor::OnUpdateReceived(
    const sync_pb::ModelTypeState& model_type_state,
    const syncer::UpdateResponseDataList& updates) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  ScopedRemoteUpdateBookmarks update_bookmarks(sync_client_);

  for (const syncer::UpdateResponseData* update : ReorderUpdates(updates)) {
    const syncer::EntityData& update_data = update->entity.value();
    // TODO(crbug.com/516866): Check |update_data| for sanity.
    // 1. Has bookmark specifics or no specifics in case of delete.
    // 2. All meta info entries in the specifics have unique keys.
    const SyncedBookmarkTracker::Entity* tracked_entity =
        bookmark_tracker_.GetEntityForSyncId(update_data.id);
    if (update_data.is_deleted()) {
      ProcessRemoteDelete(update_data, tracked_entity);
      continue;
    }
    if (!tracked_entity) {
      ProcessRemoteCreate(update_data);
      continue;
    }
    // Ignore changes to the permanent nodes (e.g. bookmarks bar). We only care
    // about their children.
    if (bookmark_model_->is_permanent_node(tracked_entity->bookmark_node())) {
      continue;
    }
    ProcessRemoteUpdate(update_data, tracked_entity);
  }
}

// static
std::vector<const syncer::UpdateResponseData*>
BookmarkModelTypeProcessor::ReorderUpdatesForTest(
    const syncer::UpdateResponseDataList& updates) {
  return ReorderUpdates(updates);
}

const SyncedBookmarkTracker* BookmarkModelTypeProcessor::GetTrackerForTest()
    const {
  return &bookmark_tracker_;
}

// static
std::vector<const syncer::UpdateResponseData*>
BookmarkModelTypeProcessor::ReorderUpdates(
    const syncer::UpdateResponseDataList& updates) {
  // TODO(crbug.com/516866): This is a very simple (hacky) reordering algorithm
  // that assumes no folders exist except the top level permanent ones. This
  // should be fixed before enabling USS for bookmarks.
  std::vector<const syncer::UpdateResponseData*> ordered_updates;
  for (const syncer::UpdateResponseData& update : updates) {
    const syncer::EntityData& update_data = update.entity.value();
    if (update_data.parent_id == "0") {
      continue;
    }
    if (update_data.parent_id == kBookmarksRootId) {
      ordered_updates.push_back(&update);
    }
  }
  for (const syncer::UpdateResponseData& update : updates) {
    const syncer::EntityData& update_data = update.entity.value();
    // Deletions should come last.
    if (update_data.is_deleted()) {
      continue;
    }
    if (update_data.parent_id != "0" &&
        update_data.parent_id != kBookmarksRootId) {
      ordered_updates.push_back(&update);
    }
  }
  // Now add deletions.
  for (const syncer::UpdateResponseData& update : updates) {
    const syncer::EntityData& update_data = update.entity.value();
    if (!update_data.is_deleted()) {
      continue;
    }
    if (update_data.parent_id != "0" &&
        update_data.parent_id != kBookmarksRootId) {
      ordered_updates.push_back(&update);
    }
  }
  return ordered_updates;
}

void BookmarkModelTypeProcessor::ProcessRemoteCreate(
    const syncer::EntityData& update_data) {
  // Because the Synced Bookmarks node can be created server side, it's possible
  // it'll arrive at the client as an update. In that case it won't have been
  // associated at startup, the GetChromeNodeFromSyncId call above will return
  // null, and we won't detect it as a permanent node, resulting in us trying to
  // create it here (which will fail). Therefore, we add special logic here just
  // to detect the Synced Bookmarks folder.
  if (update_data.parent_id == kBookmarksRootId) {
    // Associate permanent folders.
    AssociatePermanentFolder(update_data);
    return;
  }

  const bookmarks::BookmarkNode* parent_node = GetParentNode(update_data);
  if (!parent_node) {
    // If we cannot find the parent, we can do nothing.
    DLOG(ERROR) << "Could not find parent of node being added."
                << " Node title: " << update_data.specifics.bookmark().title()
                << ", parent id = " << update_data.parent_id;
    return;
  }
  // TODO(crbug.com/516866): This code appends the code to the very end of the
  // list of the children by assigning the index to the
  // parent_node->child_count(). It should instead compute the exact using the
  // unique position information of the new node as well as the siblings.
  const bookmarks::BookmarkNode* bookmark_node =
      CreateBookmarkNode(update_data, parent_node, bookmark_model_,
                         sync_client_, parent_node->child_count());
  if (!bookmark_node) {
    // We ignore bookmarks we can't add.
    DLOG(ERROR) << "Failed to create bookmark node with title "
                << update_data.specifics.bookmark().title() << " and url "
                << update_data.specifics.bookmark().url();
    return;
  }
  bookmark_tracker_.Associate(update_data.id, bookmark_node);
  // TODO(crbug.com/516866): Update metadata (e.g. server version,
  // specifics_hash).
}

void BookmarkModelTypeProcessor::ProcessRemoteUpdate(
    const syncer::EntityData& update_data,
    const SyncedBookmarkTracker::Entity* tracked_entity) {
  // Can only update existing nodes.
  DCHECK(tracked_entity);
  DCHECK_EQ(tracked_entity,
            bookmark_tracker_.GetEntityForSyncId(update_data.id));
  // Must no be a deletion
  DCHECK(!update_data.is_deleted());
  if (tracked_entity->IsUnsynced()) {
    // TODO(crbug.com/516866): Handle conflict resolution.
    return;
  }
  if (tracked_entity->MatchesData(update_data)) {
    // TODO(crbug.com/516866): Update metadata (e.g. server version,
    // specifics_hash).
    return;
  }
  const bookmarks::BookmarkNode* node = tracked_entity->bookmark_node();
  if (update_data.is_folder != node->is_folder()) {
    DLOG(ERROR) << "Could not update node. Remote node is a "
                << (update_data.is_folder ? "folder" : "bookmark")
                << " while local node is a "
                << (node->is_folder() ? "folder" : "bookmark");
    return;
  }
  const sync_pb::BookmarkSpecifics& specifics =
      update_data.specifics.bookmark();
  if (!update_data.is_folder) {
    bookmark_model_->SetURL(node, GURL(specifics.url()));
  }

  bookmark_model_->SetTitle(node, base::UTF8ToUTF16(specifics.title()));
  // TODO(crbug.com/516866): Add the favicon related code.
  bookmark_model_->SetNodeMetaInfoMap(node, GetBookmarkMetaInfo(update_data));
  // TODO(crbug.com/516866): Update metadata (e.g. server version,
  // specifics_hash).
  // TODO(crbug.com/516866): Handle reparenting.
  // TODO(crbug.com/516866): Handle the case of moving the bookmark to a new
  // position under the same parent (i.e. change in the unique position)
}

void BookmarkModelTypeProcessor::ProcessRemoteDelete(
    const syncer::EntityData& update_data,
    const SyncedBookmarkTracker::Entity* tracked_entity) {
  DCHECK(update_data.is_deleted());

  DCHECK_EQ(tracked_entity,
            bookmark_tracker_.GetEntityForSyncId(update_data.id));

  // Handle corner cases first.
  if (tracked_entity == nullptr) {
    // Local entity doesn't exist and update is tombstone.
    DLOG(WARNING) << "Received remote delete for a non-existing item.";
    return;
  }

  const bookmarks::BookmarkNode* node = tracked_entity->bookmark_node();
  // Ignore changes to the permanent top-level nodes.  We only care about
  // their children.
  if (bookmark_model_->is_permanent_node(node)) {
    return;
  }
  // TODO(crbug.com/516866): Allow deletions of non-empty direcoties if makes
  // sense, and recursively delete children.
  if (node->child_count() > 0) {
    DLOG(WARNING) << "Trying to delete a non-empty folder.";
    return;
  }

  bookmark_model_->Remove(node);
  bookmark_tracker_.Disassociate(update_data.id);
}

const bookmarks::BookmarkNode* BookmarkModelTypeProcessor::GetParentNode(
    const syncer::EntityData& update_data) const {
  const SyncedBookmarkTracker::Entity* parent_entity =
      bookmark_tracker_.GetEntityForSyncId(update_data.parent_id);
  if (!parent_entity) {
    return nullptr;
  }
  return parent_entity->bookmark_node();
}

void BookmarkModelTypeProcessor::AssociatePermanentFolder(
    const syncer::EntityData& update_data) {
  DCHECK_EQ(update_data.parent_id, kBookmarksRootId);

  const bookmarks::BookmarkNode* permanent_node = nullptr;
  if (update_data.server_defined_unique_tag == kBookmarkBarTag) {
    permanent_node = bookmark_model_->bookmark_bar_node();
  } else if (update_data.server_defined_unique_tag == kOtherBookmarksTag) {
    permanent_node = bookmark_model_->other_node();
  } else if (update_data.server_defined_unique_tag == kMobileBookmarksTag) {
    permanent_node = bookmark_model_->mobile_node();
  }

  if (permanent_node != nullptr) {
    bookmark_tracker_.Associate(update_data.id, permanent_node);
  }
}

base::WeakPtr<syncer::ModelTypeProcessor>
BookmarkModelTypeProcessor::GetWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace sync_bookmarks
