// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_pointer_set.h"

namespace crazy {

// Result type for the BinarySearch function below.
// The packing generates smaller and faster machine code on ARM and x86.
struct SearchResult {
  bool found : 1;
  size_t pos : sizeof(size_t) * CHAR_BIT - 1;
};

static SearchResult BinarySearch(const Vector<void*>& items, void* key) {
  auto key_val = reinterpret_cast<uintptr_t>(key);
  size_t min = 0, max = items.GetCount();
  while (min < max) {
    size_t mid = min + ((max - min) >> 1);
    auto mid_val = reinterpret_cast<uintptr_t>(items[mid]);
    if (mid_val == key_val) {
      return {true, mid};
    }
    if (mid_val < key_val)
      min = mid + 1;
    else
      max = mid;
  }
  return {false, min};
}

bool PointerSet::Add(void* item) {
  SearchResult ret = BinarySearch(items_, item);
  if (ret.found)
    return true;

  items_.InsertAt(ret.pos, item);
  return false;
}

bool PointerSet::Remove(void* item) {
  SearchResult ret = BinarySearch(items_, item);
  if (!ret.found)
    return false;

  items_.RemoveAt(ret.pos);
  return true;
}

bool PointerSet::Has(void* item) const {
  SearchResult ret = BinarySearch(items_, item);
  return ret.found;
}

}  // namespace crazy
