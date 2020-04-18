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

#include "third_party/blink/renderer/modules/accessibility/ax_relation_cache.h"

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/html/forms/html_label_element.h"
#include "third_party/blink/renderer/core/html/forms/labelable_element.h"

namespace blink {

AXRelationCache::AXRelationCache(AXObjectCacheImpl* object_cache)
    : object_cache_(object_cache) {}

AXRelationCache::~AXRelationCache() = default;

bool AXRelationCache::IsAriaOwned(const AXObject* child) const {
  return aria_owned_child_to_owner_mapping_.Contains(child->AXObjectID());
}

AXObject* AXRelationCache::GetAriaOwnedParent(const AXObject* child) const {
  return ObjectFromAXID(
      aria_owned_child_to_owner_mapping_.at(child->AXObjectID()));
}

// Update reverse relation map, where relation_source is related to target_ids.
void AXRelationCache::UpdateReverseRelations(const AXObject* relation_source,
                                             const Vector<String>& target_ids) {
  AXID relation_source_axid = relation_source->AXObjectID();

  // Add entries to reverse map.
  for (const String& target_id : target_ids) {
    HashSet<AXID>* source_axids = id_attr_to_related_mapping_.at(target_id);
    if (!source_axids) {
      source_axids = new HashSet<AXID>();
      id_attr_to_related_mapping_.Set(target_id,
                                      base::WrapUnique(source_axids));
    }
    source_axids->insert(relation_source_axid);
  }
}

static bool ContainsCycle(AXObject* owner, AXObject* child) {
  // Walk up the parents of the owner object, make sure that this child
  // doesn't appear there, as that would create a cycle.
  for (AXObject* parent = owner; parent; parent = parent->ParentObject()) {
    if (parent == child)
      return true;
    ;
  }
  return false;
}

bool AXRelationCache::IsValidOwnsRelation(AXObject* owner,
                                          AXObject* child) const {
  if (!child)
    return false;

  // If this child is already aria-owned by a different owner, continue.
  // It's an author error if this happens and we don't worry about which of
  // the two owners wins ownership of the child, as long as only one of them
  // does.
  if (IsAriaOwned(child) && GetAriaOwnedParent(child) != owner)
    return false;

  // You can't own yourself or an ancestor!
  if (ContainsCycle(owner, child))
    return false;

  return true;
}

void AXRelationCache::UnmapOwnedChildren(const AXObject* owner,
                                         const Vector<AXID> child_ids) {
  for (size_t i = 0; i < child_ids.size(); ++i) {
    // Find the AXObject for the child that this owner no longer owns.
    AXID removed_child_id = child_ids[i];
    AXObject* removed_child = ObjectFromAXID(removed_child_id);

    // It's possible that this child has already been owned by some other
    // owner, in which case we don't need to do anything.
    if (removed_child && GetAriaOwnedParent(removed_child) != owner)
      continue;

    // Remove it from the child -> owner mapping so it's not owned by this
    // owner anymore.
    aria_owned_child_to_owner_mapping_.erase(removed_child_id);

    if (removed_child) {
      // If the child still exists, find its "real" parent, and reparent it
      // back to its real parent in the tree by detaching it from its current
      // parent and calling childrenChanged on its real parent.
      removed_child->DetachFromParent();
      AXID real_parent_id =
          aria_owned_child_to_real_parent_mapping_.at(removed_child_id);
      AXObject* real_parent = ObjectFromAXID(real_parent_id);
      ChildrenChanged(real_parent);
    }

    // Remove the child -> original parent mapping too since this object has
    // now been reparented back to its original parent.
    aria_owned_child_to_real_parent_mapping_.erase(removed_child_id);
  }
}

void AXRelationCache::MapOwnedChildren(const AXObject* owner,
                                       const Vector<AXID> child_ids) {
  for (size_t i = 0; i < child_ids.size(); ++i) {
    // Find the AXObject for the child that will now be a child of this
    // owner.
    AXID added_child_id = child_ids[i];
    AXObject* added_child = ObjectFromAXID(added_child_id);

    // Add this child to the mapping from child to owner.
    aria_owned_child_to_owner_mapping_.Set(added_child_id, owner->AXObjectID());

    // Add its parent object to a mapping from child to real parent. If later
    // this owner doesn't own this child anymore, we need to return it to its
    // original parent.
    AXObject* original_parent = added_child->ParentObject();
    aria_owned_child_to_real_parent_mapping_.Set(added_child_id,
                                                 original_parent->AXObjectID());

    // Now detach the object from its original parent and call childrenChanged
    // on the original parent so that it can recompute its list of children.
    added_child->DetachFromParent();
    ChildrenChanged(original_parent);
  }
}

void AXRelationCache::UpdateAriaOwns(
    const AXObject* owner,
    const Vector<String>& owned_id_vector,
    HeapVector<Member<AXObject>>& validated_owned_children_result) {
  // Track reverse relations for future tree updates.
  UpdateReverseRelations(owner, owned_id_vector);

  //
  // Figure out the ids that actually correspond to children that exist
  // and that we can legally own (not cyclical, not already owned, etc.) and
  // update the maps and |validated_owned_children_result| based on that.
  //
  // Figure out the children that are owned by this object and are in the
  // tree.
  TreeScope& scope = owner->GetNode()->GetTreeScope();
  Vector<AXID> validated_owned_child_axids;
  for (const String& id_name : owned_id_vector) {
    Element* element = scope.getElementById(AtomicString(id_name));
    AXObject* child = GetOrCreate(element);
    if (IsValidOwnsRelation(const_cast<AXObject*>(owner), child)) {
      validated_owned_child_axids.push_back(child->AXObjectID());
      validated_owned_children_result.push_back(child);
    }
  }

  // Compare this to the current list of owned children, and exit early if
  // there are no changes.
  Vector<AXID> current_child_axids =
      aria_owner_to_children_mapping_.at(owner->AXObjectID());
  if (current_child_axids == validated_owned_child_axids)
    return;

  // The list of owned children has changed. Even if they were just reordered,
  // to be safe and handle all cases we remove all of the current owned
  // children and add the new list of owned children.
  UnmapOwnedChildren(owner, current_child_axids);
  MapOwnedChildren(owner, validated_owned_child_axids);

  // Finally, update the mapping from the owner to the list of child IDs.
  aria_owner_to_children_mapping_.Set(owner->AXObjectID(),
                                      validated_owned_child_axids);
}

// Fill source_objects with AXObjects for relations pointing to target.
void AXRelationCache::GetReverseRelated(
    Node* target,
    HeapVector<Member<AXObject>>& source_objects) {
  if (!target || !target->IsElementNode())
    return;

  Element* element = ToElement(target);
  if (!element->HasID())
    return;

  String id = element->GetIdAttribute();
  HashSet<AXID>* source_axids = id_attr_to_related_mapping_.at(id);
  if (!source_axids)
    return;

  for (const auto& source_axid : *source_axids) {
    AXObject* source_object = ObjectFromAXID(source_axid);
    if (source_object)
      source_objects.push_back(source_object);
  }
}

void AXRelationCache::UpdateRelatedTree(Node* node) {
  HeapVector<Member<AXObject>> related_sources;
  AXObject* related_target = Get(node);
  // If it's already owned, call childrenChanged on the owner to make sure
  // it's still an owner.
  if (related_target && IsAriaOwned(related_target)) {
    AXObject* owned_parent = GetAriaOwnedParent(related_target);
    DCHECK(owned_parent);
    ChildrenChanged(owned_parent);
  }

  // Ensure children are updated if there is a change.
  GetReverseRelated(node, related_sources);
  for (AXObject* related : related_sources) {
    if (related)
      ChildrenChanged(related);
  }

  UpdateRelatedText(node);
}

void AXRelationCache::UpdateRelatedText(Node* node) {
  // Walk up ancestor chain from node and refresh text of any related content.
  while (node) {
    // Reverse relations via aria-labelledby, aria-describedby, aria-owns.
    HeapVector<Member<AXObject>> related_sources;
    GetReverseRelated(node, related_sources);
    for (AXObject* related : related_sources) {
      if (related)
        TextChanged(related);
    }

    // Forward relation via <label for="[id]">.
    if (IsHTMLLabelElement(*node))
      LabelChanged(node);

    node = node->parentNode();
  }
}

void AXRelationCache::RemoveAXID(AXID obj_id) {
  if (aria_owner_to_children_mapping_.Contains(obj_id)) {
    Vector<AXID> child_axids = aria_owner_to_children_mapping_.at(obj_id);
    for (size_t i = 0; i < child_axids.size(); ++i)
      aria_owned_child_to_owner_mapping_.erase(child_axids[i]);
    aria_owner_to_children_mapping_.erase(obj_id);
  }
  aria_owned_child_to_owner_mapping_.erase(obj_id);
  aria_owned_child_to_real_parent_mapping_.erase(obj_id);
}

AXObject* AXRelationCache::ObjectFromAXID(AXID axid) const {
  return object_cache_->ObjectFromAXID(axid);
}

AXObject* AXRelationCache::Get(Node* node) {
  return object_cache_->Get(node);
}

AXObject* AXRelationCache::GetOrCreate(Node* node) {
  return object_cache_->GetOrCreate(node);
}

void AXRelationCache::ChildrenChanged(AXObject* object) {
  object_cache_->ChildrenChanged(object);
}

void AXRelationCache::TextChanged(AXObject* object) {
  object_cache_->TextChanged(object);
}

void AXRelationCache::LabelChanged(Node* node) {
  if (LabelableElement* control = ToHTMLLabelElement(node)->control())
    TextChanged(Get(control));
}

}  // namespace blink
