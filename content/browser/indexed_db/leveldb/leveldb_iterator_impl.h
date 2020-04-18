// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_ITERATOR_IMPL_H_
#define CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_ITERATOR_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator.h"
#include "content/common/content_export.h"
#include "third_party/leveldatabase/src/include/leveldb/iterator.h"

namespace leveldb {
class Snapshot;
}

namespace content {
class LevelDBDatabase;

class CONTENT_EXPORT LevelDBIteratorImpl : public content::LevelDBIterator {
 public:
  ~LevelDBIteratorImpl() override;
  bool IsValid() const override;
  leveldb::Status SeekToLast() override;
  leveldb::Status Seek(const base::StringPiece& target) override;
  leveldb::Status Next() override;
  leveldb::Status Prev() override;
  base::StringPiece Key() const override;
  base::StringPiece Value() const override;
  void Detach() override;
  bool IsDetached() const override;

 protected:
  explicit LevelDBIteratorImpl(std::unique_ptr<leveldb::Iterator> iterator,
                               LevelDBDatabase* db,
                               const leveldb::Snapshot* snapshot);

 private:
  enum class IteratorState { ACTIVE, EVICTED_AND_VALID, EVICTED_AND_INVALID };

  leveldb::Status CheckStatus();

  // Notifies the database of iterator usage and recreates iterator if needed.
  void WillUseDBIterator();

  friend class IndexedDBClassFactory;
  friend class MockBrowserTestIndexedDBClassFactory;

  std::unique_ptr<leveldb::Iterator> iterator_;

  // State used to facilitate memory purging.
  LevelDBDatabase* db_;
  IteratorState iterator_state_ = IteratorState::ACTIVE;
  std::string key_before_eviction_;
  const leveldb::Snapshot* snapshot_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBIteratorImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_ITERATOR_IMPL_H_
