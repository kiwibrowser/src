// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_piece.h"
#include "content/browser/indexed_db/leveldb/leveldb_comparator.h"
#include "content/browser/indexed_db/leveldb/leveldb_database.h"
#include "content/browser/indexed_db/leveldb/leveldb_env.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator.h"
#include "content/browser/indexed_db/leveldb/leveldb_transaction.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/env_chromium.h"

namespace content {

namespace {
static const size_t kTestingMaxOpenCursors = 3;

class SimpleComparator : public LevelDBComparator {
 public:
  int Compare(const base::StringPiece& a,
              const base::StringPiece& b) const override {
    size_t len = std::min(a.size(), b.size());
    return memcmp(a.begin(), b.begin(), len);
  }
  const char* Name() const override { return "temp_comparator"; }
};

}  // namespace

class LevelDBTransactionTest : public testing::Test {
 public:
  LevelDBTransactionTest() {}
  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    leveldb::Status s =
        LevelDBDatabase::Open(temp_directory_.GetPath(), &comparator_,
                              kTestingMaxOpenCursors, &leveldb_);
    ASSERT_TRUE(s.ok());
    ASSERT_TRUE(leveldb_);
  }
  void TearDown() override {}

 protected:
  // Convenience methods to access the database outside any
  // transaction to cut down on boilerplate around calls.
  void Put(const base::StringPiece& key, const std::string& value) {
    std::string put_value = value;
    leveldb::Status s = leveldb_->Put(key, &put_value);
    ASSERT_TRUE(s.ok());
  }

  void Remove(const base::StringPiece& key) {
    leveldb::Status s = leveldb_->Remove(key);
    ASSERT_TRUE(s.ok());
  }

  void Get(const base::StringPiece& key, std::string* value, bool* found) {
    leveldb::Status s = leveldb_->Get(key, value, found);
    ASSERT_TRUE(s.ok());
  }

  bool Has(const base::StringPiece& key) {
    bool found;
    std::string value;
    leveldb::Status s = leveldb_->Get(key, &value, &found);
    EXPECT_TRUE(s.ok());
    return found;
  }

  // Convenience wrappers for LevelDBTransaction operations to
  // avoid boilerplate in tests.
  bool TransactionHas(LevelDBTransaction* transaction,
                      const base::StringPiece& key) {
    std::string value;
    bool found;
    leveldb::Status s = transaction->Get(key, &value, &found);
    EXPECT_TRUE(s.ok());
    return found;
  }

  void TransactionPut(LevelDBTransaction* transaction,
                      const base::StringPiece& key,
                      const std::string& value) {
    std::string put_value = value;
    transaction->Put(key, &put_value);
  }

  int Compare(const base::StringPiece& a, const base::StringPiece& b) const {
    return comparator_.Compare(a, b);
  }

  LevelDBDatabase* db() { return leveldb_.get(); }

  scoped_refptr<LevelDBTransaction> CreateTransaction() {
    return new LevelDBTransaction(db());
  }

  static constexpr size_t SizeOfRecord() {
    return sizeof(LevelDBTransaction::Record);
  }

 private:
  base::ScopedTempDir temp_directory_;
  SimpleComparator comparator_;
  std::unique_ptr<LevelDBDatabase> leveldb_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBTransactionTest);
};

TEST_F(LevelDBTransactionTest, GetPutDelete) {
  leveldb::Status status;

  const std::string key("key");
  std::string got_value;

  const std::string old_value("value");
  Put(key, old_value);

  scoped_refptr<LevelDBTransaction> transaction = new LevelDBTransaction(db());

  const std::string new_value("new value");
  Put(key, new_value);

  bool found = false;
  status = transaction->Get(key, &got_value, &found);
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(found);
  EXPECT_EQ(Compare(got_value, old_value), 0);

  Get(key, &got_value, &found);
  EXPECT_TRUE(found);
  EXPECT_EQ(Compare(got_value, new_value), 0);

  const std::string added_key("added key");
  const std::string added_value("added value");
  Put(added_key, added_value);

  Get(added_key, &got_value, &found);
  EXPECT_TRUE(found);
  EXPECT_EQ(Compare(got_value, added_value), 0);

  EXPECT_FALSE(TransactionHas(transaction.get(), added_key));

  const std::string another_key("another key");
  const std::string another_value("another value");
  EXPECT_EQ(0ull, transaction->GetTransactionSize());
  TransactionPut(transaction.get(), another_key, another_value);
  EXPECT_EQ(SizeOfRecord() + another_key.size() * 2 + another_value.size(),
            transaction->GetTransactionSize());

  status = transaction->Get(another_key, &got_value, &found);
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(found);
  EXPECT_EQ(Compare(got_value, another_value), 0);

  transaction->Remove(another_key);
  EXPECT_EQ(SizeOfRecord() + another_key.size() * 2,
            transaction->GetTransactionSize());
}

