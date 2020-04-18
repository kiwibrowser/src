// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/safe_browsing_store_file.h"

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/md5.h"
#include "base/path_service.h"
#include "base/test/test_simple_task_runner.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"
#include "components/safe_browsing/db/prefix_set.h"
#include "components/safe_browsing/db/util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace safe_browsing {

namespace {

const int kAddChunk1 = 1;
const int kAddChunk2 = 3;
const int kAddChunk3 = 5;
const int kAddChunk4 = 7;
// Disjoint chunk numbers for subs to flush out typos.
const int kSubChunk1 = 2;
const int kSubChunk2 = 4;

const SBFullHash kHash1 = SBFullHashForString("one");
const SBFullHash kHash2 = SBFullHashForString("two");
const SBFullHash kHash3 = SBFullHashForString("three");
const SBFullHash kHash4 = SBFullHashForString("four");
const SBFullHash kHash5 = SBFullHashForString("five");
const SBFullHash kHash6 = SBFullHashForString("six");

const SBPrefix kMinSBPrefix = 0u;
const SBPrefix kMaxSBPrefix = ~kMinSBPrefix;

}  // namespace

class SafeBrowsingStoreFileTest : public PlatformTest {
 public:
  SafeBrowsingStoreFileTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        corruption_detected_(false) {}

  void SetUp() override {
    PlatformTest::SetUp();

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    filename_ = temp_dir_.GetPath();
    filename_ = filename_.AppendASCII("SafeBrowsingTestStore");

    store_.reset(new SafeBrowsingStoreFile(task_runner_));
    store_->Init(filename_,
                 base::Bind(&SafeBrowsingStoreFileTest::OnCorruptionDetected,
                            base::Unretained(this)));
  }
  void TearDown() override {
    if (store_.get())
      store_->Delete();
    store_.reset();

    PlatformTest::TearDown();
  }

  void OnCorruptionDetected() {
    corruption_detected_ = true;
  }

  // Populate the store with some testing data.
  void PopulateStore() {
    ASSERT_TRUE(store_->BeginUpdate());

    EXPECT_TRUE(store_->BeginChunk());
    store_->SetAddChunk(kAddChunk1);
    EXPECT_TRUE(store_->CheckAddChunk(kAddChunk1));
    EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash1.prefix));
    EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash2.prefix));
    EXPECT_TRUE(store_->FinishChunk());

    EXPECT_TRUE(store_->BeginChunk());
    store_->SetSubChunk(kSubChunk1);
    EXPECT_TRUE(store_->CheckSubChunk(kSubChunk1));
    EXPECT_TRUE(store_->WriteSubPrefix(kSubChunk1, kAddChunk3, kHash3.prefix));
    EXPECT_TRUE(store_->WriteSubHash(kSubChunk1, kAddChunk3, kHash3));
    EXPECT_TRUE(store_->FinishChunk());

    EXPECT_TRUE(store_->BeginChunk());
    store_->SetAddChunk(kAddChunk2);
    EXPECT_TRUE(store_->CheckAddChunk(kAddChunk2));
    EXPECT_TRUE(store_->WriteAddHash(kAddChunk2, kHash4));
    EXPECT_TRUE(store_->FinishChunk());

    // Chunk numbers shouldn't leak over.
    EXPECT_FALSE(store_->CheckAddChunk(kSubChunk1));
    EXPECT_FALSE(store_->CheckAddChunk(kAddChunk3));
    EXPECT_FALSE(store_->CheckSubChunk(kAddChunk1));
    EXPECT_FALSE(store_->CheckSubChunk(kAddChunk2));

    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;

    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));
  }

  // Manually read the shard stride info from the file.
  uint32_t ReadStride() {
    base::ScopedFILE file(base::OpenFile(filename_, "rb"));
    const long kOffset = 4 * sizeof(uint32_t);
    EXPECT_EQ(fseek(file.get(), kOffset, SEEK_SET), 0);
    uint32_t shard_stride = 0;
    EXPECT_EQ(fread(&shard_stride, sizeof(shard_stride), 1, file.get()), 1U);
    return shard_stride;
  }

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ScopedTempDir temp_dir_;
  base::FilePath filename_;
  std::unique_ptr<SafeBrowsingStoreFile> store_;
  bool corruption_detected_;
};

