// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_COMPARATOR_H_
#define CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_COMPARATOR_H_

#include "base/strings/string_piece.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT LevelDBComparator {
 public:
  virtual ~LevelDBComparator() {}

  virtual int Compare(const base::StringPiece& a,
                      const base::StringPiece& b) const = 0;
  virtual const char* Name() const = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_COMPARATOR_H_
