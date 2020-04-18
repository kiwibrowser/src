/*
 * Copyright (C) 2017, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_RELATION_CACHE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_RELATION_CACHE_H_

#include "base/macros.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"

namespace blink {

// This class should only be used from inside the accessibility directory.
class AXRelationCache {
 public:
  explicit AXRelationCache(AXObjectCacheImpl*);
  virtual ~AXRelationCache();

  // Returns true if the given object's position in the tree was due to
  // aria-owns.
  bool IsAriaOwned(const AXObject*) const;

  // Returns the parent of the given object due to aria-owns.
  AXObject* GetAriaOwnedParent(const AXObject*) const;

  // Given an object that has an aria-owns attributes, and a vector of ids from
  // the value of that attribute, updates the internal state to reflect the new
  // set of children owned by this object, returning the result in
  // |ownedChildren|. The result is validated - illegal, duplicate, or cyclical
  // references have been removed.
  void UpdateAriaOwns(const AXObject* owner,
                      const Vector<String>& id_vector,
                      HeapVector<Member<AXObject>>& owned_children);

  // Given an element in the DOM tree that was either just added or whose id
  // just changed, check to see if another object wants to be its parent due to
  // aria-owns. If so, update the tree by calling childrenChanged() on the
  // potential owner, possibly reparenting this element.
  void UpdateRelatedTree(Node*);

  // Remove given AXID from cache.
  void RemoveAXID(AXID);

  // Update map of ids to related objects.
  // If one or more ids aren't found, they're added to a lookup table so that if
  // an element with that id appears later, it can be added when you call
  // UpdateRelatedTree.
  void UpdateReverseRelations(const AXObject* relation_source,
                              const Vector<String>& target_ids);

 private:
  // If any object is related to this object via <label for>, aria-owns,
  // aria-describedby or aria-labeledby, update the text for the related object.
  void UpdateRelatedText(Node*);

  bool IsValidOwnsRelation(AXObject* owner, AXObject* child) const;
  void UnmapOwnedChildren(const AXObject* owner, Vector<AXID>);
  void MapOwnedChildren(const AXObject* owner, Vector<AXID>);
  void GetReverseRelated(Node*, HeapVector<Member<AXObject>>& sources);

  WeakPersistent<AXObjectCacheImpl> object_cache_;

  // Map from the AXID of the owner to the AXIDs of the children.
  // This is a validated map, it doesn't contain illegal, duplicate,
  // or cyclical matches, or references to IDs that don't exist.
  HashMap<AXID, Vector<AXID>> aria_owner_to_children_mapping_;

  // Map from the AXID of a child to the AXID of the parent that owns it.
  HashMap<AXID, AXID> aria_owned_child_to_owner_mapping_;

  // Map from the AXID of a child to the AXID of its real parent in the tree if
  // we ignored aria-owns. This is needed in case the owner no longer wants to
  // own it.
  HashMap<AXID, AXID> aria_owned_child_to_real_parent_mapping_;

  // Reverse relation map from an ID (the ID attribute of a DOM element) to the
  // set of elements that at some time pointed to that ID via aria-owns,
  // aria-labelledby, aria-desribedby. This is *unvalidated*, it includes
  // possible extras and duplicates.
  // This is used so that:
  // - When an element with an ID is added to the tree or changes its ID, we can
  //   quickly determine if it affects an aria-owns relationship.
  // - When text changes, we can recompute any label or description based on it
  //   and fire the appropriate change events.
  HashMap<String, std::unique_ptr<HashSet<AXID>>> id_attr_to_related_mapping_;

  // Helpers that call back into object cache
  AXObject* ObjectFromAXID(AXID) const;
  AXObject* GetOrCreate(Node*);
  AXObject* Get(Node*);
  void ChildrenChanged(AXObject*);
  void TextChanged(AXObject*);
  void LabelChanged(Node*);

  DISALLOW_COPY_AND_ASSIGN(AXRelationCache);
};

}  // namespace blink

#endif
