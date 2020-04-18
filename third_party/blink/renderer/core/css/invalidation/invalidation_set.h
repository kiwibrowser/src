/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_INVALIDATION_INVALIDATION_SET_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_INVALIDATION_INVALIDATION_SET_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/invalidation/invalidation_flags.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"

namespace blink {

class Element;
class TracedValue;

enum InvalidationType { kInvalidateDescendants, kInvalidateSiblings };

class InvalidationSet;

struct CORE_EXPORT InvalidationSetDeleter {
  static void Destruct(const InvalidationSet*);
};

// Tracks data to determine which descendants in a DOM subtree, or
// siblings and their descendants, need to have style recalculated.
//
// Some example invalidation sets:
//
// .z {}
//   For class z we will have a DescendantInvalidationSet with invalidatesSelf
//   (the element itself is invalidated).
//
// .y .z {}
//   For class y we will have a DescendantInvalidationSet containing class z.
//
// .x ~ .z {}
//   For class x we will have a SiblingInvalidationSet containing class z, with
//   invalidatesSelf (the sibling itself is invalidated).
//
// .w ~ .y .z {}
//   For class w we will have a SiblingInvalidationSet containing class y, with
//   the SiblingInvalidationSet havings siblingDescendants containing class z.
//
// .v * {}
//   For class v we will have a DescendantInvalidationSet with
//   wholeSubtreeInvalid.
//
// .u ~ * {}
//   For class u we will have a SiblingInvalidationSet with wholeSubtreeInvalid
//   and invalidatesSelf (for all siblings, the sibling itself is invalidated).
//
// .t .v, .t ~ .z {}
//   For class t we will have a SiblingInvalidationSet containing class z, with
//   the SiblingInvalidationSet also holding descendants containing class v.
//
// We avoid virtual functions to minimize space consumption.
class CORE_EXPORT InvalidationSet
    : public WTF::RefCounted<InvalidationSet, InvalidationSetDeleter> {
  USING_FAST_MALLOC_WITH_TYPE_NAME(blink::InvalidationSet);

 public:
  InvalidationType GetType() const {
    return static_cast<InvalidationType>(type_);
  }
  bool IsDescendantInvalidationSet() const {
    return GetType() == kInvalidateDescendants;
  }
  bool IsSiblingInvalidationSet() const {
    return GetType() == kInvalidateSiblings;
  }

  static void CacheTracingFlag();

  bool InvalidatesElement(Element&) const;
  bool InvalidatesTagName(Element&) const;

  void AddClass(const AtomicString& class_name);
  void AddId(const AtomicString& id);
  void AddTagName(const AtomicString& tag_name);
  void AddAttribute(const AtomicString& attribute_local_name);

  void SetInvalidationFlags(InvalidationFlags flags) {
    invalidation_flags_ = flags;
  };

  void SetWholeSubtreeInvalid();
  bool WholeSubtreeInvalid() const {
    return invalidation_flags_.WholeSubtreeInvalid();
  }

  void SetInvalidatesSelf() { invalidates_self_ = true; }
  bool InvalidatesSelf() const { return invalidates_self_; }

  void SetTreeBoundaryCrossing() {
    invalidation_flags_.SetTreeBoundaryCrossing(true);
  }
  bool TreeBoundaryCrossing() const {
    return invalidation_flags_.TreeBoundaryCrossing();
  }

  void SetInsertionPointCrossing() {
    invalidation_flags_.SetInsertionPointCrossing(true);
  }
  bool InsertionPointCrossing() const {
    return invalidation_flags_.InsertionPointCrossing();
  }

  void SetCustomPseudoInvalid() {
    invalidation_flags_.SetInvalidateCustomPseudo(true);
  }
  bool CustomPseudoInvalid() const {
    return invalidation_flags_.InvalidateCustomPseudo();
  }

  void SetInvalidatesSlotted() {
    invalidation_flags_.SetInvalidatesSlotted(true);
  }
  bool InvalidatesSlotted() const {
    return invalidation_flags_.InvalidatesSlotted();
  }

  const InvalidationFlags GetInvalidationFlags() const {
    return invalidation_flags_;
  };

  void SetInvalidatesParts() { invalidation_flags_.SetInvalidatesParts(true); }
  bool InvalidatesParts() const {
    return invalidation_flags_.InvalidatesParts();
  }

  bool IsEmpty() const {
    return !classes_ && !ids_ && !tag_names_ && !attributes_ &&
           !invalidation_flags_.InvalidateCustomPseudo() &&
           !invalidation_flags_.InsertionPointCrossing() &&
           !invalidation_flags_.InvalidatesSlotted() &&
           !invalidation_flags_.InvalidatesParts();
  }

  bool IsAlive() const { return is_alive_; }

  void ToTracedValue(TracedValue*) const;

#ifndef NDEBUG
  void Show() const;
#endif

  const HashSet<AtomicString>& ClassSetForTesting() const {
    DCHECK(classes_);
    return *classes_;
  }
  const HashSet<AtomicString>& IdSetForTesting() const {
    DCHECK(ids_);
    return *ids_;
  }
  const HashSet<AtomicString>& TagNameSetForTesting() const {
    DCHECK(tag_names_);
    return *tag_names_;
  }
  const HashSet<AtomicString>& AttributeSetForTesting() const {
    DCHECK(attributes_);
    return *attributes_;
  }

  void Combine(const InvalidationSet& other);

  // Returns a singleton DescendantInvalidationSet which only has
  // InvalidatesSelf() set and is otherwise empty. As this is a common
  // invalidation set for features only found in rightmost compounds,
  // sharing this singleton between such features saves a lot of memory on
  // sites with a big number of style rules.
  static InvalidationSet* SelfInvalidationSet();
  bool IsSelfInvalidationSet() const { return this == SelfInvalidationSet(); }

 protected:
  explicit InvalidationSet(InvalidationType);

  ~InvalidationSet() {
    CHECK(is_alive_);
    is_alive_ = false;
  }

 private:
  friend struct InvalidationSetDeleter;
  void Destroy() const;

  HashSet<AtomicString>& EnsureClassSet();
  HashSet<AtomicString>& EnsureIdSet();
  HashSet<AtomicString>& EnsureTagNameSet();
  HashSet<AtomicString>& EnsureAttributeSet();

  // FIXME: optimize this if it becomes a memory issue.
  std::unique_ptr<HashSet<AtomicString>> classes_;
  std::unique_ptr<HashSet<AtomicString>> ids_;
  std::unique_ptr<HashSet<AtomicString>> tag_names_;
  std::unique_ptr<HashSet<AtomicString>> attributes_;

  InvalidationFlags invalidation_flags_;

  unsigned type_ : 1;

  // If true, the element or sibling itself is invalid.
  unsigned invalidates_self_ : 1;

  // If true, the instance is alive and can be used.
  unsigned is_alive_ : 1;
  DISALLOW_COPY_AND_ASSIGN(InvalidationSet);
};

class CORE_EXPORT DescendantInvalidationSet final : public InvalidationSet {
 public:
  static scoped_refptr<DescendantInvalidationSet> Create() {
    return base::AdoptRef(new DescendantInvalidationSet);
  }