TEST_F(LevelDBTransactionTest, Iterator) {
  const std::string key1("key1");
  const std::string value1("value1");
  const std::string key2("key2");
  const std::string value2("value2");

  Put(key1, value1);
  Put(key2, value2);

  scoped_refptr<LevelDBTransaction> transaction = new LevelDBTransaction(db());

  Remove(key2);

  std::unique_ptr<LevelDBIterator> it = transaction->CreateIterator();

  it->Seek(std::string());

  EXPECT_TRUE(it->IsValid());
  EXPECT_EQ(Compare(it->Key(), key1), 0);
  EXPECT_EQ(Compare(it->Value(), value1), 0);

  it->Next();

  EXPECT_TRUE(it->IsValid());
  EXPECT_EQ(Compare(it->Key(), key2), 0);
  EXPECT_EQ(Compare(it->Value(), value2), 0);

  it->Next();

  EXPECT_FALSE(it->IsValid());
}

TEST_F(LevelDBTransactionTest, Commit) {
  const std::string key1("key1");
  const std::string key2("key2");
  const std::string value1("value1");
  const std::string value2("value2");
  const std::string value3("value3");

  std::string got_value;
  bool found;

  scoped_refptr<LevelDBTransaction> transaction = new LevelDBTransaction(db());

  TransactionPut(transaction.get(), key1, value1);
  TransactionPut(transaction.get(), key2, value2);
  TransactionPut(transaction.get(), key2, value3);

  leveldb::Status status = transaction->Commit();
  EXPECT_TRUE(status.ok());

  Get(key1, &got_value, &found);
  EXPECT_TRUE(found);
  EXPECT_EQ(value1, got_value);

  Get(key2, &got_value, &found);
  EXPECT_TRUE(found);
  EXPECT_EQ(value3, got_value);
}

TEST_F(LevelDBTransactionTest, IterationWithEvictedCursors) {
  leveldb::Status status;

  Put("key1", "value1");
  Put("key2", "value2");
  Put("key3", "value3");

  scoped_refptr<LevelDBTransaction> transaction = CreateTransaction();

  std::unique_ptr<LevelDBIterator> evicted_normal_location =
      transaction->CreateIterator();

  std::unique_ptr<LevelDBIterator> evicted_before_start =
      transaction->CreateIterator();

  std::unique_ptr<LevelDBIterator> evicted_after_end =
      transaction->CreateIterator();

  std::unique_ptr<LevelDBIterator> it1 = transaction->CreateIterator();
  std::unique_ptr<LevelDBIterator> it2 = transaction->CreateIterator();
  std::unique_ptr<LevelDBIterator> it3 = transaction->CreateIterator();

  evicted_normal_location->Seek("key1");
  evicted_before_start->Seek("key1");
  evicted_before_start->Prev();
  evicted_after_end->SeekToLast();
  evicted_after_end->Next();

  // Nothing is purged, as we just have 3 iterators used.
  EXPECT_FALSE(evicted_normal_location->IsDetached());
  EXPECT_FALSE(evicted_before_start->IsDetached());
  EXPECT_FALSE(evicted_after_end->IsDetached());

  // Should purge all of our earlier iterators.
  it1->Seek("key1");
  it2->Seek("key2");
  it3->Seek("key3");

  EXPECT_TRUE(evicted_normal_location->IsDetached());
  EXPECT_TRUE(evicted_before_start->IsDetached());
  EXPECT_TRUE(evicted_after_end->IsDetached());

  EXPECT_FALSE(evicted_before_start->IsValid());
  EXPECT_TRUE(evicted_normal_location->IsValid());
  EXPECT_FALSE(evicted_after_end->IsValid());

  // Check we don't need to reload for just the key.
  EXPECT_EQ("key1", evicted_normal_location->Key());
  EXPECT_TRUE(evicted_normal_location->IsDetached());

  // Make sure iterators are reloaded correctly.
  EXPECT_TRUE(evicted_normal_location->IsValid());
  EXPECT_EQ("value1", evicted_normal_location->Value());
  EXPECT_FALSE(evicted_normal_location->IsDetached());
  EXPECT_FALSE(evicted_before_start->IsValid());
  EXPECT_FALSE(evicted_after_end->IsValid());

  // And our |Value()| call purged the earlier iterator.
  EXPECT_TRUE(it1->IsDetached());
}

namespace {
enum RangePrepareMode {
  DataInMemory,
  DataInDatabase,
  DataMixed,
};
}  // namespace

