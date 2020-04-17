// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_tombstone_sweeper.h"

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/time/tick_clock.h"
#include "content/browser/indexed_db/leveldb/leveldb_comparator.h"
#include "content/browser/indexed_db/leveldb/leveldb_database.h"
#include "content/browser/indexed_db/leveldb/mock_level_db.h"
#include "content/common/indexed_db/indexed_db_metadata.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/filter_policy.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"

namespace content {
class BrowserContext;

namespace indexed_db_tombstone_sweeper_unittest {
using ::testing::_;
using ::testing::Eq;
using ::testing::Return;
using ::testing::StrictMock;
using Status = ::leveldb::Status;
using Slice = ::leveldb::Slice;
using Mode = IndexedDBTombstoneSweeper::Mode;

constexpr int kRoundIterations = 11;
constexpr int kMaxIterations = 100;
const base::TimeTicks kTaskStartTime =
    base::TimeTicks() + base::TimeDelta::FromSeconds(1);
const base::TimeTicks kTaskEndTime =
    base::TimeTicks() + base::TimeDelta::FromSeconds(2);

constexpr int64_t kDb1 = 1;
constexpr int64_t kDb2 = 1;
constexpr int64_t kOs1 = 3;
constexpr int64_t kOs2 = 5;
constexpr int64_t kOs3 = 8;
constexpr int64_t kOs4 = 9;
constexpr int64_t kIndex1 = 31;
constexpr int64_t kIndex2 = 32;
constexpr int64_t kIndex3 = 35;
constexpr int kTombstoneSize = 33;

MATCHER_P(SliceEq,
          str,
          std::string(negation ? "isn't" : "is") + " equal to " +
              base::HexEncode(str.data(), str.size())) {
  *result_listener << "which is " << base::HexEncode(arg.data(), arg.size());
  return std::string(arg.data(), arg.size()) == str;
};

class MockTickClock : public base::TickClock {
 public:
  MockTickClock() {}
  ~MockTickClock() override {}

  MOCK_CONST_METHOD0(NowTicks, base::TimeTicks());
};

class Comparator : public LevelDBComparator {
 public:
  int Compare(const base::StringPiece& a,
              const base::StringPiece& b) const override {
    return content::Compare(a, b, false /*index_keys*/);
  }
  const char* Name() const override { return "idb_cmp1"; }
};

class IndexedDBTombstoneSweeperTest : public testing::TestWithParam<Mode> {
 public:
  IndexedDBTombstoneSweeperTest() {}
  ~IndexedDBTombstoneSweeperTest() {}

  void PopulateMultiDBMetdata() {
    // db1
    //   os1
    //   os2
    //     index1
    //     index2
    metadata_.emplace_back(base::ASCIIToUTF16("db1"), kDb1, 1, 29);
    auto& db1 = metadata_.back();
    db1.object_stores[kOs1] = IndexedDBObjectStoreMetadata(
        base::ASCIIToUTF16("os1"), kOs1, IndexedDBKeyPath(), false, 1000);
    db1.object_stores[kOs2] = IndexedDBObjectStoreMetadata(
        base::ASCIIToUTF16("os2"), kOs2, IndexedDBKeyPath(), false, 1000);
    auto& os2 = db1.object_stores[kOs2];
    os2.indexes[kIndex1] = IndexedDBIndexMetadata(
        base::ASCIIToUTF16("index1"), kIndex1, IndexedDBKeyPath(), true, false);
    os2.indexes[kIndex2] = IndexedDBIndexMetadata(
        base::ASCIIToUTF16("index2"), kIndex2, IndexedDBKeyPath(), true, false);
    // db2
    //   os3
    //     index3
    //   os4
    metadata_.emplace_back(base::ASCIIToUTF16("db2"), kDb2, 1, 29);
    auto& db2 = metadata_.back();
    db2.object_stores[kOs3] = IndexedDBObjectStoreMetadata(
        base::ASCIIToUTF16("os3"), kOs3, IndexedDBKeyPath(), false, 1000);
    db2.object_stores[kOs4] = IndexedDBObjectStoreMetadata(
        base::ASCIIToUTF16("os4"), kOs4, IndexedDBKeyPath(), false, 1000);
    auto& os3 = db2.object_stores[kOs3];
    os3.indexes[kIndex3] = IndexedDBIndexMetadata(
        base::ASCIIToUTF16("index3"), kIndex3, IndexedDBKeyPath(), true, false);
  }