// Test that the empty store looks empty.
TEST_F(SafeBrowsingStoreFileTest, Empty) {
  ASSERT_TRUE(store_->BeginUpdate());

  std::vector<int> chunks;
  store_->GetAddChunks(&chunks);
  EXPECT_TRUE(chunks.empty());
  store_->GetSubChunks(&chunks);
  EXPECT_TRUE(chunks.empty());

  // Shouldn't see anything, but anything is a big set to test.
  EXPECT_FALSE(store_->CheckAddChunk(0));
  EXPECT_FALSE(store_->CheckAddChunk(1));
  EXPECT_FALSE(store_->CheckAddChunk(-1));

  EXPECT_FALSE(store_->CheckSubChunk(0));
  EXPECT_FALSE(store_->CheckSubChunk(1));
  EXPECT_FALSE(store_->CheckSubChunk(-1));

  PrefixSetBuilder builder;
  std::vector<SBAddFullHash> add_full_hashes_result;

  EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));
  EXPECT_TRUE(add_full_hashes_result.empty());

  std::vector<SBPrefix> prefixes_result;
  builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
  EXPECT_TRUE(prefixes_result.empty());
}

// Write some prefix and hash data to the store, add more data in another
// transaction, then verify that the union of all the data is present.
TEST_F(SafeBrowsingStoreFileTest, BasicStore) {
  PopulateStore();

  ASSERT_TRUE(store_->BeginUpdate());

  std::vector<int> chunks;
  store_->GetAddChunks(&chunks);
  ASSERT_EQ(2U, chunks.size());
  EXPECT_EQ(kAddChunk1, chunks[0]);
  EXPECT_EQ(kAddChunk2, chunks[1]);

  store_->GetSubChunks(&chunks);
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(kSubChunk1, chunks[0]);

  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk1));
  EXPECT_TRUE(store_->CheckSubChunk(kSubChunk1));

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk3);
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk3, kHash5.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  // Still has the chunks expected in the next update.
  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(3U, prefixes_result.size());
    EXPECT_EQ(kHash1.prefix, prefixes_result[0]);
    EXPECT_EQ(kHash5.prefix, prefixes_result[1]);
    EXPECT_EQ(kHash2.prefix, prefixes_result[2]);

    ASSERT_EQ(1U, add_full_hashes_result.size());
    EXPECT_EQ(kAddChunk2, add_full_hashes_result[0].chunk_id);
    EXPECT_TRUE(SBFullHashEqual(
        kHash4, add_full_hashes_result[0].full_hash));
  }
}

// Verify that the min and max prefixes are stored and operated on.
TEST_F(SafeBrowsingStoreFileTest, PrefixMinMax) {
  PopulateStore();

  ASSERT_TRUE(store_->BeginUpdate());

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk3);
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk3, kMinSBPrefix));
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk3, kMaxSBPrefix));
  EXPECT_TRUE(store_->FinishChunk());

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(4U, prefixes_result.size());
    EXPECT_EQ(kMinSBPrefix, prefixes_result[0]);
    EXPECT_EQ(kHash1.prefix, prefixes_result[1]);
    EXPECT_EQ(kHash2.prefix, prefixes_result[2]);
    EXPECT_EQ(kMaxSBPrefix, prefixes_result[3]);
  }

  ASSERT_TRUE(store_->BeginUpdate());

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kSubChunk2);
  EXPECT_TRUE(store_->WriteSubPrefix(kSubChunk2, kAddChunk3, kMinSBPrefix));
  EXPECT_TRUE(store_->WriteSubPrefix(kSubChunk2, kAddChunk3, kMaxSBPrefix));
  EXPECT_TRUE(store_->FinishChunk());

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(2U, prefixes_result.size());
    EXPECT_EQ(kHash1.prefix, prefixes_result[0]);
    EXPECT_EQ(kHash2.prefix, prefixes_result[1]);
  }
}

