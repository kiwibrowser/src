// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/mock_indexed_db_factory.h"

namespace content {

MockIndexedDBFactory::MockIndexedDBFactory() {
}

MockIndexedDBFactory::~MockIndexedDBFactory() {
}

IndexedDBFactory::OriginDBs MockIndexedDBFactory::GetOpenDatabasesForOrigin(
    const url::Origin& origin) const {
  return OriginDBs();
}

}  // namespace content
