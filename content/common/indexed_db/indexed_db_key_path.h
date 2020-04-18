// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_PATH_H_
#define CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_PATH_H_

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace content {

class CONTENT_EXPORT IndexedDBKeyPath {
 public:
  IndexedDBKeyPath();  // Defaults to blink::WebIDBKeyPathTypeNull.
  explicit IndexedDBKeyPath(const base::string16&);
  explicit IndexedDBKeyPath(const std::vector<base::string16>&);
  IndexedDBKeyPath(const IndexedDBKeyPath& other);
  IndexedDBKeyPath(IndexedDBKeyPath&& other);
  ~IndexedDBKeyPath();
  IndexedDBKeyPath& operator=(const IndexedDBKeyPath& other);
  IndexedDBKeyPath& operator=(IndexedDBKeyPath&& other);

  bool IsNull() const { return type_ == blink::kWebIDBKeyPathTypeNull; }
  bool operator==(const IndexedDBKeyPath& other) const;

  blink::WebIDBKeyPathType type() const { return type_; }
  const std::vector<base::string16>& array() const;
  const base::string16& string() const;

 private:
  blink::WebIDBKeyPathType type_;
  base::string16 string_;
  std::vector<base::string16> array_;
};

}  // namespace content

#endif  // CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_PATH_H_