  void PopulateSingleIndexDBMetadata() {
    // db1
    //   os1
    //     index1
    metadata_.emplace_back(base::ASCIIToUTF16("db1"), kDb1, 1, 29);
    auto& db1 = metadata_.back();
    db1.object_stores[kOs1] = IndexedDBObjectStoreMetadata(
        base::ASCIIToUTF16("os1"), kOs1, IndexedDBKeyPath(), false, 1000);
    auto& os2 = db1.object_stores[kOs1];
    os2.indexes[kIndex1] = IndexedDBIndexMetadata(
        base::ASCIIToUTF16("index1"), kIndex1, IndexedDBKeyPath(), true, false);
  }

  void SetupMockDB() {
    sweeper_ = std::make_unique<IndexedDBTombstoneSweeper>(
        GetParam(), kRoundIterations, kMaxIterations, &mock_db_);
    sweeper_->SetStartSeedsForTesting(0, 0, 0);
  }

  void SetupRealDB() {
    comparator_.reset(new Comparator());
    in_memory_db_ = LevelDBDatabase::OpenInMemory(comparator_.get());
    sweeper_ = std::make_unique<IndexedDBTombstoneSweeper>(
        GetParam(), kRoundIterations, kMaxIterations, in_memory_db_->db());
    sweeper_->SetStartSeedsForTesting(0, 0, 0);
  }

  void SetClockExpectations() {
    EXPECT_CALL(tick_clock_, NowTicks())
        .WillOnce(testing::Return(kTaskStartTime))
        .WillOnce(testing::Return(kTaskEndTime));
    sweeper_->SetClockForTesting(&tick_clock_);
  }

  void SetFirstClockExpectation() {
    EXPECT_CALL(tick_clock_, NowTicks())
        .WillOnce(testing::Return(kTaskStartTime));
    sweeper_->SetClockForTesting(&tick_clock_);
  }

  void ExpectUmaTombstones(int num, int size, bool reached_max = false) {
    std::string category = reached_max ? "MaxIterations" : "Complete";
    if (GetParam() == Mode::STATISTICS) {
      histogram_tester_.ExpectUniqueSample(
          "WebCore.IndexedDB.TombstoneSweeper.NumTombstones." + category, num,
          1);
      histogram_tester_.ExpectUniqueSample(
          "WebCore.IndexedDB.TombstoneSweeper.TombstonesSize." + category, size,
          1);
    } else {
      histogram_tester_.ExpectUniqueSample(
          "WebCore.IndexedDB.TombstoneSweeper.NumDeletedTombstones." + category,
          num, 1);
      histogram_tester_.ExpectUniqueSample(
          "WebCore.IndexedDB.TombstoneSweeper.DeletedTombstonesSize." +
              category,
          size, 1);
    }
  }

  void ExpectTaskTimeRecorded() {
    if (GetParam() == Mode::STATISTICS) {
      histogram_tester_.ExpectTimeBucketCount(
          "WebCore.IndexedDB.TombstoneSweeper.StatsTotalTime.Complete",
          base::TimeDelta::FromSeconds(1), 1);
    } else {
      histogram_tester_.ExpectTimeBucketCount(
          "WebCore.IndexedDB.TombstoneSweeper.DeletionTotalTime.Complete",
          base::TimeDelta::FromSeconds(1), 1);
    }
  }

  void ExpectIndexEntry(leveldb::MockIterator& iterator,
                        int64_t db,
                        int64_t os,
                        int64_t index,
                        const IndexedDBKey& index_key,
                        const IndexedDBKey& primary_key,
                        int index_version) {
    testing::InSequence sequence_enforcer;

    EXPECT_CALL(iterator, key())
        .WillOnce(Return(
            IndexDataKey::Encode(db, os, index, index_key, primary_key)));
    std::string value_str;
    EncodeVarInt(index_version, &value_str);
    EncodeIDBKey(primary_key, &value_str);
    EXPECT_CALL(iterator, value()).WillOnce(Return(value_str));
  }

