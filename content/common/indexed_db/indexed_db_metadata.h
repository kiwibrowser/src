// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_METADATA_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_METADATA_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/strings/string16.h"
#include "content/common/indexed_db/indexed_db_key_path.h"

namespace content {

struct CONTENT_EXPORT IndexedDBIndexMetadata {
  static const int64_t kInvalidId = -1;

  IndexedDBIndexMetadata();
  IndexedDBIndexMetadata(const base::string16& name,
                         int64_t id,
                         const IndexedDBKeyPath& key_path,
                         bool unique,
                         bool multi_entry);
  IndexedDBIndexMetadata(const IndexedDBIndexMetadata& other);
  IndexedDBIndexMetadata(IndexedDBIndexMetadata&& other);
  ~IndexedDBIndexMetadata();
  IndexedDBIndexMetadata& operator=(const IndexedDBIndexMetadata& other);
  IndexedDBIndexMetadata& operator=(IndexedDBIndexMetadata&& other);
  bool operator==(const IndexedDBIndexMetadata& other) const;

  base::string16 name;
  int64_t id;
  IndexedDBKeyPath key_path;
  bool unique;
  bool multi_entry;
};

struct CONTENT_EXPORT IndexedDBObjectStoreMetadata {
  static const int64_t kInvalidId = -1;
  static const int64_t kMinimumIndexId = 30;

  IndexedDBObjectStoreMetadata();
  IndexedDBObjectStoreMetadata(const base::string16& name,
                               int64_t id,
                               const IndexedDBKeyPath& key_path,
                               bool auto_increment,
                               int64_t max_index_id);
  IndexedDBObjectStoreMetadata(const IndexedDBObjectStoreMetadata& other);
  IndexedDBObjectStoreMetadata(IndexedDBObjectStoreMetadata&& other);
  ~IndexedDBObjectStoreMetadata();
  IndexedDBObjectStoreMetadata& operator=(
      const IndexedDBObjectStoreMetadata& other);
  IndexedDBObjectStoreMetadata& operator=(IndexedDBObjectStoreMetadata&& other);
  bool operator==(const IndexedDBObjectStoreMetadata& other) const;

  base::string16 name;
  int64_t id;
  IndexedDBKeyPath key_path;
  bool auto_increment;
  int64_t max_index_id;

  std::map<int64_t, IndexedDBIndexMetadata> indexes;
};

struct CONTENT_EXPORT IndexedDBDatabaseMetadata {
  // TODO(jsbell): These can probably be collapsed into 0.
  enum { NO_VERSION = -1, DEFAULT_VERSION = 0 };

  IndexedDBDatabaseMetadata();
  IndexedDBDatabaseMetadata(const base::string16& name,
                            int64_t id,
                            int64_t version,
                            int64_t max_object_store_id);
  IndexedDBDatabaseMetadata(const IndexedDBDatabaseMetadata& other);
  IndexedDBDatabaseMetadata(IndexedDBDatabaseMetadata&& other);
  ~IndexedDBDatabaseMetadata();
  IndexedDBDatabaseMetadata& operator=(const IndexedDBDatabaseMetadata& other);
  IndexedDBDatabaseMetadata& operator=(IndexedDBDatabaseMetadata&& other);
  bool operator==(const IndexedDBDatabaseMetadata& other) const;

  base::string16 name;
  int64_t id;
  int64_t version;
  int64_t max_object_store_id;

  std::map<int64_t, IndexedDBObjectStoreMetadata> object_stores;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_METADATA_H_
