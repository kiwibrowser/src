// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_database_error.h"

namespace content {

IndexedDBDatabaseError::IndexedDBDatabaseError(uint16_t code) : code_(code) {}

IndexedDBDatabaseError::IndexedDBDatabaseError() = default;

IndexedDBDatabaseError::IndexedDBDatabaseError(uint16_t code,
                                               const char* message)
    : code_(code), message_(base::ASCIIToUTF16(message)) {}

IndexedDBDatabaseError::IndexedDBDatabaseError(uint16_t code,
                                               const base::string16& message)
    : code_(code), message_(message) {}

IndexedDBDatabaseError::~IndexedDBDatabaseError() = default;

IndexedDBDatabaseError& IndexedDBDatabaseError::operator=(
    const IndexedDBDatabaseError& rhs) = default;

}  // namespace content
