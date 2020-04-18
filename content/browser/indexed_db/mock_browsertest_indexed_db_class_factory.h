// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_MOCK_BROWSERTEST_INDEXED_DB_CLASS_FACTORY_H_
#define CONTENT_BROWSER_INDEXED_DB_MOCK_BROWSERTEST_INDEXED_DB_CLASS_FACTORY_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>

#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_class_factory.h"
#include "content/browser/indexed_db/indexed_db_database.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace content {

class IndexedDBConnection;
class IndexedDBMetadataCoding;
class LevelDBTransaction;
class LevelDBDatabase;

enum FailClass {
  FAIL_CLASS_NOTHING,
  FAIL_CLASS_LEVELDB_ITERATOR,
  FAIL_CLASS_LEVELDB_TRANSACTION,
};

enum FailMethod {
  FAIL_METHOD_NOTHING,
  FAIL_METHOD_COMMIT,
  FAIL_METHOD_COMMIT_DISK_FULL,
  FAIL_METHOD_GET,
  FAIL_METHOD_SEEK,
};

class MockBrowserTestIndexedDBClassFactory : public IndexedDBClassFactory {
 public:
  MockBrowserTestIndexedDBClassFactory();
  ~MockBrowserTestIndexedDBClassFactory() override;

  scoped_refptr<IndexedDBDatabase> CreateIndexedDBDatabase(
      const base::string16& name,
      scoped_refptr<IndexedDBBackingStore> backing_store,
      scoped_refptr<IndexedDBFactory> factory,
      std::unique_ptr<IndexedDBMetadataCoding> metadata_coding,
      const IndexedDBDatabase::Identifier& unique_identifier) override;
  std::unique_ptr<IndexedDBTransaction> CreateIndexedDBTransaction(
      int64_t id,
      IndexedDBConnection* connection,
      const std::set<int64_t>& scope,
      blink::WebIDBTransactionMode mode,
      IndexedDBBackingStore::Transaction* backing_store_transaction) override;
  scoped_refptr<LevelDBTransaction> CreateLevelDBTransaction(
      LevelDBDatabase* db) override;
  std::unique_ptr<LevelDBIteratorImpl> CreateIteratorImpl(
      std::unique_ptr<leveldb::Iterator> iterator,
      LevelDBDatabase* db,
      const leveldb::Snapshot* snapshot) override;

  void FailOperation(FailClass failure_class,
                     FailMethod failure_method,
                     int fail_on_instance_num,
                     int fail_on_call_num);
  void Reset();

 private:
  FailClass failure_class_;
  FailMethod failure_method_;
  std::map<FailClass, int> instance_count_;
  std::map<FailClass, int> fail_on_instance_num_;
  std::map<FailClass, int> fail_on_call_num_;
  bool only_trace_calls_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_MOCK_BROWSERTEST_INDEXED_DB_CLASS_FACTORY_H_