// Test that subs knockout adds.
TEST_F(SafeBrowsingStoreFileTest, SubKnockout) {
  ASSERT_TRUE(store_->BeginUpdate());

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk1);
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash1.prefix));
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash2.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk2);
  EXPECT_TRUE(store_->WriteAddHash(kAddChunk2, kHash4));
  EXPECT_TRUE(store_->FinishChunk());

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetSubChunk(kSubChunk1);
  EXPECT_TRUE(store_->WriteSubPrefix(kSubChunk1, kAddChunk3, kHash3.prefix));
  EXPECT_TRUE(store_->WriteSubPrefix(kSubChunk1, kAddChunk1, kHash2.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    // Knocked out the chunk expected.
    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(1U, prefixes_result.size());
    EXPECT_EQ(kHash1.prefix, prefixes_result[0]);

    ASSERT_EQ(1U, add_full_hashes_result.size());
    EXPECT_EQ(kAddChunk2, add_full_hashes_result[0].chunk_id);
    EXPECT_TRUE(SBFullHashEqual(
        kHash4, add_full_hashes_result[0].full_hash));
  }

  ASSERT_TRUE(store_->BeginUpdate());

  // This add should be knocked out by an existing sub.
  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk3);
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk3, kHash3.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(1U, prefixes_result.size());
    EXPECT_EQ(kHash1.prefix, prefixes_result[0]);

    ASSERT_EQ(1U, add_full_hashes_result.size());
    EXPECT_EQ(kAddChunk2, add_full_hashes_result[0].chunk_id);
    EXPECT_TRUE(SBFullHashEqual(
        kHash4, add_full_hashes_result[0].full_hash));
  }

  ASSERT_TRUE(store_->BeginUpdate());

  // But by here the sub should be gone, so it should stick this time.
  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk3);
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk3, kHash3.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(2U, prefixes_result.size());
    EXPECT_EQ(kHash1.prefix, prefixes_result[0]);
    EXPECT_EQ(kHash3.prefix, prefixes_result[1]);

    ASSERT_EQ(1U, add_full_hashes_result.size());
    EXPECT_EQ(kAddChunk2, add_full_hashes_result[0].chunk_id);
    EXPECT_TRUE(SBFullHashEqual(
        kHash4, add_full_hashes_result[0].full_hash));
  }
}

