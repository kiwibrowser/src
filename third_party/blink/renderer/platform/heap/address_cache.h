// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_ADDRESS_CACHE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_ADDRESS_CACHE_H_

#include "third_party/blink/renderer/platform/heap/blink_gc.h"

namespace blink {

// Negative cache for addresses outside of Blink's garbage collected heap.
// - Internally maps to pages (NormalPage) to cover a larger range of addresses.
// - Requires flushing when adding new addresses.
class PLATFORM_EXPORT AddressCache {
  USING_FAST_MALLOC(AddressCache);

 public:
  AddressCache() : enabled_(false), has_entries_(false), dirty_(false) {
    // Start by flushing the cache in a non-empty state to initialize all the
    // cache entries.
    for (size_t i = 0; i < kNumberOfEntries; ++i)
      entries_[i] = nullptr;
  }

  void EnableLookup() { enabled_ = true; }
  void DisableLookup() { enabled_ = false; }

  void MarkDirty() { dirty_ = true; }
  void Flush();
  void FlushIfDirty();
  bool IsEmpty() { return !has_entries_; }

  // Perform a lookup in the cache. Returns true if the address is guaranteed
  // to not in Blink's heap and false otherwise.
  bool Lookup(Address);

  // Add an entry to the cache.
  void AddEntry(Address);

 private:
  static constexpr size_t kNumberOfEntriesLog2 = 12;
  static constexpr size_t kNumberOfEntries = 1 << kNumberOfEntriesLog2;

  static size_t GetHash(Address);

  Address entries_[kNumberOfEntries];
  bool enabled_ : 1;
  bool has_entries_ : 1;
  bool dirty_ : 1;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_ADDRESS_CACHE_H_
