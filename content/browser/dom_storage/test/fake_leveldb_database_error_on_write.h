// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_TEST_FAKE_LEVELDB_DATABASE_ERROR_ON_WRITE_H_
#define CONTENT_BROWSER_DOM_STORAGE_TEST_FAKE_LEVELDB_DATABASE_ERROR_ON_WRITE_H_

#include <stdint.h>
#include <map>
#include <vector>

#include "content/test/fake_leveldb_database.h"

namespace content {
namespace test {

// Reports an error on every |Write| call.
class FakeLevelDBDatabaseErrorOnWrite : public FakeLevelDBDatabase {
 public:
  explicit FakeLevelDBDatabaseErrorOnWrite(
      std::map<std::vector<uint8_t>, std::vector<uint8_t>>* mock_data);
  ~FakeLevelDBDatabaseErrorOnWrite() override;

  void Write(std::vector<leveldb::mojom::BatchedOperationPtr> operations,
             WriteCallback callback) override;
};

}  // namespace test
}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_TEST_FAKE_LEVELDB_DATABASE_ERROR_ON_WRITE_H_