// Test that deletes delete the chunk's data.
TEST_F(SafeBrowsingStoreFileTest, DeleteChunks) {
  ASSERT_TRUE(store_->BeginUpdate());

  // A prefix chunk which will be deleted.
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk1));
  store_->SetAddChunk(kAddChunk1);
  EXPECT_TRUE(store_->BeginChunk());
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash1.prefix));
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash2.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  // A prefix chunk which won't be deleted.
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk2));
  store_->SetAddChunk(kAddChunk2);
  EXPECT_TRUE(store_->BeginChunk());
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk2, kHash3.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  // A full-hash chunk which won't be deleted.
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk3));
  store_->SetAddChunk(kAddChunk3);
  EXPECT_TRUE(store_->BeginChunk());
  EXPECT_TRUE(store_->WriteAddHash(kAddChunk3, kHash6));
  EXPECT_TRUE(store_->FinishChunk());

  // A sub chunk to delete.
  EXPECT_FALSE(store_->CheckSubChunk(kSubChunk1));
  store_->SetSubChunk(kSubChunk1);
  EXPECT_TRUE(store_->BeginChunk());
  EXPECT_TRUE(store_->WriteSubHash(kSubChunk1, kAddChunk3, kHash4));
  EXPECT_TRUE(store_->FinishChunk());

  // A sub chunk to keep.
  EXPECT_FALSE(store_->CheckSubChunk(kSubChunk2));
  store_->SetSubChunk(kSubChunk2);
  EXPECT_TRUE(store_->BeginChunk());
  EXPECT_TRUE(store_->WriteSubHash(kSubChunk2, kAddChunk4, kHash5));
  EXPECT_TRUE(store_->FinishChunk());

  store_->DeleteAddChunk(kAddChunk1);
  store_->DeleteSubChunk(kSubChunk1);

  // Not actually deleted until finish.
  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk1));
  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk2));
  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk3));
  EXPECT_TRUE(store_->CheckSubChunk(kSubChunk1));
  EXPECT_TRUE(store_->CheckSubChunk(kSubChunk2));

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(1U, prefixes_result.size());
    EXPECT_EQ(kHash3.prefix, prefixes_result[0]);

    ASSERT_EQ(1U, add_full_hashes_result.size());
    EXPECT_EQ(kAddChunk3, add_full_hashes_result[0].chunk_id);
    EXPECT_TRUE(SBFullHashEqual(
        kHash6, add_full_hashes_result[0].full_hash));
  }

  // Expected chunks are there in another update.
  ASSERT_TRUE(store_->BeginUpdate());
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk1));
  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk2));
  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk3));
  EXPECT_FALSE(store_->CheckSubChunk(kSubChunk1));
  EXPECT_TRUE(store_->CheckSubChunk(kSubChunk2));

  // Delete them, too.
  store_->DeleteAddChunk(kAddChunk2);
  store_->DeleteAddChunk(kAddChunk3);
  store_->DeleteSubChunk(kSubChunk2);

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    EXPECT_TRUE(prefixes_result.empty());
    EXPECT_TRUE(add_full_hashes_result.empty());
  }

  // Expect no more chunks.
  ASSERT_TRUE(store_->BeginUpdate());
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk1));
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk2));
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk3));
  EXPECT_FALSE(store_->CheckSubChunk(kSubChunk1));
  EXPECT_FALSE(store_->CheckSubChunk(kSubChunk2));

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    EXPECT_TRUE(prefixes_result.empty());
    EXPECT_TRUE(add_full_hashes_result.empty());
  }
}

// Test that deleting the store deletes the store.
TEST_F(SafeBrowsingStoreFileTest, Delete) {
  // Delete should work if the file wasn't there in the first place.
  EXPECT_FALSE(base::PathExists(filename_));
  EXPECT_TRUE(store_->Delete());

  // Create a store file.
  PopulateStore();

  EXPECT_TRUE(base::PathExists(filename_));
  EXPECT_TRUE(store_->Delete());
  EXPECT_FALSE(base::PathExists(filename_));
}

// Test that Delete() deletes the temporary store, if present.
TEST_F(SafeBrowsingStoreFileTest, DeleteTemp) {
  const base::FilePath temp_file =
      SafeBrowsingStoreFile::TemporaryFileForFilename(filename_);

  EXPECT_FALSE(base::PathExists(filename_));
  EXPECT_FALSE(base::PathExists(temp_file));

  // Starting a transaction creates a temporary file.
  ASSERT_TRUE(store_->BeginUpdate());
  EXPECT_TRUE(base::PathExists(temp_file));

  // Pull the rug out from under the existing store, simulating a
  // crash.
  store_.reset(new SafeBrowsingStoreFile(task_runner_));
  store_->Init(filename_, base::Closure());
  EXPECT_FALSE(base::PathExists(filename_));
  EXPECT_TRUE(base::PathExists(temp_file));

  // Make sure the temporary file is deleted.
  EXPECT_TRUE(store_->Delete());
  EXPECT_FALSE(base::PathExists(filename_));
  EXPECT_FALSE(base::PathExists(temp_file));
}

