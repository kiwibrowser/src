// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_CHANGE_PROCESSOR_H_
#define COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_CHANGE_PROCESSOR_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/sync/model/change_processor.h"
#include "components/sync/model/data_type_error_handler.h"
#include "components/sync_bookmarks/bookmark_model_associator.h"

namespace base {
class RefCountedMemory;
}  // namespace base

namespace syncer {
class SyncClient;
class WriteNode;
class WriteTransaction;
}  // namespace syncer

namespace sync_bookmarks {

// This class is responsible for taking changes from the BookmarkModel
// and applying them to the sync API 'syncable' model, and vice versa.
// All operations and use of this class are from the UI thread.
// This is currently bookmarks specific.
class BookmarkChangeProcessor : public bookmarks::BookmarkModelObserver,
                                public syncer::ChangeProcessor {
 public:
  BookmarkChangeProcessor(
      syncer::SyncClient* sync_client,
      BookmarkModelAssociator* model_associator,
      std::unique_ptr<syncer::DataTypeErrorHandler> error_handler);
  ~BookmarkChangeProcessor() override;

  // bookmarks::BookmarkModelObserver:
  // BookmarkModel -> sync API model change application.
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void OnWillRemoveBookmarks(bookmarks::BookmarkModel* model,
                             const bookmarks::BookmarkNode* parent,
                             int old_index,
                             const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* parent,
                           int old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& no_longer_bookmarked) override;
  void BookmarkAllUserNodesRemoved(bookmarks::BookmarkModel* model,
                                   const std::set<GURL>& removed_urls) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkMetaInfoChanged(bookmarks::BookmarkModel* model,
                               const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;

  // The change processor implementation, responsible for applying changes from
  // the sync model to the bookmarks model.
  void ApplyChangesFromSyncModel(
      const syncer::BaseTransaction* trans,
      int64_t model_version,
      const syncer::ImmutableChangeRecordList& changes) override;

  // The following methods are static and hence may be invoked at any time, and
  // do not depend on having a running ChangeProcessor.
  // TODO(stanisc): considier refactoring these methods out of this class.

  // Updates the title, URL, creation time and favicon of the bookmark |node|
  // with data taken from the |sync_node| sync node.
  static void UpdateBookmarkWithSyncData(const syncer::BaseNode& sync_node,
                                         bookmarks::BookmarkModel* model,
                                         const bookmarks::BookmarkNode* node,
                                         syncer::SyncClient* sync_client);

  // Creates a bookmark node under the given parent node from the given sync
  // node. Returns the newly created node.  The created node is placed at the
  // specified index among the parent's children.
  static const bookmarks::BookmarkNode* CreateBookmarkNode(
      const syncer::BaseNode* sync_node,
      const bookmarks::BookmarkNode* parent,
      bookmarks::BookmarkModel* model,
      syncer::SyncClient* sync_client,
      int index);

  // Overload of CreateBookmarkNode function above that helps to avoid
  // converting / parsing the bookmark title and URL multiple times.
  static const bookmarks::BookmarkNode* CreateBookmarkNode(
      const base::string16& title,
      const GURL& url,
      const syncer::BaseNode* sync_node,
      const bookmarks::BookmarkNode* parent,
      bookmarks::BookmarkModel* model,
      syncer::SyncClient* sync_client,
      int index);

  // Sets the favicon of the given bookmark node from the given sync node.
  // |profile| is the profile that contains the HistoryService and BookmarkModel
  // for the bookmark in question.
  static void SetBookmarkFavicon(const syncer::BaseNode* sync_node,
                                 const bookmarks::BookmarkNode* bookmark_node,
                                 bookmarks::BookmarkModel* model,
                                 syncer::SyncClient* sync_client);

  // Applies the 1x favicon |bitmap_data| and |icon_url| to |bookmark_node|.
  // |profile| is the profile that contains the HistoryService and BookmarkModel
  // for the bookmark in question.
  static void ApplyBookmarkFavicon(
      const bookmarks::BookmarkNode* bookmark_node,
      syncer::SyncClient* sync_client,
      const GURL& icon_url,
      const scoped_refptr<base::RefCountedMemory>& bitmap_data);

  // Sets the favicon of the given sync node from the given bookmark node.
  static void SetSyncNodeFavicon(const bookmarks::BookmarkNode* bookmark_node,
                                 bookmarks::BookmarkModel* model,
                                 syncer::WriteNode* sync_node);

  // Treat the |index|th child of |parent| as a newly added node, and create a
  // corresponding node in the sync domain using |trans|.  All properties
  // will be transferred to the new node.  A node corresponding to |parent|
  // must already exist and be associated for this call to succeed.  Returns
  // the ID of the just-created node, or if creation fails, kInvalidID.
  static int64_t CreateSyncNode(const bookmarks::BookmarkNode* parent,
                                bookmarks::BookmarkModel* model,
                                int index,
                                syncer::WriteTransaction* trans,
                                BookmarkModelAssociator* associator,
                                syncer::DataTypeErrorHandler* error_handler);

  // Update |bookmark_node|'s sync node.
  static int64_t UpdateSyncNode(const bookmarks::BookmarkNode* bookmark_node,
                                bookmarks::BookmarkModel* model,
                                syncer::WriteTransaction* trans,
                                BookmarkModelAssociator* associator,
                                syncer::DataTypeErrorHandler* error_handler);

  // Tombstone |topmost_sync_node| node and all its children in the sync domain
  // using transaction |trans|. Returns the number of removed nodes.
  static int RemoveSyncNodeHierarchy(syncer::WriteTransaction* trans,
                                     syncer::WriteNode* topmost_sync_node,
                                     BookmarkModelAssociator* associator);

  // Update transaction version of |model| and |nodes| to |new_version| if
  // it's valid.
  static void UpdateTransactionVersion(
      int64_t new_version,
      bookmarks::BookmarkModel* model,
      const std::vector<const bookmarks::BookmarkNode*>& nodes);

 protected:
  void StartImpl() override;

 private:
  enum MoveOrCreate {
    MOVE,
    CREATE,
  };

  // Retrieves the meta info from the given sync node.
  static std::unique_ptr<bookmarks::BookmarkNode::MetaInfoMap>
  GetBookmarkMetaInfo(const syncer::BaseNode* sync_node);

  // Sets the meta info of the given sync node from the given bookmark node.
  static void SetSyncNodeMetaInfo(const bookmarks::BookmarkNode* node,
                                  syncer::WriteNode* sync_node);

  // Helper function used to fix the position of a sync node so that it matches
  // the position of a corresponding bookmark model node. |parent| and
  // |index| identify the bookmark model position.  |dst| is the node whose
  // position is to be fixed.  If |operation| is CREATE, treat |dst| as an
  // uncreated node and set its position via InitByCreation(); otherwise,
  // |dst| is treated as an existing node, and its position will be set via
  // SetPosition().  |trans| is the transaction to which |dst| belongs. Returns
  // false on failure.
  static bool PlaceSyncNode(MoveOrCreate operation,
                            const bookmarks::BookmarkNode* parent,
                            int index,
                            syncer::WriteTransaction* trans,
                            syncer::WriteNode* dst,
                            BookmarkModelAssociator* associator);

  // Copy properties (but not position) from |src| to |dst|.
  static void UpdateSyncNodeProperties(
      const bookmarks::BookmarkNode* src,
      bookmarks::BookmarkModel* model,
      syncer::WriteNode* dst,
      syncer::DataTypeErrorHandler* error_handler);

  // Helper function to encode a bookmark's favicon into raw PNG data.
  static void EncodeFavicon(const bookmarks::BookmarkNode* src,
                            bookmarks::BookmarkModel* model,
                            scoped_refptr<base::RefCountedMemory>* dst);

  // Remove all sync nodes, except the permanent nodes.
  void RemoveAllSyncNodes();

  // Remove all the sync nodes associated with |node| and its children.
  void RemoveSyncNodeHierarchy(const bookmarks::BookmarkNode* node);

  // Remove all children of |sync_node|. Returns the number of removed
  // children.
  static int RemoveAllChildNodes(syncer::WriteTransaction* trans,
                                 int64_t sync_id,
                                 BookmarkModelAssociator* associator);

  // Remove |sync_node|. It should not have any children.
  static void RemoveOneSyncNode(syncer::WriteNode* sync_node,
                                BookmarkModelAssociator* associator);

  // Creates or updates a sync node associated with |node|.
  void CreateOrUpdateSyncNode(const bookmarks::BookmarkNode* node);

  // Returns false if |node| should not be synced.
  bool CanSyncNode(const bookmarks::BookmarkNode* node);

  base::ThreadChecker thread_checker_;

  // The bookmark model we are processing changes from.  Non-null when
  // |running_| is true.
  bookmarks::BookmarkModel* bookmark_model_;

  syncer::SyncClient* sync_client_;

  // The two models should be associated according to this ModelAssociator.
  BookmarkModelAssociator* model_associator_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkChangeProcessor);
};

}  // namespace sync_bookmarks

#endif  // COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_CHANGE_PROCESSOR_H_
