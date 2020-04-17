// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_BASE_STD_UTIL_H_
#define OSP_BASE_STD_UTIL_H_

#include <map>

#include "absl/algorithm/container.h"

namespace openscreen {

template <typename Key, typename Value>
void RemoveValueFromMap(std::map<Key, Value*>* map, Value* value) {
  for (auto it = map->begin(); it != map->end();) {
    if (it->second == value) {
      it = map->erase(it);
    } else {
      ++it;
    }
  }
}

template <typename ForwardIteratingContainer>
bool AreElementsSortedAndUnique(const ForwardIteratingContainer& c) {
  return absl::c_is_sorted(c) && (absl::c_adjacent_find(c) == c.end());
}

template <typename RandomAccessContainer>
void SortAndDedupeElements(RandomAccessContainer* c) {
  std::sort(c->begin(), c->end());
  const auto new_end = std::unique(c->begin(), c->end());
  c->erase(new_end, c->end());
}

}  // namespace openscreen

#endif  // OSP_BASE_STD_UTIL_H_