// Test basic corruption-handling.
TEST_F(SafeBrowsingStoreFileTest, DetectsCorruption) {
  // Load a store with some data.
  PopulateStore();

  // Can successfully open and read the store.
  {
    std::vector<SBPrefix> orig_prefixes;
    std::vector<SBAddFullHash> orig_hashes;
    PrefixSetBuilder builder;
    ASSERT_TRUE(store_->BeginUpdate());
    EXPECT_TRUE(store_->FinishUpdate(&builder, &orig_hashes));
    builder.GetPrefixSetNoHashes()->GetPrefixes(&orig_prefixes);
    EXPECT_GT(orig_prefixes.size(), 0U);
    EXPECT_GT(orig_hashes.size(), 0U);
    EXPECT_FALSE(corruption_detected_);
  }

  // Corrupt the store.
  base::ScopedFILE file(base::OpenFile(filename_, "rb+"));
  const long kOffset = 60;
  EXPECT_EQ(fseek(file.get(), kOffset, SEEK_SET), 0);
  const uint32_t kZero = 0;
  uint32_t previous = kZero;
  EXPECT_EQ(fread(&previous, sizeof(previous), 1, file.get()), 1U);
  EXPECT_NE(previous, kZero);
  EXPECT_EQ(fseek(file.get(), kOffset, SEEK_SET), 0);
  EXPECT_EQ(fwrite(&kZero, sizeof(kZero), 1, file.get()), 1U);
  file.reset();

  // Update fails and corruption callback is called.
  std::vector<SBAddFullHash> add_hashes;
  corruption_detected_ = false;
  {
    PrefixSetBuilder builder;
    ASSERT_TRUE(store_->BeginUpdate());
    EXPECT_FALSE(store_->FinishUpdate(&builder, &add_hashes));
    EXPECT_TRUE(corruption_detected_);
  }

  // Make it look like there is a lot of add-chunks-seen data.
  const long kAddChunkCountOffset = 2 * sizeof(int32_t);
  const int32_t kLargeCount = 1000 * 1000 * 1000;
  file.reset(base::OpenFile(filename_, "rb+"));
  EXPECT_EQ(fseek(file.get(), kAddChunkCountOffset, SEEK_SET), 0);
  EXPECT_EQ(fwrite(&kLargeCount, sizeof(kLargeCount), 1, file.get()), 1U);
  file.reset();

  // Detects corruption and fails to even begin the update.
  corruption_detected_ = false;
  EXPECT_FALSE(store_->BeginUpdate());
  EXPECT_TRUE(corruption_detected_);
}

TEST_F(SafeBrowsingStoreFileTest, CheckValidity) {
  // Empty store is valid.
  EXPECT_FALSE(base::PathExists(filename_));
  ASSERT_TRUE(store_->BeginUpdate());
  EXPECT_FALSE(corruption_detected_);
  EXPECT_TRUE(store_->CheckValidity());
  EXPECT_FALSE(corruption_detected_);
  EXPECT_TRUE(store_->CancelUpdate());

  // A store with some data is valid.
  EXPECT_FALSE(base::PathExists(filename_));
  PopulateStore();
  EXPECT_TRUE(base::PathExists(filename_));
  ASSERT_TRUE(store_->BeginUpdate());
  EXPECT_FALSE(corruption_detected_);
  EXPECT_TRUE(store_->CheckValidity());
  EXPECT_FALSE(corruption_detected_);
  EXPECT_TRUE(store_->CancelUpdate());
}

// Corrupt the header.
TEST_F(SafeBrowsingStoreFileTest, CheckValidityHeader) {
  PopulateStore();
  EXPECT_TRUE(base::PathExists(filename_));

  // 37 is the most random prime number.  It's also past the initial header
  // struct, somewhere in the chunk list.
  const size_t kOffset = 37;

  {
    base::ScopedFILE file(base::OpenFile(filename_, "rb+"));
    EXPECT_EQ(0, fseek(file.get(), kOffset, SEEK_SET));
    EXPECT_GE(fputs("hello", file.get()), 0);
  }
  ASSERT_FALSE(store_->BeginUpdate());
  EXPECT_TRUE(corruption_detected_);
}

// Corrupt the prefix payload.
TEST_F(SafeBrowsingStoreFileTest, CheckValidityPayload) {
  PopulateStore();
  EXPECT_TRUE(base::PathExists(filename_));

  // 137 is the second most random prime number.  It's also past the header and
  // chunk-id area.  Corrupting the header would fail BeginUpdate() in which
  // case CheckValidity() cannot be called.
  const size_t kOffset = 137;

  {
    base::ScopedFILE file(base::OpenFile(filename_, "rb+"));
    EXPECT_EQ(0, fseek(file.get(), kOffset, SEEK_SET));
    EXPECT_GE(fputs("hello", file.get()), 0);
  }
  ASSERT_TRUE(store_->BeginUpdate());
  EXPECT_FALSE(corruption_detected_);
  EXPECT_FALSE(store_->CheckValidity());
  EXPECT_TRUE(corruption_detected_);
  EXPECT_TRUE(store_->CancelUpdate());
}

