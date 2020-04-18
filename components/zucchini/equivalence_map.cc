// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/equivalence_map.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "components/zucchini/encoded_view.h"
#include "components/zucchini/patch_reader.h"
#include "components/zucchini/suffix_array.h"

namespace zucchini {

/******** Utility Functions ********/

double GetTokenSimilarity(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    offset_t src,
    offset_t dst) {
  DCHECK(old_image_index.IsToken(src));
  DCHECK(new_image_index.IsToken(dst));

  TypeTag old_type = old_image_index.LookupType(src);
  TypeTag new_type = new_image_index.LookupType(dst);
  if (old_type != new_type)
    return kMismatchFatal;

  // Raw comparison.
  if (!old_image_index.IsReference(src) && !new_image_index.IsReference(dst)) {
    return old_image_index.GetRawValue(src) == new_image_index.GetRawValue(dst)
               ? 1.0
               : -1.5;
  }

  const ReferenceSet& old_ref_set = old_image_index.refs(old_type);
  const ReferenceSet& new_ref_set = new_image_index.refs(new_type);
  IndirectReference old_reference = old_ref_set.at(src);
  IndirectReference new_reference = new_ref_set.at(dst);
  PoolTag pool_tag = old_ref_set.pool_tag();

  double affinity = targets_affinities[pool_tag.value()].AffinityBetween(
      old_reference.target_key, new_reference.target_key);

  // Both targets are not associated, which implies a weak match.
  if (affinity == 0.0)
    return 0.5 * old_ref_set.width();

  // At least one target is associated, so values are compared.
  return affinity > 0.0 ? old_ref_set.width() : -2.0;
}

double GetEquivalenceSimilarity(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    const Equivalence& equivalence) {
  double similarity = 0.0;
  for (offset_t k = 0; k < equivalence.length; ++k) {
    // Non-tokens are joined with the nearest previous token: skip until we
    // cover the unit.
    if (!new_image_index.IsToken(equivalence.dst_offset + k))
      continue;

    similarity += GetTokenSimilarity(
        old_image_index, new_image_index, targets_affinities,
        equivalence.src_offset + k, equivalence.dst_offset + k);
    if (similarity == kMismatchFatal)
      return kMismatchFatal;
  }
  return similarity;
}

EquivalenceCandidate ExtendEquivalenceForward(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    const EquivalenceCandidate& candidate,
    double min_similarity) {
  Equivalence equivalence = candidate.eq;
  offset_t best_k = equivalence.length;
  double current_similarity = candidate.similarity;
  double best_similarity = current_similarity;
  double current_penalty = min_similarity;
  for (offset_t k = best_k;
       equivalence.src_offset + k < old_image_index.size() &&
       equivalence.dst_offset + k < new_image_index.size();
       ++k) {
    // Mismatch in type, |candidate| cannot be extended further.
    if (old_image_index.LookupType(equivalence.src_offset + k) !=
        new_image_index.LookupType(equivalence.dst_offset + k)) {
      break;
    }

    if (!new_image_index.IsToken(equivalence.dst_offset + k)) {
      // Non-tokens are joined with the nearest previous token: skip until we
      // cover the unit, and extend |best_k| if applicable.
      if (best_k == k)
        best_k = k + 1;
      continue;
    }

    double similarity = GetTokenSimilarity(
        old_image_index, new_image_index, targets_affinities,
        equivalence.src_offset + k, equivalence.dst_offset + k);
    current_similarity += similarity;
    current_penalty = std::max(0.0, current_penalty) - similarity;

    if (current_similarity < 0.0 || current_penalty >= min_similarity)
      break;
    if (current_similarity >= best_similarity) {
      best_similarity = current_similarity;
      best_k = k + 1;
    }
  }
  equivalence.length = best_k;
  return {equivalence, best_similarity};
}

EquivalenceCandidate ExtendEquivalenceBackward(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    const EquivalenceCandidate& candidate,
    double min_similarity) {
  Equivalence equivalence = candidate.eq;
  offset_t best_k = 0;
  double current_similarity = candidate.similarity;
  double best_similarity = current_similarity;
  double current_penalty = 0.0;
  for (offset_t k = 1;
       k <= equivalence.dst_offset && k <= equivalence.src_offset; ++k) {
    // Mismatch in type, |candidate| cannot be extended further.
    if (old_image_index.LookupType(equivalence.src_offset - k) !=
        new_image_index.LookupType(equivalence.dst_offset - k)) {
      break;
    }

    // Non-tokens are joined with the nearest previous token: skip until we
    // reach the next token.
    if (!new_image_index.IsToken(equivalence.dst_offset - k))
      continue;

    DCHECK_EQ(old_image_index.LookupType(equivalence.src_offset - k),
              new_image_index.LookupType(equivalence.dst_offset -
                                         k));  // Sanity check.
    double similarity = GetTokenSimilarity(
        old_image_index, new_image_index, targets_affinities,
        equivalence.src_offset - k, equivalence.dst_offset - k);

    current_similarity += similarity;
    current_penalty = std::max(0.0, current_penalty) - similarity;

    if (current_similarity < 0.0 || current_penalty >= min_similarity)
      break;
    if (current_similarity >= best_similarity) {
      best_similarity = current_similarity;
      best_k = k;
    }
  }

  equivalence.dst_offset -= best_k;
  equivalence.src_offset -= best_k;
  equivalence.length += best_k;
  return {equivalence, best_similarity};
}

EquivalenceCandidate VisitEquivalenceSeed(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index,
    const std::vector<TargetsAffinity>& targets_affinities,
    offset_t src,
    offset_t dst,
    double min_similarity) {
  EquivalenceCandidate candidate{{src, dst, 0}, 0.0};  // Empty.
  if (!old_image_index.IsToken(src))
    return candidate;
  candidate =
      ExtendEquivalenceForward(old_image_index, new_image_index,
                               targets_affinities, candidate, min_similarity);
  if (candidate.similarity < min_similarity)
    return candidate;  // Not worth exploring any more.
  return ExtendEquivalenceBackward(old_image_index, new_image_index,
                                   targets_affinities, candidate,
                                   min_similarity);
}

/******** OffsetMapper ********/

OffsetMapper::OffsetMapper(std::vector<Equivalence>&& equivalences)
    : equivalences_(std::move(equivalences)) {
  DCHECK(std::is_sorted(equivalences_.begin(), equivalences_.end(),
                        [](const Equivalence& a, const Equivalence& b) {
                          return a.src_offset < b.src_offset;
                        }));
}

OffsetMapper::OffsetMapper(EquivalenceSource&& equivalence_source) {
  for (auto e = equivalence_source.GetNext(); e.has_value();
       e = equivalence_source.GetNext()) {
    equivalences_.push_back(*e);
  }
  PruneEquivalencesAndSortBySource(&equivalences_);
}

OffsetMapper::OffsetMapper(const EquivalenceMap& equivalence_map)
    : equivalences_(equivalence_map.size()) {
  std::transform(equivalence_map.begin(), equivalence_map.end(),
                 equivalences_.begin(),
                 [](const EquivalenceCandidate& c) { return c.eq; });
  PruneEquivalencesAndSortBySource(&equivalences_);
}

OffsetMapper::~OffsetMapper() = default;

offset_t OffsetMapper::ForwardProject(offset_t offset) const {
  auto pos = std::upper_bound(
      equivalences_.begin(), equivalences_.end(), offset,
      [](offset_t a, const Equivalence& b) { return a < b.src_offset; });
  if (pos != equivalences_.begin()) {
    if (pos == equivalences_.end() || offset < pos[-1].src_end() ||
        offset - pos[-1].src_end() < pos->src_offset - offset) {
      --pos;
    }
  }
  return offset - pos->src_offset + pos->dst_offset;
}

void OffsetMapper::ForwardProjectAll(std::vector<offset_t>* offsets) const {
  DCHECK(std::is_sorted(offsets->begin(), offsets->end()));
  auto current = equivalences_.begin();
  for (auto& src : *offsets) {
    while (current != end() && current->src_end() <= src) {
      ++current;
    }

    if (current != end() && current->src_offset <= src) {
      src = src - current->src_offset + current->dst_offset;
    } else {
      src = kInvalidOffset;
    }
  }
  offsets->erase(std::remove(offsets->begin(), offsets->end(), kInvalidOffset),
                 offsets->end());
  offsets->shrink_to_fit();
}

void OffsetMapper::PruneEquivalencesAndSortBySource(
    std::vector<Equivalence>* equivalences) {
  std::sort(equivalences->begin(), equivalences->end(),
            [](const Equivalence& a, const Equivalence& b) {
              return a.src_offset < b.src_offset;
            });

  for (auto current = equivalences->begin(); current != equivalences->end();
       ++current) {
    // A "reaper" is an equivalence after |current| that overlaps with it, but
    // is longer, and so truncates |current|.  For example:
    //  ******  <=  |current|
    //    **
    //    ****
    //      ****
    //      **********  <= |next| as reaper.
    // If a reaper is found (as |next|), every equivalence strictly between
    // |current| and |next| would be truncated to 0 and discarded. Handling this
    // case is important to avoid O(n^2) behavior.
    bool next_is_reaper = false;

    // Look ahead to resolve overlaps, until a better candidate is found.
    auto next = current + 1;
    for (; next != equivalences->end(); ++next) {
      DCHECK_GE(next->src_offset, current->src_offset);
      if (next->src_offset >= current->src_end())
        break;  // No more overlap.

      if (current->length < next->length) {
        // |next| is better: So it is a reaper that shrinks |current|.
        offset_t delta = current->src_end() - next->src_offset;
        current->length -= delta;
        next_is_reaper = true;
        break;
      }
    }

    if (next_is_reaper) {
      // Discard all equivalences strictly between |cur| and |next|.
      for (auto reduced = current + 1; reduced != next; ++reduced)
        reduced->length = 0;
      current = next - 1;
    } else {
      // Shrink all equivalences that overlap with |current|. These are all
      // worse than |current| since no reaper is found.
      for (auto reduced = current + 1; reduced != next; ++reduced) {
        offset_t delta = current->src_end() - reduced->src_offset;
        reduced->length -= std::min(reduced->length, delta);
        reduced->src_offset += delta;
        reduced->dst_offset += delta;
        DCHECK_EQ(reduced->src_offset, current->src_end());
      }
    }
  }

  // Discard all equivalences with length == 0.
  equivalences->erase(std::remove_if(equivalences->begin(), equivalences->end(),
                                     [](const Equivalence& equivalence) {
                                       return equivalence.length == 0;
                                     }),
                      equivalences->end());
}

/******** EquivalenceMap ********/

EquivalenceMap::EquivalenceMap() = default;

EquivalenceMap::EquivalenceMap(std::vector<EquivalenceCandidate>&& equivalences)
    : candidates_(std::move(equivalences)) {
  SortByDestination();
}

EquivalenceMap::EquivalenceMap(EquivalenceMap&&) = default;

EquivalenceMap::~EquivalenceMap() = default;

void EquivalenceMap::Build(
    const std::vector<offset_t>& old_sa,
    const EncodedView& old_view,
    const EncodedView& new_view,
    const std::vector<TargetsAffinity>& targets_affinities,
    double min_similarity) {
  DCHECK_EQ(old_sa.size(), old_view.size());

  CreateCandidates(old_sa, old_view, new_view, targets_affinities,
                   min_similarity);
  SortByDestination();
  Prune(old_view, new_view, targets_affinities, min_similarity);

  offset_t coverage = 0;
  offset_t current_offset = 0;
  for (auto candidate : candidates_) {
    DCHECK_GE(candidate.eq.dst_offset, current_offset);
    coverage += candidate.eq.length;
    current_offset = candidate.eq.dst_end();
  }
  LOG(INFO) << "Equivalence Count: " << size();
  LOG(INFO) << "Coverage / Extra / Total: " << coverage << " / "
            << new_view.size() - coverage << " / " << new_view.size();
}

void EquivalenceMap::CreateCandidates(
    const std::vector<offset_t>& old_sa,
    const EncodedView& old_view,
    const EncodedView& new_view,
    const std::vector<TargetsAffinity>& targets_affinities,
    double min_similarity) {
  candidates_.clear();

  // This is an heuristic to find 'good' equivalences on encoded views.
  // Equivalences are found in ascending order of |new_image|.
  offset_t dst_offset = 0;

  while (dst_offset < new_view.size()) {
    if (!new_view.IsToken(dst_offset)) {
      ++dst_offset;
      continue;
    }
    auto match =
        SuffixLowerBound(old_sa, old_view.begin(),
                         new_view.begin() + dst_offset, new_view.end());

    offset_t next_dst_offset = dst_offset + 1;
    // TODO(huangs): Clean up.
    double best_similarity = min_similarity;
    EquivalenceCandidate best_candidate = {{0, 0, 0}, 0.0};
    for (auto it = match; it != old_sa.end(); ++it) {
      EquivalenceCandidate candidate = VisitEquivalenceSeed(
          old_view.image_index(), new_view.image_index(), targets_affinities,
          static_cast<offset_t>(*it), dst_offset, min_similarity);
      if (candidate.similarity > best_similarity) {
        best_candidate = candidate;
        best_similarity = candidate.similarity;
        next_dst_offset = candidate.eq.dst_end();
      } else {
        break;
      }
    }
    for (auto it = match; it != old_sa.begin(); --it) {
      EquivalenceCandidate candidate = VisitEquivalenceSeed(
          old_view.image_index(), new_view.image_index(), targets_affinities,
          static_cast<offset_t>(it[-1]), dst_offset, min_similarity);
      if (candidate.similarity > best_similarity) {
        best_candidate = candidate;
        best_similarity = candidate.similarity;
        next_dst_offset = candidate.eq.dst_end();
      } else {
        break;
      }
    }
    if (best_candidate.similarity >= min_similarity) {
      candidates_.push_back(best_candidate);
    }

    dst_offset = next_dst_offset;
  }
}

void EquivalenceMap::SortByDestination() {
  std::sort(candidates_.begin(), candidates_.end(),
            [](const EquivalenceCandidate& a, const EquivalenceCandidate& b) {
              return a.eq.dst_offset < b.eq.dst_offset;
            });
}

void EquivalenceMap::Prune(
    const EncodedView& old_view,
    const EncodedView& new_view,
    const std::vector<TargetsAffinity>& target_affinities,
    double min_similarity) {
  // TODO(etiennep): unify with
  // OffsetMapper::PruneEquivalencesAndSortBySource().
  for (auto current = candidates_.begin(); current != candidates_.end();
       ++current) {
    if (current->similarity < min_similarity)
      continue;  // This candidate will be discarded anyways.

    bool next_is_reaper = false;

    // Look ahead to resolve overlaps, until a better candidate is found.
    auto next = current + 1;
    for (; next != candidates_.end(); ++next) {
      DCHECK_GE(next->eq.dst_offset, current->eq.dst_offset);
      if (next->eq.dst_offset >= current->eq.dst_offset + current->eq.length)
        break;  // No more overlap.

      if (current->similarity < next->similarity) {
        // |next| is better: So it is a reaper that shrinks |current|.
        offset_t delta = current->eq.dst_end() - next->eq.dst_offset;
        current->eq.length -= delta;
        current->similarity = GetEquivalenceSimilarity(
            old_view.image_index(), new_view.image_index(), target_affinities,
            current->eq);

        next_is_reaper = true;
        break;
      }
    }

    if (next_is_reaper) {
      // Discard all equivalences strictly between |cur| and |next|.
      for (auto reduced = current + 1; reduced != next; ++reduced) {
        reduced->eq.length = 0;
        reduced->similarity = 0;
      }
      current = next - 1;
    } else {
      // Shrinks all overlapping candidates following and worse than |current|.
      for (auto reduced = current + 1; reduced != next; ++reduced) {
        offset_t delta = current->eq.dst_end() - reduced->eq.dst_offset;
        reduced->eq.length -= std::min(reduced->eq.length, delta);
        reduced->eq.src_offset += delta;
        reduced->eq.dst_offset += delta;
        reduced->similarity = GetEquivalenceSimilarity(
            old_view.image_index(), new_view.image_index(), target_affinities,
            reduced->eq);
        DCHECK_EQ(reduced->eq.dst_offset, current->eq.dst_end());
      }
    }
  }

  // Discard all candidates with similarity smaller than |min_similarity|.
  candidates_.erase(
      std::remove_if(candidates_.begin(), candidates_.end(),
                     [min_similarity](const EquivalenceCandidate& candidate) {
                       return candidate.similarity < min_similarity;
                     }),
      candidates_.end());
}

}  // namespace zucchini
