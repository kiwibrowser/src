// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_READ_NODE_H_
#define COMPONENTS_SYNC_SYNCABLE_READ_NODE_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/syncable/base_node.h"

namespace syncer {

// ReadNode wraps a syncable::Entry to provide the functionality of a
// read-only BaseNode.
class ReadNode : public BaseNode {
 public:
  // Create an unpopulated ReadNode on the given transaction.  Call some flavor
  // of Init to populate the ReadNode with a database entry.
  explicit ReadNode(const BaseTransaction* transaction);
  ~ReadNode() override;

  // A client must use one (and only one) of the following Init variants to
  // populate the node.

  // BaseNode implementation.
  InitByLookupResult InitByIdLookup(int64_t id) override;
  InitByLookupResult InitByClientTagLookup(ModelType model_type,
                                           const std::string& tag) override;

  // There is always a root node, so this can't fail.  The root node is
  // never mutable, so root lookup is only possible on a ReadNode.
  void InitByRootLookup();

  // Returns the type root node, if it exists.  This is usually created by the
  // server during first sync.  Eventually, we plan to remove support for it
  // from the protocol and have the client create the node instead.
  InitByLookupResult InitTypeRoot(ModelType type);

  // Returns a server-created and unique-server-tagged item.
  //
  // This functionality is only useful for bookmarks because only bookmarks
  // have server-tagged items.  All other server-tagged items are type root
  // nodes, which should be looked up with InitTypeRoot().
  InitByLookupResult InitByTagLookupForBookmarks(const std::string& tag);

  // Returns transaction version of the last transaction where this node has
  // been modified.
  int64_t GetTransactionVersion() const;

  // Implementation of BaseNode's abstract virtual accessors.
  const syncable::Entry* GetEntry() const override;
  const BaseTransaction* GetTransaction() const override;

 protected:
  ReadNode();

 private:
  void* operator new(size_t size);  // Node is meant for stack use only.

  // The underlying syncable object which this class wraps.
  syncable::Entry* entry_;

  // The sync API transaction that is the parent of this node.
  const BaseTransaction* transaction_;

  DISALLOW_COPY_AND_ASSIGN(ReadNode);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_READ_NODE_H_
