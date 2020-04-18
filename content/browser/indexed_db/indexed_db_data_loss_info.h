// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATA_LOSS_INFO_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATA_LOSS_INFO_H_

#include <string>

#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace content {

struct IndexedDBDataLossInfo {
  blink::WebIDBDataLoss status = blink::kWebIDBDataLossNone;
  std::string message;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATA_LOSS_INFO_H_
