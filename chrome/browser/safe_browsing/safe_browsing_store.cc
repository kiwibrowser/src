// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/safe_browsing_store.h"

#include <algorithm>

#include "base/logging.h"

namespace {

// Return |true| if the range is sorted by the given comparator.
template <typename CTI, typename LESS>
bool sorted(CTI beg, CTI end, LESS less) {
  while ((end - beg) > 2) {
    CTI n = beg++;
    if (less(*beg, *n))
      return false;
  }
  return true;
}

// Find items matching between |subs| and |adds|, and remove them.  To minimize
// copies, the inputs are processing in parallel, so |subs| and |adds| should be
// compatibly ordered (either by SBAddPrefixLess or SBAddPrefixHashLess).
//
// |predAddSub| provides add < sub, |predSubAdd| provides sub < add, for the
// tightest compare appropriate (see calls in SBProcessSubs).
template <typename SubsT, typename AddsT,
          typename PredAddSubT, typename PredSubAddT>
void KnockoutSubs(SubsT* subs, AddsT* adds,
                  PredAddSubT predAddSub, PredSubAddT predSubAdd) {
  // Keep a pair of output iterators for writing kept items.  Due to
  // deletions, these may lag the main iterators.  Using erase() on
  // individual items would result in O(N^2) copies.  Using std::list
  // would work around that, at double or triple the memory cost.
  typename AddsT::iterator add_out = adds->begin();
  typename SubsT::iterator sub_out = subs->begin();

  // Current location in containers.
  // TODO(shess): I want these to be const_iterator, but then
  // std::copy() gets confused.  Could snag a const_iterator add_end,
  // or write an inline std::copy(), but it seems like I'm doing
  // something wrong.
  typename AddsT::iterator add_iter = adds->begin();
  typename SubsT::iterator sub_iter = subs->begin();

  while (add_iter != adds->end() && sub_iter != subs->end()) {
    // If |*sub_iter| < |*add_iter|, retain the sub.
    if (predSubAdd(*sub_iter, *add_iter)) {
      *sub_out = *sub_iter;
      ++sub_out;
      ++sub_iter;

      // If |*add_iter| < |*sub_iter|, retain the add.
    } else if (predAddSub(*add_iter, *sub_iter)) {
      *add_out = *add_iter;
      ++add_out;
      ++add_iter;

      // Drop equal items.
    } else {
      ++add_iter;
      ++sub_iter;
    }
  }

  // Erase any leftover gap.
  adds->erase(add_out, add_iter);
  subs->erase(sub_out, sub_iter);
}

// Remove deleted items (|chunk_id| in |del_set|) from the container.
template <typename ItemsT>
void RemoveDeleted(ItemsT* items, const base::hash_set<int32_t>& del_set) {
  DCHECK(items);

  // Move items from |iter| to |end_iter|, skipping items in |del_set|.
  typename ItemsT::iterator end_iter = items->begin();
  for (typename ItemsT::iterator iter = end_iter;
       iter != items->end(); ++iter) {
    if (del_set.count(iter->chunk_id) == 0) {
      *end_iter = *iter;
      ++end_iter;
    }
  }
  items->erase(end_iter, items->end());
}

}  // namespace

namespace safe_browsing {

void SBProcessSubs(SBAddPrefixes* add_prefixes,
                   SBSubPrefixes* sub_prefixes,
                   std::vector<SBAddFullHash>* add_full_hashes,
                   std::vector<SBSubFullHash>* sub_full_hashes,
                   const base::hash_set<int32_t>& add_chunks_deleted,
                   const base::hash_set<int32_t>& sub_chunks_deleted) {
  // It is possible to structure templates and template
  // specializations such that the following calls work without having
  // to qualify things.  It becomes very arbitrary, though, and less
  // clear how things are working.

  // Make sure things are sorted appropriately.
  DCHECK(sorted(add_prefixes->begin(), add_prefixes->end(),
                SBAddPrefixLess<SBAddPrefix,SBAddPrefix>));
  DCHECK(sorted(sub_prefixes->begin(), sub_prefixes->end(),
                SBAddPrefixLess<SBSubPrefix,SBSubPrefix>));
  DCHECK(sorted(add_full_hashes->begin(), add_full_hashes->end(),
                SBAddPrefixHashLess<SBAddFullHash,SBAddFullHash>));
  DCHECK(sorted(sub_full_hashes->begin(), sub_full_hashes->end(),
                SBAddPrefixHashLess<SBSubFullHash,SBSubFullHash>));

  // Factor out the prefix subs.
  KnockoutSubs(sub_prefixes, add_prefixes,
               SBAddPrefixLess<SBAddPrefix,SBSubPrefix>,
               SBAddPrefixLess<SBSubPrefix,SBAddPrefix>);

  // Factor out the full-hash subs.
  KnockoutSubs(sub_full_hashes, add_full_hashes,
               SBAddPrefixHashLess<SBAddFullHash,SBSubFullHash>,
               SBAddPrefixHashLess<SBSubFullHash,SBAddFullHash>);

  // Remove items from the deleted chunks.  This is done after other
  // processing to allow subs to knock out adds (and be removed) even
  // if the add's chunk is deleted.
  RemoveDeleted(add_prefixes, add_chunks_deleted);
  RemoveDeleted(sub_prefixes, sub_chunks_deleted);
  RemoveDeleted(add_full_hashes, add_chunks_deleted);
  RemoveDeleted(sub_full_hashes, sub_chunks_deleted);
}

}  // namespace safe_browsing