// Corrupt the checksum.
TEST_F(SafeBrowsingStoreFileTest, CheckValidityChecksum) {
  PopulateStore();
  EXPECT_TRUE(base::PathExists(filename_));

  // An offset from the end of the file which is in the checksum.
  const int kOffset = -static_cast<int>(sizeof(base::MD5Digest));

  {
    base::ScopedFILE file(base::OpenFile(filename_, "rb+"));
    EXPECT_EQ(0, fseek(file.get(), kOffset, SEEK_END));
    EXPECT_GE(fputs("hello", file.get()), 0);
  }
  ASSERT_TRUE(store_->BeginUpdate());
  EXPECT_FALSE(corruption_detected_);
  EXPECT_FALSE(store_->CheckValidity());
  EXPECT_TRUE(corruption_detected_);
  EXPECT_TRUE(store_->CancelUpdate());
}

TEST_F(SafeBrowsingStoreFileTest, GetAddPrefixesAndHashes) {
  ASSERT_TRUE(store_->BeginUpdate());

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk1);
  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk1));
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash1.prefix));
  EXPECT_TRUE(store_->WriteAddPrefix(kAddChunk1, kHash2.prefix));
  EXPECT_TRUE(store_->FinishChunk());

  EXPECT_TRUE(store_->BeginChunk());
  store_->SetAddChunk(kAddChunk2);
  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk2));
  EXPECT_TRUE(store_->WriteAddHash(kAddChunk2, kHash4));
  EXPECT_TRUE(store_->FinishChunk());

  store_->SetSubChunk(kSubChunk1);
  EXPECT_TRUE(store_->CheckSubChunk(kSubChunk1));
  EXPECT_TRUE(store_->WriteSubPrefix(kSubChunk1, kAddChunk3, kHash3.prefix));
  EXPECT_TRUE(store_->WriteSubHash(kSubChunk1, kAddChunk3, kHash3));
  EXPECT_TRUE(store_->FinishChunk());

  // Chunk numbers shouldn't leak over.
  EXPECT_FALSE(store_->CheckAddChunk(kSubChunk1));
  EXPECT_FALSE(store_->CheckAddChunk(kAddChunk3));
  EXPECT_FALSE(store_->CheckSubChunk(kAddChunk1));

  std::vector<int> chunks;
  store_->GetAddChunks(&chunks);
  ASSERT_EQ(2U, chunks.size());
  EXPECT_EQ(kAddChunk1, chunks[0]);
  EXPECT_EQ(kAddChunk2, chunks[1]);

  store_->GetSubChunks(&chunks);
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(kSubChunk1, chunks[0]);

  PrefixSetBuilder builder;
  std::vector<SBAddFullHash> add_full_hashes_result;
  EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

  SBAddPrefixes add_prefixes;
  EXPECT_TRUE(store_->GetAddPrefixes(&add_prefixes));
  ASSERT_EQ(2U, add_prefixes.size());
  EXPECT_EQ(kAddChunk1, add_prefixes[0].chunk_id);
  EXPECT_EQ(kHash1.prefix, add_prefixes[0].prefix);
  EXPECT_EQ(kAddChunk1, add_prefixes[1].chunk_id);
  EXPECT_EQ(kHash2.prefix, add_prefixes[1].prefix);

  std::vector<SBAddFullHash> add_hashes;
  EXPECT_TRUE(store_->GetAddFullHashes(&add_hashes));
  ASSERT_EQ(1U, add_hashes.size());
  EXPECT_EQ(kAddChunk2, add_hashes[0].chunk_id);
  EXPECT_TRUE(SBFullHashEqual(kHash4, add_hashes[0].full_hash));
}

