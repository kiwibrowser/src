// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/mock_indexed_db_callbacks.h"

#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

namespace content {

MockIndexedDBCallbacks::MockIndexedDBCallbacks()
    : IndexedDBCallbacks(nullptr, url::Origin(), nullptr, nullptr) {}
MockIndexedDBCallbacks::MockIndexedDBCallbacks(bool expect_connection)
    : IndexedDBCallbacks(nullptr, url::Origin(), nullptr, nullptr),
      expect_connection_(expect_connection) {}

MockIndexedDBCallbacks::~MockIndexedDBCallbacks() {
  EXPECT_EQ(expect_connection_, !!connection_);
}

void MockIndexedDBCallbacks::OnError(const IndexedDBDatabaseError& error) {
  error_called_ = true;
}

void MockIndexedDBCallbacks::OnSuccess() {}

void MockIndexedDBCallbacks::OnSuccess(int64_t result) {}

void MockIndexedDBCallbacks::OnSuccess(const std::vector<base::string16>&) {}

void MockIndexedDBCallbacks::OnSuccess(const IndexedDBKey& key) {}

void MockIndexedDBCallbacks::OnSuccess(
    std::unique_ptr<IndexedDBConnection> connection,
    const IndexedDBDatabaseMetadata& metadata) {
  connection_ = std::move(connection);
}

void MockIndexedDBCallbacks::OnUpgradeNeeded(
    int64_t old_version,
    std::unique_ptr<IndexedDBConnection> connection,
    const content::IndexedDBDatabaseMetadata& metadata,
    const IndexedDBDataLossInfo& data_loss_info) {
  connection_ = std::move(connection);
  upgrade_called_ = true;
}

}  // namespace content