  void ExpectIndexAndExistsEntries(leveldb::MockIterator& iterator,
                                   int64_t db,
                                   int64_t os,
                                   int64_t index,
                                   const IndexedDBKey& index_key,
                                   const IndexedDBKey& primary_key,
                                   int index_version,
                                   int exists_version) {
    ExpectIndexEntry(iterator, db, os, index, index_key, primary_key,
                     index_version);

    testing::InSequence sequence_enforcer;

    std::string encoded_primary_key;
    EncodeIDBKey(primary_key, &encoded_primary_key);

    std::string exists_value;
    EncodeVarInt(exists_version, &exists_value);
    EXPECT_CALL(
        mock_db_,
        Get(_, SliceEq(ExistsEntryKey::Encode(db, os, encoded_primary_key)), _))
        .WillOnce(testing::DoAll(testing::SetArgPointee<2>(exists_value),
                                 Return(Status::OK())));
  }

 protected:
  std::unique_ptr<Comparator> comparator_;
  std::unique_ptr<LevelDBDatabase> in_memory_db_;
  leveldb::MockLevelDB mock_db_;

  std::unique_ptr<IndexedDBTombstoneSweeper> sweeper_;

  StrictMock<MockTickClock> tick_clock_;

  std::vector<IndexedDBDatabaseMetadata> metadata_;

  // Used to verify recorded data.
  base::HistogramTester histogram_tester_;

 private:
  TestBrowserThreadBundle thread_bundle_;
};

TEST_P(IndexedDBTombstoneSweeperTest, EmptyDB) {
  SetupMockDB();
  sweeper_->SetMetadata(&metadata_);
  EXPECT_TRUE(sweeper_->RunRound());

  EXPECT_TRUE(
      histogram_tester_.GetTotalCountsForPrefix("WebCore.IndexedDB.").empty());
}

TEST_P(IndexedDBTombstoneSweeperTest, NoTombstonesComplexDB) {
  SetupMockDB();
  PopulateMultiDBMetdata();
  sweeper_->SetMetadata(&metadata_);
  SetClockExpectations();

  // We'll have one index entry per index, and simulate reaching the end.
  leveldb::MockIterator* mock_iterator = new leveldb::MockIterator();
  EXPECT_CALL(mock_db_, NewIterator(testing::_))
      .WillOnce(testing::Return(mock_iterator));

  // First index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb1, kOs2, kIndex1))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));

    ExpectIndexAndExistsEntries(*mock_iterator, kDb1, kOs2, kIndex1,
                                IndexedDBKey(10, blink::kWebIDBKeyTypeNumber),
                                IndexedDBKey(20, blink::kWebIDBKeyTypeNumber),
                                1, 1);
    EXPECT_CALL(*mock_iterator, Next());

    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    // Return the beginning of the second index, which should cause us to error
    // & go restart our index seek.
    ExpectIndexEntry(*mock_iterator, kDb1, kOs2, kIndex2,
                     IndexedDBKey(30, blink::kWebIDBKeyTypeNumber),
                     IndexedDBKey(10, blink::kWebIDBKeyTypeNumber), 1);
  }

  // Second index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb1, kOs2, kIndex2))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    ExpectIndexAndExistsEntries(*mock_iterator, kDb1, kOs2, kIndex2,
                                IndexedDBKey(30, blink::kWebIDBKeyTypeNumber),
                                IndexedDBKey(10, blink::kWebIDBKeyTypeNumber),
                                1, 1);
    EXPECT_CALL(*mock_iterator, Next());

