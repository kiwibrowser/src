// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_DATABASE_H_
#define CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_DATABASE_H_

#include <memory>
#include <string>

#include "base/containers/mru_cache.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/time/clock.h"
#include "base/trace_event/memory_dump_provider.h"
#include "content/common/content_export.h"
#include "third_party/leveldatabase/src/include/leveldb/comparator.h"
#include "third_party/leveldatabase/src/include/leveldb/options.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace leveldb {
class Comparator;
class DB;
class FilterPolicy;
class Iterator;
class Env;
class Snapshot;
}

namespace content {

class LevelDBComparator;
class LevelDBDatabase;
class LevelDBIterator;
class LevelDBWriteBatch;

class LevelDBSnapshot {
 private:
  friend class LevelDBDatabase;
  friend class LevelDBTransaction;

  explicit LevelDBSnapshot(LevelDBDatabase* db);
  ~LevelDBSnapshot();

  leveldb::DB* db_;
  const leveldb::Snapshot* snapshot_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBSnapshot);
};

class CONTENT_EXPORT LevelDBLock {
 public:
  LevelDBLock() {}
  virtual ~LevelDBLock() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(LevelDBLock);
};

class CONTENT_EXPORT LevelDBDatabase
    : public base::trace_event::MemoryDumpProvider {
 public:
  // Necessary because every iterator hangs onto leveldb blocks which can be
  // large. See https://crbug/696055.
  static const size_t kDefaultMaxOpenIteratorsPerDatabase = 50;

  // |max_open_cursors| cannot be 0.
  static leveldb::Status Open(const base::FilePath& file_name,
                              const LevelDBComparator* comparator,
                              size_t max_open_cursors,
                              std::unique_ptr<LevelDBDatabase>* db,
                              bool* is_disk_full = 0);

  static std::unique_ptr<LevelDBDatabase> OpenInMemory(
      const LevelDBComparator* comparator);
  static leveldb::Status Destroy(const base::FilePath& file_name);
  ~LevelDBDatabase() override;

  leveldb::Status Put(const base::StringPiece& key, std::string* value);
  leveldb::Status Remove(const base::StringPiece& key);
  virtual leveldb::Status Get(const base::StringPiece& key,
                              std::string* value,
                              bool* found,
                              const LevelDBSnapshot* = 0);
  leveldb::Status Write(const LevelDBWriteBatch& write_batch);
  // Note: Use DefaultReadOptions() and then adjust any values afterwards.
  std::unique_ptr<LevelDBIterator> CreateIterator(
      const leveldb::ReadOptions& options);
  const LevelDBComparator* Comparator() const;
  void Compact(const base::StringPiece& start, const base::StringPiece& stop);
  void CompactAll();

  leveldb::ReadOptions DefaultReadOptions();
  leveldb::ReadOptions DefaultReadOptions(const LevelDBSnapshot* snapshot);

  // base::trace_event::MemoryDumpProvider implementation.
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override;

  leveldb::DB* db() { return db_.get(); }
  leveldb::Env* env() { return env_.get(); }
  base::Time LastModified() const { return last_modified_; }

  void SetClockForTesting(std::unique_ptr<base::Clock> clock);

 protected:
  explicit LevelDBDatabase(size_t max_open_iterators);

 private:
  friend class LevelDBSnapshot;
  friend class LevelDBIteratorImpl;
  FRIEND_TEST_ALL_PREFIXES(IndexedDBTest, DeleteFailsIfDirectoryLocked);

  static std::unique_ptr<LevelDBLock> LockForTesting(
      const base::FilePath& file_name);

  // Methods for iterator pooling.
  std::unique_ptr<leveldb::Iterator> CreateLevelDBIterator(
      const leveldb::Snapshot*);
  void OnIteratorUsed(LevelDBIterator*);
  void OnIteratorDestroyed(LevelDBIterator*);

  void CloseDatabase();

  std::unique_ptr<leveldb::Env> env_;
  std::unique_ptr<leveldb::Comparator> comparator_adapter_;
  std::unique_ptr<leveldb::DB> db_;
  std::unique_ptr<const leveldb::FilterPolicy> filter_policy_;
  const LevelDBComparator* comparator_;
  base::Time last_modified_;
  std::unique_ptr<base::Clock> clock_;

  struct DetachIteratorOnDestruct {
    DetachIteratorOnDestruct() {}
    explicit DetachIteratorOnDestruct(LevelDBIterator* it) : it_(it) {}
    DetachIteratorOnDestruct(DetachIteratorOnDestruct&& that) {
      it_ = that.it_;
      that.it_ = nullptr;
    }
    ~DetachIteratorOnDestruct();

   private:
    LevelDBIterator* it_ = nullptr;

    DISALLOW_COPY_AND_ASSIGN(DetachIteratorOnDestruct);
  };

  // Despite the type name, this object uses LRU eviction.
  base::HashingMRUCache<LevelDBIterator*, DetachIteratorOnDestruct>
      iterator_lru_;

  // Recorded for UMA reporting.
  uint32_t num_iterators_ = 0;
  uint32_t max_iterators_ = 0;

  std::string file_name_for_tracing;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_DATABASE_H_
