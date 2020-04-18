// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/indexed_db/mock_webidbcallbacks.h"

namespace content {

MockWebIDBCallbacks::MockWebIDBCallbacks() {}

MockWebIDBCallbacks::~MockWebIDBCallbacks() {}

void MockWebIDBCallbacks::OnSuccess(blink::WebIDBKey key,
                                    blink::WebIDBKey primaryKey,
                                    blink::WebIDBValue value) {
  DoOnSuccess(key, primaryKey, value);
}

void MockWebIDBCallbacks::OnSuccess(blink::WebIDBCursor* cursor,
                                    blink::WebIDBKey key,
                                    blink::WebIDBKey primaryKey,
                                    blink::WebIDBValue value) {
  DoOnSuccess(cursor, key, primaryKey, value);
}

void MockWebIDBCallbacks::OnSuccess(blink::WebIDBKey key) {
  DoOnSuccess(key);
}

void MockWebIDBCallbacks::OnSuccess(blink::WebIDBValue value) {
  DoOnSuccess(value);
}

void MockWebIDBCallbacks::OnSuccess(
    blink::WebVector<blink::WebIDBValue> values) {
  DoOnSuccess(values);
}

}  // namespace content