    // Return next key, which should make it error
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    ExpectIndexEntry(*mock_iterator, kDb2, kOs3, kIndex3,
                     IndexedDBKey(1501, blink::kWebIDBKeyTypeNumber),
                     IndexedDBKey(15123, blink::kWebIDBKeyTypeNumber), 12);
  }

  // Third index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb2, kOs3, kIndex3))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    ExpectIndexAndExistsEntries(
        *mock_iterator, kDb2, kOs3, kIndex3,
        IndexedDBKey(1501, blink::kWebIDBKeyTypeNumber),
        IndexedDBKey(15123, blink::kWebIDBKeyTypeNumber), 12, 12);
    EXPECT_CALL(*mock_iterator, Next());

    // Return next key, which should make it error
    EXPECT_CALL(*mock_iterator, Valid()).WillOnce(Return(false));
    EXPECT_CALL(*mock_iterator, status()).WillOnce(Return(Status::OK()));
    EXPECT_CALL(*mock_iterator, Valid()).WillOnce(Return(false));
  }

  ASSERT_TRUE(sweeper_->RunRound());
  ExpectTaskTimeRecorded();
  ExpectUmaTombstones(0, 0);
  histogram_tester_.ExpectUniqueSample(
      "WebCore.IndexedDB.TombstoneSweeper.IndexScanPercent", 20, 1);
}

TEST_P(IndexedDBTombstoneSweeperTest, AllTombstonesComplexDB) {
  SetupMockDB();
  PopulateMultiDBMetdata();
  sweeper_->SetMetadata(&metadata_);
  SetClockExpectations();

  // We'll have one index entry per index, and simulate reaching the end.
  leveldb::MockIterator* mock_iterator = new leveldb::MockIterator();
  EXPECT_CALL(mock_db_, NewIterator(testing::_))
      .WillOnce(testing::Return(mock_iterator));

  // First index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb1, kOs2, kIndex1))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));

    ExpectIndexAndExistsEntries(*mock_iterator, kDb1, kOs2, kIndex1,
                                IndexedDBKey(10, blink::kWebIDBKeyTypeNumber),
                                IndexedDBKey(20, blink::kWebIDBKeyTypeNumber),
                                1, 2);
    EXPECT_CALL(*mock_iterator, Next());

    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    // Return the beginning of the second index, which should cause us to error
    // & go restart our index seek.
    ExpectIndexEntry(*mock_iterator, kDb1, kOs2, kIndex2,
                     IndexedDBKey(30, blink::kWebIDBKeyTypeNumber),
                     IndexedDBKey(10, blink::kWebIDBKeyTypeNumber), 1);
  }

  // Second index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb1, kOs2, kIndex2))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    ExpectIndexAndExistsEntries(*mock_iterator, kDb1, kOs2, kIndex2,
                                IndexedDBKey(30, blink::kWebIDBKeyTypeNumber),
                                IndexedDBKey(10, blink::kWebIDBKeyTypeNumber),
                                1, 2);
    EXPECT_CALL(*mock_iterator, Next());

    // Return next key, which should make it error
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    ExpectIndexEntry(*mock_iterator, kDb2, kOs3, kIndex3,
                     IndexedDBKey(1501, blink::kWebIDBKeyTypeNumber),
                     IndexedDBKey(15123, blink::kWebIDBKeyTypeNumber), 12);
  }

  // Third index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb2, kOs3, kIndex3))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    ExpectIndexAndExistsEntries(
        *mock_iterator, kDb2, kOs3, kIndex3,
        IndexedDBKey(1501, blink::kWebIDBKeyTypeNumber),
        IndexedDBKey(15123, blink::kWebIDBKeyTypeNumber), 12, 13);
    EXPECT_CALL(*mock_iterator, Next());

    // Return next key, which should make it error
    EXPECT_CALL(*mock_iterator, Valid()).WillOnce(Return(false));
    EXPECT_CALL(*mock_iterator, status()).WillOnce(Return(Status::OK()));
    EXPECT_CALL(*mock_iterator, Valid()).WillOnce(Return(false));
  }

  if (GetParam() == Mode::DELETION)
    EXPECT_CALL(mock_db_, Write(_, _));

  ASSERT_TRUE(sweeper_->RunRound());
  ExpectTaskTimeRecorded();
  ExpectUmaTombstones(3, kTombstoneSize * 3);
  histogram_tester_.ExpectUniqueSample(
      "WebCore.IndexedDB.TombstoneSweeper.IndexScanPercent", 20, 1);
}

