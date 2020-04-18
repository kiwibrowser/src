// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CLASS_FACTORY_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CLASS_FACTORY_H_

#include <stdint.h>

#include <memory>
#include <set>

#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_database.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace leveldb {
class Iterator;
class Snapshot;
}  // namespace leveldb

namespace content {

class IndexedDBBackingStore;
class IndexedDBConnection;
class IndexedDBFactory;
class IndexedDBTransaction;
class LevelDBDatabase;
class LevelDBIteratorImpl;
class LevelDBTransaction;

// Use this factory to create some IndexedDB objects. Exists solely to
// facilitate tests which sometimes need to inject mock objects into the system.
class CONTENT_EXPORT IndexedDBClassFactory {
 public:
  typedef IndexedDBClassFactory* GetterCallback();

  static IndexedDBClassFactory* Get();

  static void SetIndexedDBClassFactoryGetter(GetterCallback* cb);

  virtual scoped_refptr<IndexedDBDatabase> CreateIndexedDBDatabase(
      const base::string16& name,
      scoped_refptr<IndexedDBBackingStore> backing_store,
      scoped_refptr<IndexedDBFactory> factory,
      std::unique_ptr<IndexedDBMetadataCoding> metadata_coding,
      const IndexedDBDatabase::Identifier& unique_identifier);

  virtual std::unique_ptr<IndexedDBTransaction> CreateIndexedDBTransaction(
      int64_t id,
      IndexedDBConnection* connection,
      const std::set<int64_t>& scope,
      blink::WebIDBTransactionMode mode,
      IndexedDBBackingStore::Transaction* backing_store_transaction);

  virtual std::unique_ptr<LevelDBIteratorImpl> CreateIteratorImpl(
      std::unique_ptr<leveldb::Iterator> iterator,
      LevelDBDatabase* db,
      const leveldb::Snapshot* snapshot);

  virtual scoped_refptr<LevelDBTransaction> CreateLevelDBTransaction(
      LevelDBDatabase* db);

 protected:
  IndexedDBClassFactory() {}
  virtual ~IndexedDBClassFactory() {}
  friend struct base::LazyInstanceTraitsBase<IndexedDBClassFactory>;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CLASS_FACTORY_H_
