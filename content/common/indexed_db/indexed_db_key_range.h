// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_RANGE_H_
#define CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_RANGE_H_

#include "content/common/content_export.h"
#include "content/common/indexed_db/indexed_db_key.h"

namespace content {

class CONTENT_EXPORT IndexedDBKeyRange {
 public:
  IndexedDBKeyRange();
  explicit IndexedDBKeyRange(const IndexedDBKey& key);
  IndexedDBKeyRange(const IndexedDBKey& lower,
                    const IndexedDBKey& upper,
                    bool lower_open,
                    bool upper_open);
  IndexedDBKeyRange(const IndexedDBKeyRange& other);
  ~IndexedDBKeyRange();
  IndexedDBKeyRange& operator=(const IndexedDBKeyRange& other);

  const IndexedDBKey& lower() const { return lower_; }
  const IndexedDBKey& upper() const { return upper_; }
  bool lower_open() const { return lower_open_; }
  bool upper_open() const { return upper_open_; }

  bool IsOnlyKey() const;
  bool IsEmpty() const;

 private:
  IndexedDBKey lower_ = IndexedDBKey(blink::kWebIDBKeyTypeNull);
  IndexedDBKey upper_ = IndexedDBKey(blink::kWebIDBKeyTypeNull);
  bool lower_open_ = false;
  bool upper_open_ = false;
};

}  // namespace content

#endif  // CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_RANGE_H_