TEST_P(IndexedDBTombstoneSweeperTest, SimpleRealDBNoTombstones) {
  PopulateSingleIndexDBMetadata();
  SetupRealDB();
  sweeper_->SetMetadata(&metadata_);
  SetClockExpectations();

  for (int i = 0; i < kRoundIterations; i++) {
    auto index_key = IndexedDBKey(i, blink::kWebIDBKeyTypeNumber);
    auto primary_key = IndexedDBKey(i + 1, blink::kWebIDBKeyTypeNumber);
    std::string value_str;
    EncodeVarInt(1, &value_str);
    EncodeIDBKey(primary_key, &value_str);
    in_memory_db_->Put(
        IndexDataKey::Encode(kDb1, kOs1, kIndex1, index_key, primary_key),
        &value_str);

    std::string exists_value;
    std::string encoded_primary_key;
    EncodeIDBKey(primary_key, &encoded_primary_key);
    EncodeVarInt(1, &exists_value);
    in_memory_db_->Put(ExistsEntryKey::Encode(kDb1, kOs1, encoded_primary_key),
                       &exists_value);
  }

  ASSERT_FALSE(sweeper_->RunRound());
  EXPECT_TRUE(sweeper_->RunRound());

  ExpectTaskTimeRecorded();
  ExpectUmaTombstones(0, 0);
  histogram_tester_.ExpectUniqueSample(
      "WebCore.IndexedDB.TombstoneSweeper.IndexScanPercent", 20, 1);
}

TEST_P(IndexedDBTombstoneSweeperTest, SimpleRealDBWithTombstones) {
  PopulateSingleIndexDBMetadata();
  SetupRealDB();
  sweeper_->SetMetadata(&metadata_);
  SetClockExpectations();

  int tombstones = 0;
  for (int i = 0; i < kRoundIterations + 1; i++) {
    auto index_key = IndexedDBKey(i, blink::kWebIDBKeyTypeNumber);
    auto primary_key = IndexedDBKey(i + 1, blink::kWebIDBKeyTypeNumber);
    std::string value_str;
    EncodeVarInt(1, &value_str);
    EncodeIDBKey(primary_key, &value_str);
    in_memory_db_->Put(
        IndexDataKey::Encode(kDb1, kOs1, kIndex1, index_key, primary_key),
        &value_str);

    std::string exists_value;
    std::string encoded_primary_key;
    EncodeIDBKey(primary_key, &encoded_primary_key);
    bool tombstone = i % 2 != 0;
    tombstones += i % 2 ? 1 : 0;
    EncodeVarInt(tombstone ? 2 : 1, &exists_value);
    in_memory_db_->Put(ExistsEntryKey::Encode(kDb1, kOs1, encoded_primary_key),
                       &exists_value);
  }

  ASSERT_FALSE(sweeper_->RunRound());
  EXPECT_TRUE(sweeper_->RunRound());

  ExpectTaskTimeRecorded();
  ExpectUmaTombstones(tombstones, kTombstoneSize * tombstones);
  histogram_tester_.ExpectUniqueSample(
      "WebCore.IndexedDB.TombstoneSweeper.IndexScanPercent", 20, 1);

  for (int i = 0; i < kRoundIterations + 1; i++) {
    if (i % 2 == 1) {
      std::string out;
      bool found = false;
      auto index_key = IndexedDBKey(i, blink::kWebIDBKeyTypeNumber);
      auto primary_key = IndexedDBKey(i + 1, blink::kWebIDBKeyTypeNumber);
      EXPECT_TRUE(in_memory_db_
                      ->Get(IndexDataKey::Encode(kDb1, kOs1, kIndex1, index_key,
                                                 primary_key),
                            &out, &found)
                      .ok());
      EXPECT_TRUE(GetParam() == Mode::STATISTICS || !found);
    }
  }
}

