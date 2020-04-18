// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_observer.h"

namespace content {

IndexedDBObserver::Options::Options(bool include_transaction,
                                    bool no_records,
                                    bool values,
                                    uint16_t types)
    : include_transaction(include_transaction),
      no_records(no_records),
      values(values),
      operation_types(types) {}

IndexedDBObserver::Options::Options(const Options&) = default;

IndexedDBObserver::Options::~Options() {}

IndexedDBObserver::IndexedDBObserver(int32_t observer_id,
                                     std::set<int64_t> object_store_ids,
                                     const Options& options)
    : id_(observer_id),
      object_store_ids_(object_store_ids),
      options_(options) {}

IndexedDBObserver::~IndexedDBObserver() {}

}  // namespace content
