// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_MOCK_INDEXED_DB_CALLBACKS_H_
#define CONTENT_BROWSER_INDEXED_DB_MOCK_INDEXED_DB_CALLBACKS_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "content/browser/indexed_db/indexed_db_callbacks.h"
#include "content/browser/indexed_db/indexed_db_connection.h"

namespace content {

class MockIndexedDBCallbacks : public IndexedDBCallbacks {
 public:
  MockIndexedDBCallbacks();
  explicit MockIndexedDBCallbacks(bool expect_connection);

  void OnError(const IndexedDBDatabaseError& error) override;

  void OnSuccess() override;
  void OnSuccess(int64_t result) override;
  void OnSuccess(const std::vector<base::string16>& result) override;
  void OnSuccess(const IndexedDBKey& key) override;
  void OnSuccess(std::unique_ptr<IndexedDBConnection> connection,
                 const IndexedDBDatabaseMetadata& metadata) override;
  IndexedDBConnection* connection() { return connection_.get(); }

  void OnUpgradeNeeded(int64_t old_version,
                       std::unique_ptr<IndexedDBConnection> connection,
                       const content::IndexedDBDatabaseMetadata& metadata,
                       const IndexedDBDataLossInfo& data_loss_info) override;

  bool error_called() { return error_called_; }
  bool upgrade_called() { return upgrade_called_; }

 protected:
  ~MockIndexedDBCallbacks() override;

  std::unique_ptr<IndexedDBConnection> connection_;

 private:
  bool expect_connection_ = true;
  bool error_called_ = false;
  bool upgrade_called_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockIndexedDBCallbacks);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_MOCK_INDEXED_DB_CALLBACKS_H_
