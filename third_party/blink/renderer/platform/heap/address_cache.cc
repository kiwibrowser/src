// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/address_cache.h"

#include "third_party/blink/renderer/platform/heap/heap_page.h"

namespace blink {

void AddressCache::Flush() {
  if (has_entries_) {
    for (size_t i = 0; i < kNumberOfEntries; ++i)
      entries_[i] = nullptr;
    has_entries_ = false;
  }
  dirty_ = false;
}

void AddressCache::FlushIfDirty() {
  if (dirty_) {
    Flush();
    dirty_ = false;
  }
}

size_t AddressCache::GetHash(Address address) {
  size_t value = (reinterpret_cast<size_t>(address) >> kBlinkPageSizeLog2);
  value ^= value >> kNumberOfEntriesLog2;
  value ^= value >> (kNumberOfEntriesLog2 * 2);
  value &= kNumberOfEntries - 1;
  return value & ~1;  // Returns only even number.
}

bool AddressCache::Lookup(Address address) {
  DCHECK(enabled_);
  DCHECK(!dirty_);

  size_t index = GetHash(address);
  DCHECK(!(index & 1));
  Address cache_page = RoundToBlinkPageStart(address);
  if (entries_[index] == cache_page)
    return entries_[index];
  if (entries_[index + 1] == cache_page)
    return entries_[index + 1];
  return false;
}

void AddressCache::AddEntry(Address address) {
  has_entries_ = true;
  size_t index = GetHash(address);
  DCHECK(!(index & 1));
  Address cache_page = RoundToBlinkPageStart(address);
  entries_[index + 1] = entries_[index];
  entries_[index] = cache_page;
}

}  // namespace blink
