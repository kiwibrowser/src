// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_MODEL_ASSOCIATOR_H_
#define COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_MODEL_ASSOCIATOR_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/containers/stack.h"
#include "base/hash.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/threading/thread_checker.h"
#include "components/sync/base/unrecoverable_error_handler.h"
#include "components/sync/driver/model_associator.h"
#include "components/sync/model/data_type_error_handler.h"

class GURL;

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}

namespace syncer {
class BaseNode;
class BaseTransaction;
class SyncClient;
class WriteTransaction;
struct UserShare;
}

namespace sync_bookmarks {

// Contains all model association related logic:
// * Algorithm to associate bookmark model and sync model.
// * Methods to get a bookmark node for a given sync node and vice versa.
// * Persisting model associations and loading them back.
class BookmarkModelAssociator : public syncer::AssociatorInterface {
 public:
  static syncer::ModelType model_type() { return syncer::BOOKMARKS; }
  // |expect_mobile_bookmarks_folder| controls whether or not we
  // expect the mobile bookmarks permanent folder to be created.
  // Should be set to true only by mobile clients.
  BookmarkModelAssociator(
      bookmarks::BookmarkModel* bookmark_model,
      syncer::SyncClient* sync_client,
      syncer::UserShare* user_share,
      std::unique_ptr<syncer::DataTypeErrorHandler> unrecoverable_error_handler,
      bool expect_mobile_bookmarks_folder);
  ~BookmarkModelAssociator() override;

  // AssociatorInterface implementation.
  //
  // AssociateModels iterates through both the sync and the browser
  // bookmark model, looking for matched pairs of items.  For any pairs it
  // finds, it will call AssociateSyncID.  For any unmatched items,
  // MergeAndAssociateModels will try to repair the match, e.g. by adding a new
  // node.  After successful completion, the models should be identical and
  // corresponding. Returns true on success.  On failure of this step, we
  // should abort the sync operation and report an error to the user.
  syncer::SyncError AssociateModels(
      syncer::SyncMergeResult* local_merge_result,
      syncer::SyncMergeResult* syncer_merge_result) override;

  syncer::SyncError DisassociateModels() override;

  // The has_nodes out param is true if the sync model has nodes other
  // than the permanent tagged nodes.
  bool SyncModelHasUserCreatedNodes(bool* has_nodes) override;

  // Returns sync id for the given bookmark node id.
  // Returns syncer::kInvalidId if the sync node is not found for the given
  // bookmark node id.
  int64_t GetSyncIdFromChromeId(const int64_t& node_id);

  // Returns the bookmark node for the given sync id.
  // Returns null if no bookmark node is found for the given sync id.
  const bookmarks::BookmarkNode* GetChromeNodeFromSyncId(int64_t sync_id);

  // Initializes the given sync node from the given bookmark node id.
  // Returns false if no sync node was found for the given bookmark node id or
  // if the initialization of sync node fails.
  bool InitSyncNodeFromChromeId(const int64_t& node_id,
                                syncer::BaseNode* sync_node);

  // Associates the given bookmark node with the given sync node.
  void Associate(const bookmarks::BookmarkNode* node,
                 const syncer::BaseNode& sync_node);
  // Remove the association that corresponds to the given sync id.
  void Disassociate(int64_t sync_id);

  void AbortAssociation() override {
    // No implementation needed, this associator runs on the main
    // thread.
  }

  // See ModelAssociator interface.
  bool CryptoReadyIfNecessary() override;

 private:
  using BookmarkIdToSyncIdMap = std::map<int64_t, int64_t>;
  using SyncIdToBookmarkNodeMap =
      std::map<int64_t, const bookmarks::BookmarkNode*>;
  using DirtyAssociationsSyncIds = std::set<int64_t>;
  using BookmarkList = std::vector<const bookmarks::BookmarkNode*>;
  using BookmarkStack = base::stack<const bookmarks::BookmarkNode*>;

  // Add association between native node and sync node to the maps.
  void AddAssociation(const bookmarks::BookmarkNode* node, int64_t sync_id);

  // Posts a task to persist dirty associations.
  void PostPersistAssociationsTask();
  // Persists all dirty associations.
  void PersistAssociations();

  // Result of the native model version check against the sync
  // version performed by CheckModelSyncState.
  enum NativeModelSyncState {
    // The native version is syncer::syncable::kInvalidTransactionVersion,
    // which is the case when the version has either not been set yet or reset
    // as a result of a previous error during the association. Basically the
    // state should return back to UNSET on an association following the one
    // where the state was different than IN_SYNC.
    UNSET,
    // The native version was in sync with the Sync version.
    IN_SYNC,
    // The native version was behing the sync version which indicates a failure
    // to persist the native bookmarks model.
    BEHIND,
    // The native version was ahead of the sync version which indicates a
    // a failure to persist Sync DB.
    AHEAD,
    NATIVE_MODEL_SYNC_STATE_COUNT
  };

  // Helper class used within AssociateModels to simplify the logic and
  // minimize the number of arguments passed between private functions.
  class Context {
   public:
    Context(syncer::SyncMergeResult* local_merge_result,
            syncer::SyncMergeResult* syncer_merge_result);
    ~Context();

    // Push a sync node to the DFS stack.
    void PushNode(int64_t sync_id);
    // Pops a sync node from the DFS stack. Returns false if the stack
    // is empty.
    bool PopNode(int64_t* sync_id);

    // The following methods are used to update |local_merge_result_| and
    // |syncer_merge_result_|.
    void SetPreAssociationVersions(int64_t native_version,
                                   int64_t sync_version);
    void SetNumItemsBeforeAssociation(int local_num, int sync_num);
    void SetNumItemsAfterAssociation(int local_num, int sync_num);
    void IncrementLocalItemsDeleted();
    void IncrementLocalItemsAdded();
    void IncrementLocalItemsModified();
    void IncrementSyncItemsAdded();
    void IncrementSyncItemsDeleted(int count);

