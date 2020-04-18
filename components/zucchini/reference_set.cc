// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/reference_set.h"

#include <algorithm>
#include <iterator>

#include "base/logging.h"
#include "base/macros.h"
#include "components/zucchini/target_pool.h"

namespace zucchini {

namespace {

// Returns true if |refs| is sorted by location.
bool IsReferenceListSorted(const std::vector<IndirectReference>& refs) {
  return std::is_sorted(
      refs.begin(), refs.end(),
      [](const IndirectReference& a, const IndirectReference& b) {
        return a.location < b.location;
      });
}

}  // namespace

ReferenceSet::ReferenceSet(const ReferenceTypeTraits& traits,
                           const TargetPool& target_pool)
    : traits_(traits), target_pool_(target_pool) {}
ReferenceSet::ReferenceSet(ReferenceSet&&) = default;
ReferenceSet::~ReferenceSet() = default;

void ReferenceSet::InitReferences(ReferenceReader&& ref_reader) {
  DCHECK(references_.empty());
  for (auto ref = ref_reader.GetNext(); ref.has_value();
       ref = ref_reader.GetNext()) {
    references_.push_back(
        {ref->location, target_pool_.KeyForOffset(ref->target)});
  }
  DCHECK(IsReferenceListSorted(references_));
}

void ReferenceSet::InitReferences(const std::vector<Reference>& refs) {
  DCHECK(references_.empty());
  references_.reserve(refs.size());
  std::transform(refs.begin(), refs.end(), std::back_inserter(references_),
                 [&](const Reference& ref) -> IndirectReference {
                   return {ref.location, target_pool_.KeyForOffset(ref.target)};
                 });
  DCHECK(IsReferenceListSorted(references_));
}

IndirectReference ReferenceSet::at(offset_t offset) const {
  auto pos =
      std::upper_bound(references_.begin(), references_.end(), offset,
                       [](offset_t offset, const IndirectReference& ref) {
                         return offset < ref.location;
                       });

  DCHECK(pos != references_.begin());  // Iterators.
  --pos;
  DCHECK_LT(offset, pos->location + width());
  return *pos;
}

}  // namespace zucchini
