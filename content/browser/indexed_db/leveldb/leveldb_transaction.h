// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_TRANSACTION_H_
#define CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_TRANSACTION_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
#include "content/browser/indexed_db/leveldb/leveldb_comparator.h"
#include "content/browser/indexed_db/leveldb/leveldb_database.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator.h"

namespace content {
class LevelDBWriteBatch;

class CONTENT_EXPORT LevelDBTransaction
    : public base::RefCounted<LevelDBTransaction> {
 public:
  void Put(const base::StringPiece& key, std::string* value);

  void Remove(const base::StringPiece& key);

  leveldb::Status RemoveRange(const base::StringPiece& begin,
                              const base::StringPiece& end,
                              bool upper_open);

  virtual leveldb::Status Get(const base::StringPiece& key,
                              std::string* value,
                              bool* found);
  virtual leveldb::Status Commit();
  void Rollback();

  std::unique_ptr<LevelDBIterator> CreateIterator();

  uint64_t GetTransactionSize() const { return size_; }

 protected:
  virtual ~LevelDBTransaction();
  explicit LevelDBTransaction(LevelDBDatabase* db);
  friend class IndexedDBClassFactory;

 private:
  friend class base::RefCounted<LevelDBTransaction>;
  friend class LevelDBTransactionRangeTest;
  friend class LevelDBTransactionTest;
  FRIEND_TEST_ALL_PREFIXES(LevelDBTransactionTest, GetPutDelete);
  FRIEND_TEST_ALL_PREFIXES(LevelDBTransactionTest, Commit);
  FRIEND_TEST_ALL_PREFIXES(LevelDBTransactionTest, Iterator);

  struct Record {
    Record();
    ~Record();
    std::string key;
    std::string value;
    bool deleted = false;
  };
  static constexpr uint64_t SizeOfRecordInMap(size_t key_size);

  class Comparator {
   public:
    explicit Comparator(const LevelDBComparator* comparator)
        : comparator_(comparator) {}
    bool operator()(const base::StringPiece& a,
                    const base::StringPiece& b) const {
      return comparator_->Compare(a, b) < 0;
    }

   private:
    const LevelDBComparator* comparator_;
  };

  typedef std::map<base::StringPiece, std::unique_ptr<Record>, Comparator>
      DataType;

  // A DataIterator walks the uncommitted data in a transaction. It wraps a
  // std::map::iterator and provides the LevelDBIterator API. It is only used
  // internally as part of the implementation of TransactionIterator.
  class DataIterator : public LevelDBIterator {
   public:
    static std::unique_ptr<DataIterator> Create(
        LevelDBTransaction* transaction);
    ~DataIterator() override;
    bool IsValid() const override;
    leveldb::Status SeekToLast() override;
    leveldb::Status Seek(const base::StringPiece& slice) override;
    leveldb::Status Next() override;
    leveldb::Status Prev() override;
    base::StringPiece Key() const override;
    base::StringPiece Value() const override;
    bool IsDeleted() const;

    // Mark the current record as deleted.
    void Delete();

   private:
    explicit DataIterator(LevelDBTransaction* transaction);
    DataType* data_;
    DataType::iterator iterator_;

    DISALLOW_COPY_AND_ASSIGN(DataIterator);
  };

  // A TransactionIterator wraps a pair of a DataIterator (which walks the
  // uncommitted data) and LevelDBIterator wrapping a leveldb::Iterator (which
  // walks the data previously committed to the backing store). This provides
  // a unified view on committed and uncommitted data.
  class TransactionIterator : public LevelDBIterator {
   public:
    ~TransactionIterator() override;
    static std::unique_ptr<TransactionIterator> Create(
        scoped_refptr<LevelDBTransaction> transaction);

    bool IsValid() const override;
    leveldb::Status SeekToLast() override;
    leveldb::Status Seek(const base::StringPiece& target) override;
    leveldb::Status Next() override;
    leveldb::Status Prev() override;
    base::StringPiece Key() const override;
    base::StringPiece Value() const override;
    // Exposed for testing.
    bool IsDetached() const override;
    void DataChanged();

    // Mark the current record as deleted. If an existing record
    // is present in the uncommitted data this will convert it to
    // a deletion record, otherwise it will insert a new one.
    void Delete();

   private:
    enum Direction { FORWARD, REVERSE };

    explicit TransactionIterator(scoped_refptr<LevelDBTransaction> transaction);
    void HandleConflictsAndDeletes();
    void SetCurrentIteratorToSmallestKey();
    void SetCurrentIteratorToLargestKey();
    void RefreshDataIterator() const;
    bool DataIteratorIsLower() const;
    bool DataIteratorIsHigher() const;

    scoped_refptr<LevelDBTransaction> transaction_;
    const LevelDBComparator* comparator_;
    mutable std::unique_ptr<DataIterator> data_iterator_;
    std::unique_ptr<LevelDBIterator> db_iterator_;
    LevelDBIterator* current_ = nullptr;

    Direction direction_ = FORWARD;
    mutable bool data_changed_ = false;

    DISALLOW_COPY_AND_ASSIGN(TransactionIterator);
  };

  void Set(const base::StringPiece& key, std::string* value, bool deleted);
  void RegisterIterator(TransactionIterator* iterator);
  void UnregisterIterator(TransactionIterator* iterator);
  void NotifyIterators();

  LevelDBDatabase* db_;
  const LevelDBSnapshot snapshot_;
  const LevelDBComparator* comparator_;
  Comparator data_comparator_;
  DataType data_;
  uint64_t size_ = 0ull;
  bool finished_ = false;
  std::set<TransactionIterator*> iterators_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBTransaction);
};

// Reads go straight to the database, ignoring any writes cached in
// write_batch_, and writes are write-through, without consolidation.
class LevelDBDirectTransaction {
 public:
  static std::unique_ptr<LevelDBDirectTransaction> Create(LevelDBDatabase* db);

  ~LevelDBDirectTransaction();
  void Put(const base::StringPiece& key, const std::string* value);
  leveldb::Status Get(const base::StringPiece& key,
                      std::string* value,
                      bool* found);
  void Remove(const base::StringPiece& key);
  leveldb::Status Commit();

 private:
  explicit LevelDBDirectTransaction(LevelDBDatabase* db);

  LevelDBDatabase* db_;
  std::unique_ptr<LevelDBWriteBatch> write_batch_;
  bool finished_ = false;

  DISALLOW_COPY_AND_ASSIGN(LevelDBDirectTransaction);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_LEVELDB_LEVELDB_TRANSACTION_H_