    void UpdateDuplicateCount(const base::string16& title, const GURL& url);

    int duplicate_count() const { return duplicate_count_; }

    NativeModelSyncState native_model_sync_state() const {
      return native_model_sync_state_;
    }
    void set_native_model_sync_state(NativeModelSyncState state) {
      native_model_sync_state_ = state;
    }

    // Bookmark roots participating in the sync.
    void AddBookmarkRoot(const bookmarks::BookmarkNode* root);
    const BookmarkList& bookmark_roots() const { return bookmark_roots_; }

    // Gets pre-association sync version for Bookmarks datatype.
    int64_t GetSyncPreAssociationVersion() const;

    void MarkForVersionUpdate(const bookmarks::BookmarkNode* node);
    const BookmarkList& bookmarks_for_version_update() const {
      return bookmarks_for_version_update_;
    }

   private:
    // DFS stack of sync nodes traversed during association.
    base::stack<int64_t> dfs_stack_;
    // Local and merge results are not owned.
    syncer::SyncMergeResult* local_merge_result_;
    syncer::SyncMergeResult* syncer_merge_result_;
    // |hashes_| contains hash codes of all native bookmarks
    // for the purpose of detecting duplicates. A small number of
    // false positives due to hash collisions is OK because this
    // data is used for reporting purposes only.
    base::hash_set<size_t> hashes_;
    // Overall number of bookmark collisions from RecordDuplicates call.
    int duplicate_count_;
    // Result of the most recent BookmarkModelAssociator::CheckModelSyncState.
    NativeModelSyncState native_model_sync_state_;
    // List of bookmark model roots participating in the sync.
    BookmarkList bookmark_roots_;
    // List of bookmark nodes for which the transaction version needs to be
    // updated.
    BookmarkList bookmarks_for_version_update_;

    DISALLOW_COPY_AND_ASSIGN(Context);
  };

  // Matches up the bookmark model and the sync model to build model
  // associations.
  syncer::SyncError BuildAssociations(Context* context);

  // Two helper functions that populate SyncMergeResult with numbers of
  // items before/after the association.
  void SetNumItemsBeforeAssociation(syncer::BaseTransaction* trans,
                                    Context* context);
  void SetNumItemsAfterAssociation(syncer::BaseTransaction* trans,
                                   Context* context);

  // Used by SetNumItemsBeforeAssociation.
  // Similar to BookmarkNode::GetTotalNodeCount but also scans the native
  // model for duplicates and records them in |context|.
  int GetTotalBookmarkCountAndRecordDuplicates(
      const bookmarks::BookmarkNode* node,
      Context* context) const;

  // Helper function that associates all tagged permanent folders and primes
  // the provided context with sync IDs of those folders.
  syncer::SyncError AssociatePermanentFolders(syncer::BaseTransaction* trans,
                                              Context* context);

  // Associate a top-level node of the bookmark model with a permanent node in
  // the sync domain.  Such permanent nodes are identified by a tag that is
  // well known to the server and the client, and is unique within a particular
  // user's share.  For example, "other_bookmarks" is the tag for the Other
  // Bookmarks folder.  The sync nodes are server-created.
  // Returns true on success, false if association failed.
  bool AssociateTaggedPermanentNode(
      syncer::BaseTransaction* trans,
      const bookmarks::BookmarkNode* permanent_node,
      const std::string& tag) WARN_UNUSED_RESULT;

  // Removes bookmark nodes whose corresponding sync nodes have been deleted
  // according to sync delete journals.
  void ApplyDeletesFromSyncJournal(syncer::BaseTransaction* trans,
                                   Context* context);

  // The main part of the association process that associatiates
  // native nodes that are children of |parent_node| with sync nodes with IDs
  // from |sync_ids|.
  syncer::SyncError BuildAssociations(
      syncer::WriteTransaction* trans,
      const bookmarks::BookmarkNode* parent_node,
      const std::vector<int64_t>& sync_ids,
      Context* context);

  // Helper method for creating a new native bookmark node.
  const bookmarks::BookmarkNode* CreateBookmarkNode(
      const bookmarks::BookmarkNode* parent_node,
      int bookmark_index,
      const syncer::BaseNode* sync_child_node,
      const GURL& url,
      Context* context,
      syncer::SyncError* error);

  // Check whether bookmark model and sync model are synced by comparing
  // their transaction versions.
  // Returns a PERSISTENCE_ERROR if a transaction mismatch was detected where
  // the native model has a newer transaction verison.
  syncer::SyncError CheckModelSyncState(Context* context) const;

  base::ThreadChecker thread_checker_;
  bookmarks::BookmarkModel* bookmark_model_;
  syncer::SyncClient* sync_client_;
  syncer::UserShare* user_share_;
  std::unique_ptr<syncer::DataTypeErrorHandler> unrecoverable_error_handler_;
  const bool expect_mobile_bookmarks_folder_;
  BookmarkIdToSyncIdMap id_map_;
  SyncIdToBookmarkNodeMap id_map_inverse_;
  // Stores sync ids for dirty associations.
  DirtyAssociationsSyncIds dirty_associations_sync_ids_;

  // Used to post PersistAssociation tasks to the current message loop and
  // guarantees no invocations can occur if |this| has been deleted. (This
  // allows this class to be non-refcounted).
  base::WeakPtrFactory<BookmarkModelAssociator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkModelAssociator);
};

}  // namespace sync_bookmarks

#endif  // COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_MODEL_ASSOCIATOR_H_