// Test that the database handles resharding correctly, both when growing and
// which shrinking.
TEST_F(SafeBrowsingStoreFileTest, Resharding) {
  // Loop through multiple stride boundaries (1<<32, 1<<31, 1<<30, 1<<29).
  const uint32_t kTargetStride = 1 << 29;

  // Each chunk will require 8 bytes per prefix, plus 4 bytes for chunk
  // information.  It should be less than |kTargetFootprint| in the
  // implementation, but high enough to keep the number of rewrites modest (to
  // keep the test fast).
  const size_t kPrefixesPerChunk = 10000;

  uint32_t shard_stride = 0;
  int chunk_id = 1;

  // Add a series of chunks, tracking that the stride size changes in a
  // direction appropriate to increasing file size.
  do {
    ASSERT_TRUE(store_->BeginUpdate());

    EXPECT_TRUE(store_->BeginChunk());
    store_->SetAddChunk(chunk_id);
    EXPECT_TRUE(store_->CheckAddChunk(chunk_id));
    for (size_t i = 0; i < kPrefixesPerChunk; ++i) {
      EXPECT_TRUE(store_->WriteAddPrefix(chunk_id, static_cast<SBPrefix>(i)));
    }
    EXPECT_TRUE(store_->FinishChunk());

    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    SBAddPrefixes add_prefixes;
    EXPECT_TRUE(store_->GetAddPrefixes(&add_prefixes));
    ASSERT_EQ(chunk_id * kPrefixesPerChunk, add_prefixes.size());

    // New stride should be the same, or shifted one right.
    const uint32_t new_shard_stride = ReadStride();
    EXPECT_TRUE((new_shard_stride == shard_stride) ||
                ((new_shard_stride << 1) == shard_stride));
    shard_stride = new_shard_stride;
    ++chunk_id;
  } while (!shard_stride || shard_stride > kTargetStride);

  // Guard against writing too many chunks.  If this gets too big, adjust
  // |kPrefixesPerChunk|.
  EXPECT_LT(chunk_id, 20);

  // Remove each chunk and check that the stride goes back to 0.
  while (--chunk_id) {
    ASSERT_TRUE(store_->BeginUpdate());
    EXPECT_TRUE(store_->CheckAddChunk(chunk_id));
    EXPECT_FALSE(store_->CheckAddChunk(chunk_id + 1));
    store_->DeleteAddChunk(chunk_id);

    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    // New stride should be the same, or shifted one left.
    const uint32_t new_shard_stride = ReadStride();
    EXPECT_TRUE((new_shard_stride == shard_stride) ||
                (new_shard_stride == (shard_stride << 1)));
    shard_stride = new_shard_stride;
  }
  EXPECT_EQ(0u, shard_stride);
}

// Test that a golden v7 file can no longer be read.  All platforms generating
// v7 files were little-endian, so there is no point to testing this transition
// if/when a big-endian port is added.
#if defined(ARCH_CPU_LITTLE_ENDIAN)
TEST_F(SafeBrowsingStoreFileTest, Version7) {
  store_.reset();

  // Copy the golden file into temporary storage.  The golden file contains:
  // - Add chunk kAddChunk1 containing kHash1.prefix and kHash2.
  // - Sub chunk kSubChunk1 containing kHash3.
  const char kBasename[] = "FileStoreVersion7";
  base::FilePath golden_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &golden_path));
  golden_path = golden_path.AppendASCII("SafeBrowsing");
  golden_path = golden_path.AppendASCII(kBasename);
  ASSERT_TRUE(base::CopyFile(golden_path, filename_));

  // Reset the store to make sure it re-reads the file.
  ASSERT_TRUE(!task_runner_->HasPendingTask());
  store_.reset(new SafeBrowsingStoreFile(task_runner_));
  store_->Init(filename_,
               base::Bind(&SafeBrowsingStoreFileTest::OnCorruptionDetected,
                          base::Unretained(this)));
  EXPECT_FALSE(corruption_detected_);

  // The unknown version should be encountered on the first read.
  EXPECT_FALSE(store_->BeginUpdate());
  EXPECT_TRUE(corruption_detected_);

  // No more to test because corrupt file stores are cleaned up by the database
  // containing them.
}
#endif

