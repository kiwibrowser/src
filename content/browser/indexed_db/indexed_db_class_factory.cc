// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_class_factory.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/browser/indexed_db/indexed_db_factory.h"
#include "content/browser/indexed_db/indexed_db_metadata_coding.h"
#include "content/browser/indexed_db/indexed_db_transaction.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator_impl.h"
#include "content/browser/indexed_db/leveldb/leveldb_transaction.h"

namespace content {

static IndexedDBClassFactory::GetterCallback* s_factory_getter;
static ::base::LazyInstance<IndexedDBClassFactory>::Leaky s_factory =
    LAZY_INSTANCE_INITIALIZER;

void IndexedDBClassFactory::SetIndexedDBClassFactoryGetter(GetterCallback* cb) {
  s_factory_getter = cb;
}

IndexedDBClassFactory* IndexedDBClassFactory::Get() {
  if (s_factory_getter)
    return (*s_factory_getter)();
  else
    return s_factory.Pointer();
}

scoped_refptr<IndexedDBDatabase> IndexedDBClassFactory::CreateIndexedDBDatabase(
    const base::string16& name,
    scoped_refptr<IndexedDBBackingStore> backing_store,
    scoped_refptr<IndexedDBFactory> factory,
    std::unique_ptr<IndexedDBMetadataCoding> metadata_coding,
    const IndexedDBDatabase::Identifier& unique_identifier) {
  return new IndexedDBDatabase(name, backing_store, factory,
                               std::move(metadata_coding), unique_identifier);
}

std::unique_ptr<IndexedDBTransaction>
IndexedDBClassFactory::CreateIndexedDBTransaction(
    int64_t id,
    IndexedDBConnection* connection,
    const std::set<int64_t>& scope,
    blink::WebIDBTransactionMode mode,
    IndexedDBBackingStore::Transaction* backing_store_transaction) {
  return std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
      id, connection, scope, mode, backing_store_transaction));
}

scoped_refptr<LevelDBTransaction>
IndexedDBClassFactory::CreateLevelDBTransaction(LevelDBDatabase* db) {
  return new LevelDBTransaction(db);
}

std::unique_ptr<LevelDBIteratorImpl> IndexedDBClassFactory::CreateIteratorImpl(
    std::unique_ptr<leveldb::Iterator> iterator,
    LevelDBDatabase* db,
    const leveldb::Snapshot* snapshot) {
  return base::WrapUnique(
      new LevelDBIteratorImpl(std::move(iterator), db, snapshot));
}

}  // namespace content