 private:
  DescendantInvalidationSet() : InvalidationSet(kInvalidateDescendants) {}
};

class CORE_EXPORT SiblingInvalidationSet final : public InvalidationSet {
 public:
  static scoped_refptr<SiblingInvalidationSet> Create(
      scoped_refptr<DescendantInvalidationSet> descendants) {
    return base::AdoptRef(new SiblingInvalidationSet(std::move(descendants)));
  }

  unsigned MaxDirectAdjacentSelectors() const {
    return max_direct_adjacent_selectors_;
  }
  void UpdateMaxDirectAdjacentSelectors(unsigned value) {
    max_direct_adjacent_selectors_ =
        std::max(value, max_direct_adjacent_selectors_);
  }

  DescendantInvalidationSet* SiblingDescendants() const {
    return sibling_descendant_invalidation_set_.get();
  }
  DescendantInvalidationSet& EnsureSiblingDescendants();

  DescendantInvalidationSet* Descendants() const {
    return descendant_invalidation_set_.get();
  }
  DescendantInvalidationSet& EnsureDescendants();

 private:
  explicit SiblingInvalidationSet(
      scoped_refptr<DescendantInvalidationSet> descendants);

  // Indicates the maximum possible number of siblings affected.
  unsigned max_direct_adjacent_selectors_;

  // Indicates the descendants of siblings.
  scoped_refptr<DescendantInvalidationSet> sibling_descendant_invalidation_set_;

  // Null if a given feature (class, attribute, id, pseudo-class) has only
  // a SiblingInvalidationSet and not also a DescendantInvalidationSet.
  scoped_refptr<DescendantInvalidationSet> descendant_invalidation_set_;
};

using InvalidationSetVector = Vector<scoped_refptr<InvalidationSet>>;

struct InvalidationLists {
  InvalidationSetVector descendants;
  InvalidationSetVector siblings;
};

DEFINE_TYPE_CASTS(DescendantInvalidationSet,
                  InvalidationSet,
                  value,
                  value->IsDescendantInvalidationSet(),
                  value.IsDescendantInvalidationSet());
DEFINE_TYPE_CASTS(SiblingInvalidationSet,
                  InvalidationSet,
                  value,
                  value->IsSiblingInvalidationSet(),
                  value.IsSiblingInvalidationSet());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_INVALIDATION_INVALIDATION_SET_H_
