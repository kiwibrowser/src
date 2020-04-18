// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/indexeddb/idb_metadata.h"

#include "third_party/blink/public/platform/modules/indexeddb/web_idb_metadata.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

constexpr int64_t IDBIndexMetadata::kInvalidId;

constexpr int64_t IDBObjectStoreMetadata::kInvalidId;

IDBIndexMetadata::IDBIndexMetadata() = default;

IDBIndexMetadata::IDBIndexMetadata(const String& name,
                                   int64_t id,
                                   const IDBKeyPath& key_path,
                                   bool unique,
                                   bool multi_entry)
    : name(name),
      id(id),
      key_path(key_path),
      unique(unique),
      multi_entry(multi_entry) {}

IDBObjectStoreMetadata::IDBObjectStoreMetadata() = default;

IDBObjectStoreMetadata::IDBObjectStoreMetadata(const String& name,
                                               int64_t id,
                                               const IDBKeyPath& key_path,
                                               bool auto_increment,
                                               int64_t max_index_id)
    : name(name),
      id(id),
      key_path(key_path),
      auto_increment(auto_increment),
      max_index_id(max_index_id) {}

scoped_refptr<IDBObjectStoreMetadata> IDBObjectStoreMetadata::CreateCopy()
    const {
  scoped_refptr<IDBObjectStoreMetadata> copy =
      base::AdoptRef(new IDBObjectStoreMetadata(name, id, key_path,
                                                auto_increment, max_index_id));

  for (const auto& it : indexes) {
    IDBIndexMetadata* index = it.value.get();
    scoped_refptr<IDBIndexMetadata> index_copy = base::AdoptRef(
        new IDBIndexMetadata(index->name, index->id, index->key_path,
                             index->unique, index->multi_entry));
    copy->indexes.insert(it.key, std::move(index_copy));
  }
  return copy;
}

IDBDatabaseMetadata::IDBDatabaseMetadata()
    : version(IDBDatabaseMetadata::kNoVersion) {}

IDBDatabaseMetadata::IDBDatabaseMetadata(const String& name,
                                         int64_t id,
                                         int64_t version,
                                         int64_t max_object_store_id)
    : name(name),
      id(id),
      version(version),
      max_object_store_id(max_object_store_id) {}

IDBDatabaseMetadata::IDBDatabaseMetadata(const WebIDBMetadata& web_metadata)
    : name(web_metadata.name),
      id(web_metadata.id),
      version(web_metadata.version),
      max_object_store_id(web_metadata.max_object_store_id) {
  for (size_t i = 0; i < web_metadata.object_stores.size(); ++i) {
    const WebIDBMetadata::ObjectStore& web_object_store =
        web_metadata.object_stores[i];
    scoped_refptr<IDBObjectStoreMetadata> object_store =
        base::AdoptRef(new IDBObjectStoreMetadata(
            web_object_store.name, web_object_store.id,
            IDBKeyPath(web_object_store.key_path),
            web_object_store.auto_increment, web_object_store.max_index_id));

    for (size_t j = 0; j < web_object_store.indexes.size(); ++j) {
      const WebIDBMetadata::Index& web_index = web_object_store.indexes[j];
      scoped_refptr<IDBIndexMetadata> index =
          base::AdoptRef(new IDBIndexMetadata(
              web_index.name, web_index.id, IDBKeyPath(web_index.key_path),
              web_index.unique, web_index.multi_entry));
      object_store->indexes.Set(web_index.id, std::move(index));
    }
    object_stores.Set(web_object_store.id, std::move(object_store));
  }
}

void IDBDatabaseMetadata::CopyFrom(const IDBDatabaseMetadata& metadata) {
  name = metadata.name;
  id = metadata.id;
  version = metadata.version;
  max_object_store_id = metadata.max_object_store_id;
}

STATIC_ASSERT_ENUM(WebIDBMetadata::kNoVersion, IDBDatabaseMetadata::kNoVersion);

}  // namespace blink