TEST_P(IndexedDBTombstoneSweeperTest, HitMaxIters) {
  PopulateSingleIndexDBMetadata();
  SetupRealDB();
  sweeper_->SetMetadata(&metadata_);
  SetFirstClockExpectation();

  for (int i = 0; i < kMaxIterations + 1; i++) {
    auto index_key = IndexedDBKey(i, blink::kWebIDBKeyTypeNumber);
    auto primary_key = IndexedDBKey(i + 1, blink::kWebIDBKeyTypeNumber);
    std::string value_str;
    EncodeVarInt(1, &value_str);
    EncodeIDBKey(primary_key, &value_str);
    in_memory_db_->Put(
        IndexDataKey::Encode(kDb1, kOs1, kIndex1, index_key, primary_key),
        &value_str);

    std::string exists_value;
    std::string encoded_primary_key;
    EncodeIDBKey(primary_key, &encoded_primary_key);
    EncodeVarInt(i % 2 == 0 ? 2 : 1, &exists_value);
    in_memory_db_->Put(ExistsEntryKey::Encode(kDb1, kOs1, encoded_primary_key),
                       &exists_value);
  }

  while (!sweeper_->RunRound())
    ;

  ExpectUmaTombstones(41, kTombstoneSize * 41, true);
  histogram_tester_.ExpectUniqueSample(
      "WebCore.IndexedDB.TombstoneSweeper.IndexScanPercent", 0, 1);
}

TEST_P(IndexedDBTombstoneSweeperTest, LevelDBError) {
  SetupMockDB();
  PopulateMultiDBMetdata();
  sweeper_->SetMetadata(&metadata_);
  SetFirstClockExpectation();

  // We'll have one index entry per index, and simulate reaching the end.
  leveldb::MockIterator* mock_iterator = new leveldb::MockIterator();
  EXPECT_CALL(mock_db_, NewIterator(testing::_))
      .WillOnce(testing::Return(mock_iterator));

  // First index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb1, kOs2, kIndex1))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));

    ExpectIndexAndExistsEntries(*mock_iterator, kDb1, kOs2, kIndex1,
                                IndexedDBKey(10, blink::kWebIDBKeyTypeNumber),
                                IndexedDBKey(20, blink::kWebIDBKeyTypeNumber),
                                1, 1);
    EXPECT_CALL(*mock_iterator, Next());

    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    // Return the beginning of the second index, which should cause us to error
    // & go restart our index seek.
    ExpectIndexEntry(*mock_iterator, kDb1, kOs2, kIndex2,
                     IndexedDBKey(30, blink::kWebIDBKeyTypeNumber),
                     IndexedDBKey(10, blink::kWebIDBKeyTypeNumber), 1);
  }

  // Second index.
  {
    testing::InSequence sequence_enforcer;
    EXPECT_CALL(*mock_iterator,
                Seek(SliceEq(IndexDataKey::EncodeMinKey(kDb1, kOs2, kIndex2))));
    EXPECT_CALL(*mock_iterator, Valid()).Times(2).WillRepeatedly(Return(true));
    ExpectIndexAndExistsEntries(*mock_iterator, kDb1, kOs2, kIndex2,
                                IndexedDBKey(30, blink::kWebIDBKeyTypeNumber),
                                IndexedDBKey(10, blink::kWebIDBKeyTypeNumber),
                                1, 1);
    EXPECT_CALL(*mock_iterator, Next());

    // Return read error.
    EXPECT_CALL(*mock_iterator, Valid()).WillOnce(Return(false));
    EXPECT_CALL(*mock_iterator, status())
        .WillOnce(Return(Status::Corruption("Test error")));
  }

  ASSERT_TRUE(sweeper_->RunRound());

  histogram_tester_.ExpectUniqueSample(
      "WebCore.IndexedDB.TombstoneSweeper.SweepError",
      leveldb_env::GetLevelDBStatusUMAValue(Status::Corruption("")), 1);
  // Only finished scanning the first index.
  histogram_tester_.ExpectUniqueSample(
      "WebCore.IndexedDB.TombstoneSweeper.IndexScanPercent", 1 * 20 / 3, 1);
}

INSTANTIATE_TEST_CASE_P(/* No prefix needed */,
                        IndexedDBTombstoneSweeperTest,
                        testing::Values(Mode::STATISTICS, Mode::DELETION));

}  // namespace indexed_db_tombstone_sweeper_unittest
}  // namespace content
