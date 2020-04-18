// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/indexed_db/indexed_db_key_range.h"

#include "base/logging.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace content {

IndexedDBKeyRange::IndexedDBKeyRange() = default;

IndexedDBKeyRange::IndexedDBKeyRange(const IndexedDBKey& lower,
                                     const IndexedDBKey& upper,
                                     bool lower_open,
                                     bool upper_open)
    : lower_(lower),
      upper_(upper),
      lower_open_(lower_open),
      upper_open_(upper_open) {}

IndexedDBKeyRange::IndexedDBKeyRange(const IndexedDBKey& key)
    : lower_(key), upper_(key) {
}

IndexedDBKeyRange::IndexedDBKeyRange(const IndexedDBKeyRange& other) = default;
IndexedDBKeyRange::~IndexedDBKeyRange() = default;
IndexedDBKeyRange& IndexedDBKeyRange::operator=(
    const IndexedDBKeyRange& other) = default;

bool IndexedDBKeyRange::IsOnlyKey() const {
  if (lower_open_ || upper_open_)
    return false;
  if (IsEmpty())
    return false;

  return lower_.Equals(upper_);
}

bool IndexedDBKeyRange::IsEmpty() const {
  return !lower_.IsValid() && !upper_.IsValid();
}

}  // namespace content