class LevelDBTransactionRangeTest
    : public LevelDBTransactionTest,
      public testing::WithParamInterface<RangePrepareMode> {
 public:
  LevelDBTransactionRangeTest() {}
  void SetUp() override {
    LevelDBTransactionTest::SetUp();

    switch (GetParam()) {
      case DataInMemory:
        transaction_ = new LevelDBTransaction(db());

        TransactionPut(transaction_.get(), key_before_range_, value_);
        TransactionPut(transaction_.get(), range_start_, value_);
        TransactionPut(transaction_.get(), key_in_range1_, value_);
        TransactionPut(transaction_.get(), key_in_range2_, value_);
        TransactionPut(transaction_.get(), range_end_, value_);
        TransactionPut(transaction_.get(), key_after_range_, value_);
        break;

      case DataInDatabase:
        Put(key_before_range_, value_);
        Put(range_start_, value_);
        Put(key_in_range1_, value_);
        Put(key_in_range2_, value_);
        Put(range_end_, value_);
        Put(key_after_range_, value_);

        transaction_ = new LevelDBTransaction(db());
        break;

      case DataMixed:
        Put(key_before_range_, value_);
        Put(key_in_range1_, value_);
        Put(range_end_, value_);

        transaction_ = new LevelDBTransaction(db());

        TransactionPut(transaction_.get(), range_start_, value_);
        TransactionPut(transaction_.get(), key_in_range2_, value_);
        TransactionPut(transaction_.get(), key_after_range_, value_);
        break;
    }
  }

 protected:
  const std::string key_before_range_ = "a";
  const std::string range_start_ = "b";
  const std::string key_in_range1_ = "c";
  const std::string key_in_range2_ = "d";
  const std::string range_end_ = "e";
  const std::string key_after_range_ = "f";
  const std::string value_ = "value";

  scoped_refptr<LevelDBTransaction> transaction_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LevelDBTransactionRangeTest);
};

TEST_P(LevelDBTransactionRangeTest, RemoveRangeUpperClosed) {
  leveldb::Status status;

  const bool upper_open = false;
  status = transaction_->RemoveRange(range_start_, range_end_, upper_open);
  EXPECT_TRUE(status.ok());

  EXPECT_TRUE(TransactionHas(transaction_.get(), key_before_range_));
  EXPECT_FALSE(TransactionHas(transaction_.get(), range_start_));
  EXPECT_FALSE(TransactionHas(transaction_.get(), key_in_range1_));
  EXPECT_FALSE(TransactionHas(transaction_.get(), key_in_range2_));
  EXPECT_FALSE(TransactionHas(transaction_.get(), range_end_));
  EXPECT_TRUE(TransactionHas(transaction_.get(), key_after_range_));

  status = transaction_->Commit();
  EXPECT_TRUE(status.ok());

  EXPECT_TRUE(Has(key_before_range_));
  EXPECT_FALSE(Has(range_start_));
  EXPECT_FALSE(Has(key_in_range1_));
  EXPECT_FALSE(Has(key_in_range2_));
  EXPECT_FALSE(Has(range_end_));
  EXPECT_TRUE(Has(key_after_range_));
}

TEST_P(LevelDBTransactionRangeTest, RemoveRangeUpperOpen) {
  leveldb::Status status;

  const bool upper_open = true;
  status = transaction_->RemoveRange(range_start_, range_end_, upper_open);
  EXPECT_TRUE(status.ok());

  EXPECT_TRUE(TransactionHas(transaction_.get(), key_before_range_));
  EXPECT_FALSE(TransactionHas(transaction_.get(), range_start_));
  EXPECT_FALSE(TransactionHas(transaction_.get(), key_in_range1_));
  EXPECT_FALSE(TransactionHas(transaction_.get(), key_in_range2_));
  EXPECT_TRUE(TransactionHas(transaction_.get(), range_end_));
  EXPECT_TRUE(TransactionHas(transaction_.get(), key_after_range_));

  status = transaction_->Commit();
  EXPECT_TRUE(status.ok());

  EXPECT_TRUE(Has(key_before_range_));
  EXPECT_FALSE(Has(range_start_));
  EXPECT_FALSE(Has(key_in_range1_));
  EXPECT_FALSE(Has(key_in_range2_));
  EXPECT_TRUE(Has(range_end_));
  EXPECT_TRUE(Has(key_after_range_));
}

TEST_P(LevelDBTransactionRangeTest, RemoveRangeIteratorRetainsKey) {
  leveldb::Status status;

  std::unique_ptr<LevelDBIterator> it = transaction_->CreateIterator();
  status = it->Seek(key_in_range1_);
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(it->IsValid());
  EXPECT_EQ(Compare(key_in_range1_, it->Key()), 0);
  status = it->Next();
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(it->IsValid());
  EXPECT_EQ(Compare(key_in_range2_, it->Key()), 0);

  const bool upper_open = false;
  status = transaction_->RemoveRange(range_start_, range_end_, upper_open);
  EXPECT_TRUE(status.ok());

  EXPECT_TRUE(it->IsValid());
  EXPECT_EQ(Compare(key_in_range2_, it->Key()), 0);

  status = it->Next();
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(it->IsValid());
  EXPECT_EQ(Compare(key_after_range_, it->Key()), 0);
}

INSTANTIATE_TEST_CASE_P(LevelDBTransactionRangeTests,
                        LevelDBTransactionRangeTest,
                        ::testing::Values(DataInMemory,
                                          DataInDatabase,
                                          DataMixed));

}  // namespace content