// Test that a golden v8 file can be read by the current code.  All platforms
// generating v8 files are little-endian, so there is no point to testing this
// transition if/when a big-endian port is added.
#if defined(ARCH_CPU_LITTLE_ENDIAN)
TEST_F(SafeBrowsingStoreFileTest, Version8) {
  store_.reset();

  // Copy the golden file into temporary storage.  The golden file contains:
  // - Add chunk kAddChunk1 containing kHash1.prefix and kHash2.
  // - Sub chunk kSubChunk1 containing kHash3.
  const char kBasename[] = "FileStoreVersion8";
  base::FilePath golden_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &golden_path));
  golden_path = golden_path.AppendASCII("SafeBrowsing");
  golden_path = golden_path.AppendASCII(kBasename);
  ASSERT_TRUE(base::CopyFile(golden_path, filename_));

  // Reset the store to make sure it re-reads the file.
  ASSERT_TRUE(!task_runner_->HasPendingTask());
  store_.reset(new SafeBrowsingStoreFile(task_runner_));
  store_->Init(filename_,
               base::Bind(&SafeBrowsingStoreFileTest::OnCorruptionDetected,
                          base::Unretained(this)));

  // Check that the expected prefixes and hashes are in place.
  SBAddPrefixes add_prefixes;
  EXPECT_TRUE(store_->GetAddPrefixes(&add_prefixes));
  ASSERT_EQ(2U, add_prefixes.size());
  EXPECT_EQ(kAddChunk1, add_prefixes[0].chunk_id);
  EXPECT_EQ(kHash1.prefix, add_prefixes[0].prefix);
  EXPECT_EQ(kAddChunk1, add_prefixes[1].chunk_id);
  EXPECT_EQ(kHash2.prefix, add_prefixes[1].prefix);

  std::vector<SBAddFullHash> add_hashes;
  EXPECT_TRUE(store_->GetAddFullHashes(&add_hashes));
  ASSERT_EQ(1U, add_hashes.size());
  EXPECT_EQ(kAddChunk1, add_hashes[0].chunk_id);
  EXPECT_TRUE(SBFullHashEqual(kHash2, add_hashes[0].full_hash));

  // Attempt an update to make sure things work end-to-end.
  EXPECT_TRUE(store_->BeginUpdate());

  // Still has the chunks expected in the next update.
  std::vector<int> chunks;
  store_->GetAddChunks(&chunks);
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(kAddChunk1, chunks[0]);

  store_->GetSubChunks(&chunks);
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(kSubChunk1, chunks[0]);

  EXPECT_TRUE(store_->CheckAddChunk(kAddChunk1));
  EXPECT_TRUE(store_->CheckSubChunk(kSubChunk1));

  // Sub chunk kAddChunk1 hash kHash2.
  // NOTE(shess): Having full hashes and prefixes in the same chunk is no longer
  // supported, though it was when this code was written.
  store_->SetSubChunk(kSubChunk2);
  EXPECT_TRUE(store_->CheckSubChunk(kSubChunk1));
  EXPECT_TRUE(store_->WriteSubPrefix(kSubChunk1, kAddChunk1, kHash2.prefix));
  EXPECT_TRUE(store_->WriteSubHash(kSubChunk1, kAddChunk1, kHash2));
  EXPECT_TRUE(store_->FinishChunk());

  {
    PrefixSetBuilder builder;
    std::vector<SBAddFullHash> add_full_hashes_result;
    EXPECT_TRUE(store_->FinishUpdate(&builder, &add_full_hashes_result));

    // The sub'ed prefix and hash are gone.
    std::vector<SBPrefix> prefixes_result;
    builder.GetPrefixSetNoHashes()->GetPrefixes(&prefixes_result);
    ASSERT_EQ(1U, prefixes_result.size());
    EXPECT_EQ(kHash1.prefix, prefixes_result[0]);
    EXPECT_TRUE(add_full_hashes_result.empty());
  }
}
#endif

}  // namespace safe_browsing
