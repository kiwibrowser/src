// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_OBSERVATION_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_OBSERVATION_H_

#include "third_party/blink/public/platform/modules/indexeddb/web_idb_key_range.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_value.h"

namespace blink {

struct WebIDBObservation {
  int64_t object_store_id;
  WebIDBOperationType type;
  WebIDBKeyRange key_range;
  WebIDBValue value;

  WebIDBObservation(int64_t object_store_id,
                    WebIDBOperationType type,
                    WebIDBKeyRange key_range,
                    WebIDBValue value)
      : object_store_id(object_store_id),
        type(type),
        key_range(key_range),
        value(std::move(value)) {}

  WebIDBObservation(WebIDBObservation&&) = default;
  WebIDBObservation& operator=(WebIDBObservation&&) = default;

 private:
  // WebIDBObservation has to be move-only, because WebIDBValue is move-only.
  // Making the restriction explicit results in slightly better compilation
  // error messages in code that attempts copying.
  WebIDBObservation(const WebIDBObservation&) = delete;
  WebIDBObservation& operator=(const WebIDBObservation&) = delete;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_INDEXEDDB_WEB_IDB_OBSERVATION_H_
