// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_PARENT_CHILD_INDEX_H_
#define COMPONENTS_SYNC_SYNCABLE_PARENT_CHILD_INDEX_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/syncable/syncable_id.h"

namespace syncer {
namespace syncable {

struct EntryKernel;
class ParentChildIndex;

// A node ordering function.
struct ChildComparator {
  bool operator()(const EntryKernel* a, const EntryKernel* b) const;
};

// An ordered set of nodes.
using OrderedChildSet = std::set<EntryKernel*, ChildComparator>;
using OrderedChildSetRef = std::shared_ptr<OrderedChildSet>;

// Container that tracks parent-child relationships.
// Provides fast lookup of all items under a given parent.
class ParentChildIndex {
 public:
  ParentChildIndex();
  ~ParentChildIndex();

  // Returns whether or not this entry belongs in the index.
  // True for all non-deleted, non-root entries.
  static bool ShouldInclude(const EntryKernel* e);

  // Inserts a given child into the index.
  bool Insert(EntryKernel* e);

  // Removes a given child from the index.
  void Remove(EntryKernel* e);

  // Returns true if this item is in the index as a child.
  bool Contains(EntryKernel* e) const;

  // Returns all children of the entry with the given Id.  Returns null if the
  // node has no children or the Id does not identify a valid directory node.
  const OrderedChildSet* GetChildren(const Id& id) const;

  // Returns all children of the entry.  Returns null if the node has no
  // children.
  const OrderedChildSet* GetChildren(EntryKernel* e) const;

  // Returns all siblings of the entry.
  const OrderedChildSet* GetSiblings(EntryKernel* e) const;

  // Estimates amount of heap memory used.
  size_t EstimateMemoryUsage() const;

 private:
  friend class ParentChildIndexTest;

  using ParentChildrenMap = std::map<Id, OrderedChildSetRef>;
  using TypeRootIds = std::vector<Id>;
  using TypeRootChildSets = std::vector<OrderedChildSetRef>;

  static bool ShouldUseParentId(const Id& parent_id, ModelType model_type);

  // Returns OrderedChildSet that should contain the specified entry
  // based on the entry's Parent ID or model type.
  const OrderedChildSetRef GetChildSet(EntryKernel* e) const;

  // Returns OrderedChildSet that contain entries of the |model_type| type.
  const OrderedChildSetRef GetModelTypeChildSet(ModelType model_type) const;

  // Returns mutable OrderedChildSet that contain entries of the |model_type|
  // type. Create one as necessary.
  OrderedChildSetRef GetOrCreateModelTypeChildSet(ModelType model_type);

  // Returns previously cached model type root ID for the given |model_type|.
  const Id& GetModelTypeRootId(ModelType model_type) const;

  // A map of parent IDs to children.
  // Parents with no children are not included in this map.
  ParentChildrenMap parent_children_map_;

  // This array tracks model type roots IDs.
  TypeRootIds model_type_root_ids_;

  // This array contains pre-defined child sets for
  // non-hierarchical types (types with flat hierarchy) that support entries
  // with implicit parent.
  TypeRootChildSets type_root_child_sets_;

  DISALLOW_COPY_AND_ASSIGN(ParentChildIndex);
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_PARENT_CHILD_INDEX_H_
