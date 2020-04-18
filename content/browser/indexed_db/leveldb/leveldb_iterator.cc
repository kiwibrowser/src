// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/leveldb/leveldb_iterator.h"

namespace content {

bool LevelDBIterator::IsDetached() const {
  return false;
}

}  // namespace content
