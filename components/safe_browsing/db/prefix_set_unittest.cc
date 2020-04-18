// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/db/prefix_set.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>
#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "components/safe_browsing/db/util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace safe_browsing {

namespace {

const SBPrefix kHighBitClear = 1000u * 1000u * 1000u;
const SBPrefix kHighBitSet = 3u * 1000u * 1000u * 1000u;

}  // namespace

class PrefixSetTest : public PlatformTest {
 protected:
  // Constants for the v1 format.
  static const size_t kMagicOffset = 0 * sizeof(uint32_t);
  static const size_t kVersionOffset = 1 * sizeof(uint32_t);
  static const size_t kIndexSizeOffset = 2 * sizeof(uint32_t);
  static const size_t kDeltasSizeOffset = 3 * sizeof(uint32_t);
  static const size_t kFullHashesSizeOffset = 4 * sizeof(uint32_t);
  static const size_t kPayloadOffset = 5 * sizeof(uint32_t);

  // Generate a set of random prefixes to share between tests.  For
  // most tests this generation was a large fraction of the test time.
  //
  // The set should contain sparse areas where adjacent items are more
  // than 2^16 apart, and dense areas where adjacent items are less
  // than 2^16 apart.
  static void SetUpTestCase() {
    // Distribute clusters of prefixes.
    for (size_t i = 0; i < 250; ++i) {
      // Unsigned for overflow characteristics.
      const uint32_t base = static_cast<uint32_t>(base::RandUint64());
      for (size_t j = 0; j < 10; ++j) {
        const uint32_t delta =
            static_cast<uint32_t>(base::RandUint64() & 0xFFFF);
        const SBPrefix prefix = static_cast<SBPrefix>(base + delta);
        shared_prefixes_.push_back(prefix);
      }
    }

    // Lay down a sparsely-distributed layer.
    const size_t count = shared_prefixes_.size();
    for (size_t i = 0; i < count; ++i) {
      const SBPrefix prefix = static_cast<SBPrefix>(base::RandUint64());
      shared_prefixes_.push_back(prefix);
    }

    // Sort for use with PrefixSet constructor.
    std::sort(shared_prefixes_.begin(), shared_prefixes_.end());
  }

  // Check that all elements of |prefixes| are in |prefix_set|, and
  // that nearby elements are not (for lack of a more sensible set of
  // items to check for absence).
  static void CheckPrefixes(const PrefixSet& prefix_set,
                            const std::vector<SBPrefix>& prefixes) {
    // The set can generate the prefixes it believes it has, so that's
    // a good starting point.
    std::set<SBPrefix> check(prefixes.begin(), prefixes.end());
    std::vector<SBPrefix> prefixes_copy;
    prefix_set.GetPrefixes(&prefixes_copy);
    EXPECT_EQ(prefixes_copy.size(), check.size());
    EXPECT_TRUE(std::equal(check.begin(), check.end(), prefixes_copy.begin()));

    for (size_t i = 0; i < prefixes.size(); ++i) {
      EXPECT_TRUE(prefix_set.PrefixExists(prefixes[i]));

      const SBPrefix left_sibling = prefixes[i] - 1;
      if (check.count(left_sibling) == 0)
        EXPECT_FALSE(prefix_set.PrefixExists(left_sibling));

      const SBPrefix right_sibling = prefixes[i] + 1;
      if (check.count(right_sibling) == 0)
        EXPECT_FALSE(prefix_set.PrefixExists(right_sibling));
    }
  }

  // Generate a |PrefixSet| file from |shared_prefixes_|, store it in
  // a temporary file, and return the filename in |filenamep|.
  // Returns |true| on success.
  bool GetPrefixSetFile(base::FilePath* filenamep) {
    if (!temp_dir_.IsValid() && !temp_dir_.CreateUniqueTempDir())
      return false;

    base::FilePath filename = temp_dir_.GetPath().AppendASCII("PrefixSetTest");

    PrefixSetBuilder builder(shared_prefixes_);
    if (!builder.GetPrefixSetNoHashes()->WriteFile(filename))
      return false;

    *filenamep = filename;
    return true;
  }

  // Helper function to read the uint32_t value at |offset|, increment it
  // by |inc|, and write it back in place.  |fp| should be opened in
  // r+ mode.
  static void IncrementIntAt(FILE* fp, long offset, int inc) {
    uint32_t value = 0;

    ASSERT_NE(-1, fseek(fp, offset, SEEK_SET));
    ASSERT_EQ(1U, fread(&value, sizeof(value), 1, fp));

    value += inc;

    ASSERT_NE(-1, fseek(fp, offset, SEEK_SET));
    ASSERT_EQ(1U, fwrite(&value, sizeof(value), 1, fp));
  }

  // Helper function to re-generated |fp|'s checksum to be correct for
  // the file's contents.  |fp| should be opened in r+ mode.
  static void CleanChecksum(FILE* fp) {
    base::MD5Context context;
    base::MD5Init(&context);

    ASSERT_NE(-1, fseek(fp, 0, SEEK_END));
    long file_size = ftell(fp);

    using base::MD5Digest;
    size_t payload_size = static_cast<size_t>(file_size) - sizeof(MD5Digest);
    size_t digested_size = 0;
    ASSERT_NE(-1, fseek(fp, 0, SEEK_SET));
    while (digested_size < payload_size) {
      char buf[1024];
      size_t nitems = std::min(payload_size - digested_size, sizeof(buf));
      ASSERT_EQ(nitems, fread(buf, 1, nitems, fp));
      base::MD5Update(&context, base::StringPiece(buf, nitems));
      digested_size += nitems;
    }
    ASSERT_EQ(digested_size, payload_size);
    ASSERT_EQ(static_cast<long>(digested_size), ftell(fp));

    base::MD5Digest new_digest;
    base::MD5Final(&new_digest, &context);
    ASSERT_NE(-1, fseek(fp, digested_size, SEEK_SET));
    ASSERT_EQ(1U, fwrite(&new_digest, sizeof(new_digest), 1, fp));
    ASSERT_EQ(file_size, ftell(fp));
  }

  // Open |filename| and increment the uint32_t at |offset| by |inc|.
  // Then re-generate the checksum to account for the new contents.
  void ModifyAndCleanChecksum(const base::FilePath& filename,
                              long offset,
                              int inc) {
    int64_t size_64;
    ASSERT_TRUE(base::GetFileSize(filename, &size_64));

    base::ScopedFILE file(base::OpenFile(filename, "r+b"));
    IncrementIntAt(file.get(), offset, inc);
    CleanChecksum(file.get());
    file.reset();

    int64_t new_size_64;
    ASSERT_TRUE(base::GetFileSize(filename, &new_size_64));
    ASSERT_EQ(new_size_64, size_64);
  }

  base::FilePath TestFilePath() {
    base::FilePath path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
    return path.AppendASCII("components")
        .AppendASCII("test")
        .AppendASCII("data")
        .AppendASCII("SafeBrowsingDb");
  }

  // Fill |prefixes| with values read from a reference file.  The reference file
  // was generated from a specific |shared_prefixes_|.
  bool ReadReferencePrefixes(std::vector<SBPrefix>* prefixes) {
    const char kRefname[] = "PrefixSetRef";
    base::FilePath ref_path = TestFilePath();
    ref_path = ref_path.AppendASCII(kRefname);

    base::ScopedFILE file(base::OpenFile(ref_path, "r"));
    if (!file.get())
      return false;
    char buf[1024];
    while (fgets(buf, sizeof(buf), file.get())) {
      std::string trimmed;
      if (base::TRIM_TRAILING !=
          base::TrimWhitespaceASCII(buf, base::TRIM_ALL, &trimmed))
        return false;
      unsigned prefix;
      if (!base::StringToUint(trimmed, &prefix))
        return false;
      prefixes->push_back(prefix);
    }
    return true;
  }

  // Tests should not modify this shared resource.
  static std::vector<SBPrefix> shared_prefixes_;

  base::ScopedTempDir temp_dir_;
};

std::vector<SBPrefix> PrefixSetTest::shared_prefixes_;

// Test that a small sparse random input works.
TEST_F(PrefixSetTest, Baseline) {
  PrefixSetBuilder builder(shared_prefixes_);
  CheckPrefixes(*builder.GetPrefixSetNoHashes(), shared_prefixes_);
}

// Test that the empty set doesn't appear to have anything in it.
TEST_F(PrefixSetTest, Empty) {
  const std::vector<SBPrefix> empty;
  PrefixSetBuilder builder(empty);
  std::unique_ptr<const PrefixSet> prefix_set = builder.GetPrefixSetNoHashes();
  for (size_t i = 0; i < shared_prefixes_.size(); ++i) {
    EXPECT_FALSE(prefix_set->PrefixExists(shared_prefixes_[i]));
  }
}

// Single-element set should work fine.
TEST_F(PrefixSetTest, OneElement) {
  const std::vector<SBPrefix> prefixes(100, 0u);
  PrefixSetBuilder builder(prefixes);
  std::unique_ptr<const PrefixSet> prefix_set = builder.GetPrefixSetNoHashes();
  EXPECT_FALSE(prefix_set->PrefixExists(static_cast<SBPrefix>(-1)));
  EXPECT_TRUE(prefix_set->PrefixExists(prefixes[0]));
  EXPECT_FALSE(prefix_set->PrefixExists(1u));

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  EXPECT_EQ(1U, prefixes_copy.size());
  EXPECT_EQ(prefixes[0], prefixes_copy[0]);
}

// Edges of the 32-bit integer range.
TEST_F(PrefixSetTest, IntMinMax) {
  std::vector<SBPrefix> prefixes;

  // Using bit patterns rather than portable constants because this
  // really is testing how the entire 32-bit integer range is handled.
  prefixes.push_back(0x00000000);
  prefixes.push_back(0x0000FFFF);
  prefixes.push_back(0x7FFF0000);
  prefixes.push_back(0x7FFFFFFF);
  prefixes.push_back(0x80000000);
  prefixes.push_back(0x8000FFFF);
  prefixes.push_back(0xFFFF0000);
  prefixes.push_back(0xFFFFFFFF);

  std::sort(prefixes.begin(), prefixes.end());
  PrefixSetBuilder builder(prefixes);
  std::unique_ptr<const PrefixSet> prefix_set = builder.GetPrefixSetNoHashes();

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  ASSERT_EQ(prefixes_copy.size(), prefixes.size());
  EXPECT_TRUE(
      std::equal(prefixes.begin(), prefixes.end(), prefixes_copy.begin()));
}

// A range with only large deltas.
TEST_F(PrefixSetTest, AllBig) {
  std::vector<SBPrefix> prefixes;

  const unsigned kDelta = 10 * 1000 * 1000;
  for (SBPrefix prefix = kHighBitClear; prefix < kHighBitSet;
       prefix += kDelta) {
    prefixes.push_back(prefix);
  }

  std::sort(prefixes.begin(), prefixes.end());
  PrefixSetBuilder builder(prefixes);
  std::unique_ptr<const PrefixSet> prefix_set = builder.GetPrefixSetNoHashes();

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  prefixes.erase(std::unique(prefixes.begin(), prefixes.end()), prefixes.end());
  EXPECT_EQ(prefixes_copy.size(), prefixes.size());
  EXPECT_TRUE(
      std::equal(prefixes.begin(), prefixes.end(), prefixes_copy.begin()));
}

// Use artificial inputs to test various edge cases in PrefixExists().  Items
// before the lowest item aren't present.  Items after the largest item aren't
// present.  Create a sequence of items with deltas above and below 2^16, and
// make sure they're all present.  Create a very long sequence with deltas below
// 2^16 to test crossing |kMaxRun|.
TEST_F(PrefixSetTest, EdgeCases) {
  std::vector<SBPrefix> prefixes;

  // Put in a high-bit prefix.
  SBPrefix prefix = kHighBitSet;
  prefixes.push_back(prefix);

  // Add a sequence with very large deltas.
  unsigned delta = 100 * 1000 * 1000;
  for (int i = 0; i < 10; ++i) {
    prefix += delta;
    prefixes.push_back(prefix);
  }

  // Add a sequence with deltas that start out smaller than the
  // maximum delta, and end up larger.  Also include some duplicates.
  delta = 256 * 256 - 100;
  for (int i = 0; i < 200; ++i) {
    prefix += delta;
    prefixes.push_back(prefix);
    prefixes.push_back(prefix);
    delta++;
  }

  // Add a long sequence with deltas smaller than the maximum delta,
  // so a new index item will be injected.
  delta = 256 * 256 - 1;
  prefix = kHighBitClear - delta * 1000;
  prefixes.push_back(prefix);
  for (int i = 0; i < 1000; ++i) {
    prefix += delta;
    prefixes.push_back(prefix);
    delta--;
  }

  std::sort(prefixes.begin(), prefixes.end());
  PrefixSetBuilder builder(prefixes);
  std::unique_ptr<const PrefixSet> prefix_set = builder.GetPrefixSetNoHashes();

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  prefixes.erase(std::unique(prefixes.begin(), prefixes.end()), prefixes.end());
  EXPECT_EQ(prefixes_copy.size(), prefixes.size());
  EXPECT_TRUE(
      std::equal(prefixes.begin(), prefixes.end(), prefixes_copy.begin()));

  // Items before and after the set are not present, and don't crash.
  EXPECT_FALSE(prefix_set->PrefixExists(kHighBitSet - 100));
  EXPECT_FALSE(prefix_set->PrefixExists(kHighBitClear + 100));

  // Check that the set correctly flags all of the inputs, and also
  // check items just above and below the inputs to make sure they
  // aren't present.
  for (size_t i = 0; i < prefixes.size(); ++i) {
    EXPECT_TRUE(prefix_set->PrefixExists(prefixes[i]));

    EXPECT_FALSE(prefix_set->PrefixExists(prefixes[i] - 1));
    EXPECT_FALSE(prefix_set->PrefixExists(prefixes[i] + 1));
  }
}

// Test writing a prefix set to disk and reading it back in.
TEST_F(PrefixSetTest, ReadWrite) {
  base::FilePath filename;

  // Write the sample prefix set out, read it back in, and check all
  // the prefixes.  Leaves the path in |filename|.
  {
    ASSERT_TRUE(GetPrefixSetFile(&filename));
    std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
    ASSERT_TRUE(prefix_set.get());
    CheckPrefixes(*prefix_set, shared_prefixes_);
  }

  // Test writing and reading a very sparse set containing no deltas.
  {
    std::vector<SBPrefix> prefixes;
    prefixes.push_back(kHighBitClear);
    prefixes.push_back(kHighBitSet);

    PrefixSetBuilder builder(prefixes);
    ASSERT_TRUE(builder.GetPrefixSetNoHashes()->WriteFile(filename));

    std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
    ASSERT_TRUE(prefix_set.get());
    CheckPrefixes(*prefix_set, prefixes);
  }

  // Test writing and reading an empty set.
  {
    std::vector<SBPrefix> prefixes;
    PrefixSetBuilder builder(prefixes);
    ASSERT_TRUE(builder.GetPrefixSetNoHashes()->WriteFile(filename));

    std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
    ASSERT_TRUE(prefix_set.get());
    CheckPrefixes(*prefix_set, prefixes);
  }

  // Test that full hashes are persisted.
  {
    std::vector<SBFullHash> hashes;
    hashes.push_back(SBFullHashForString("one"));
    hashes.push_back(SBFullHashForString("two"));
    hashes.push_back(SBFullHashForString("three"));

    std::vector<SBPrefix> prefixes(shared_prefixes_);

    // Remove any collisions from the prefixes.
    for (size_t i = 0; i < hashes.size(); ++i) {
      std::vector<SBPrefix>::iterator iter =
          std::lower_bound(prefixes.begin(), prefixes.end(), hashes[i].prefix);
      if (iter != prefixes.end() && *iter == hashes[i].prefix)
        prefixes.erase(iter);
    }

    PrefixSetBuilder builder(prefixes);
    ASSERT_TRUE(builder.GetPrefixSet(hashes)->WriteFile(filename));

    std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
    ASSERT_TRUE(prefix_set.get());
    CheckPrefixes(*prefix_set, prefixes);

    EXPECT_TRUE(prefix_set->Exists(hashes[0]));
    EXPECT_TRUE(prefix_set->Exists(hashes[1]));
    EXPECT_TRUE(prefix_set->Exists(hashes[2]));
    EXPECT_FALSE(prefix_set->PrefixExists(hashes[0].prefix));
    EXPECT_FALSE(prefix_set->PrefixExists(hashes[1].prefix));
    EXPECT_FALSE(prefix_set->PrefixExists(hashes[2].prefix));
  }
}

// Check that |CleanChecksum()| makes an acceptable checksum.
TEST_F(PrefixSetTest, CorruptionHelpers) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // This will modify data in |index_|, which will fail the digest check.
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));
  IncrementIntAt(file.get(), kPayloadOffset, 1);
  file.reset();
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);

  // Fix up the checksum and it will read successfully (though the
  // data will be wrong).
  file.reset(base::OpenFile(filename, "r+b"));
  CleanChecksum(file.get());
  file.reset();
  prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_TRUE(prefix_set.get());
}

// Bad magic is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionMagic) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(ModifyAndCleanChecksum(filename, kMagicOffset, 1));
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Bad version is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionVersion) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(ModifyAndCleanChecksum(filename, kVersionOffset, 10));
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Bad |index_| size is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionIndexSize) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(
      ModifyAndCleanChecksum(filename, kIndexSizeOffset, 1));
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Bad |deltas_| size is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionDeltasSize) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(
      ModifyAndCleanChecksum(filename, kDeltasSizeOffset, 1));
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Bad |full_hashes_| size is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionFullHashesSize) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(
      ModifyAndCleanChecksum(filename, kFullHashesSizeOffset, 1));
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Test that the digest catches corruption in the middle of the file
// (in the payload between the header and the digest).
TEST_F(PrefixSetTest, CorruptionPayload) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  base::ScopedFILE file(base::OpenFile(filename, "r+b"));
  ASSERT_NO_FATAL_FAILURE(IncrementIntAt(file.get(), 666, 1));
  file.reset();
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Test corruption in the digest itself.
TEST_F(PrefixSetTest, CorruptionDigest) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  int64_t size_64;
  ASSERT_TRUE(base::GetFileSize(filename, &size_64));
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));
  long digest_offset = static_cast<long>(size_64 - sizeof(base::MD5Digest));
  ASSERT_NO_FATAL_FAILURE(IncrementIntAt(file.get(), digest_offset, 1));
  file.reset();
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Test excess data after the digest (fails the size test).
TEST_F(PrefixSetTest, CorruptionExcess) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // Add some junk to the trunk.
  base::ScopedFILE file(base::OpenFile(filename, "ab"));
  const char buf[] = "im in ur base, killing ur d00dz.";
  ASSERT_EQ(strlen(buf), fwrite(buf, 1, strlen(buf), file.get()));
  file.reset();
  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Test that files which had 64-bit size_t are discarded.
TEST_F(PrefixSetTest, SizeTRecovery) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // Open the file for rewrite.
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));

  // Leave existing magic and version.
  ASSERT_NE(-1, fseek(file.get(), sizeof(uint32_t) * 2, SEEK_SET));

  // Indicate two index values and two deltas.
  uint32_t val = 2;
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));

  // Write two index values with 64-bit "size_t".
  std::pair<SBPrefix, uint64_t> item;
  memset(&item, 0, sizeof(item));  // Includes any padding.
  item.first = 17;
  item.second = 0;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));
  item.first = 100042;
  item.second = 1;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));

  // Write two delta values.
  uint16_t delta = 23;
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));

  // Leave space for the digest at the end, and regenerate it.
  base::MD5Digest dummy = {{0}};
  ASSERT_EQ(sizeof(dummy), fwrite(&dummy, 1, sizeof(dummy), file.get()));
  ASSERT_TRUE(base::TruncateFile(file.get()));
  CleanChecksum(file.get());
  file.reset();  // Flush updates.

  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Test Exists() against full hashes passed to builder.
TEST_F(PrefixSetTest, FullHashBuild) {
  const SBFullHash kHash1 = SBFullHashForString("one");
  const SBFullHash kHash2 = SBFullHashForString("two");
  const SBFullHash kHash3 = SBFullHashForString("three");
  const SBFullHash kHash4 = SBFullHashForString("four");
  const SBFullHash kHash5 = SBFullHashForString("five");
  const SBFullHash kHash6 = SBFullHashForString("six");

  std::vector<SBPrefix> prefixes;
  prefixes.push_back(kHash1.prefix);
  prefixes.push_back(kHash2.prefix);
  std::sort(prefixes.begin(), prefixes.end());

  std::vector<SBFullHash> hashes;
  hashes.push_back(kHash4);
  hashes.push_back(kHash5);

  PrefixSetBuilder builder(prefixes);
  std::unique_ptr<const PrefixSet> prefix_set = builder.GetPrefixSet(hashes);

  EXPECT_TRUE(prefix_set->Exists(kHash1));
  EXPECT_TRUE(prefix_set->Exists(kHash2));
  EXPECT_FALSE(prefix_set->Exists(kHash3));
  EXPECT_TRUE(prefix_set->Exists(kHash4));
  EXPECT_TRUE(prefix_set->Exists(kHash5));
  EXPECT_FALSE(prefix_set->Exists(kHash6));

  EXPECT_TRUE(prefix_set->PrefixExists(kHash1.prefix));
  EXPECT_TRUE(prefix_set->PrefixExists(kHash2.prefix));
  EXPECT_FALSE(prefix_set->PrefixExists(kHash3.prefix));
  EXPECT_FALSE(prefix_set->PrefixExists(kHash4.prefix));
  EXPECT_FALSE(prefix_set->PrefixExists(kHash5.prefix));
  EXPECT_FALSE(prefix_set->PrefixExists(kHash6.prefix));
}

// Test that a version 1 file is discarded on read.
TEST_F(PrefixSetTest, ReadSigned) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // Open the file for rewrite.
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));

  // Leave existing magic.
  ASSERT_NE(-1, fseek(file.get(), sizeof(uint32_t), SEEK_SET));

  // Version 1.
  uint32_t version = 1;
  ASSERT_EQ(sizeof(version), fwrite(&version, 1, sizeof(version), file.get()));

  // Indicate two index values and two deltas.
  uint32_t val = 2;
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));

  std::pair<int32_t, uint32_t> item;
  memset(&item, 0, sizeof(item));  // Includes any padding.
  item.first = -1000;
  item.second = 0;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));
  item.first = 1000;
  item.second = 1;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));

  // Write two delta values.
  uint16_t delta = 23;
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));

  // Leave space for the digest at the end, and regenerate it.
  base::MD5Digest dummy = {{0}};
  ASSERT_EQ(sizeof(dummy), fwrite(&dummy, 1, sizeof(dummy), file.get()));
  ASSERT_TRUE(base::TruncateFile(file.get()));
  CleanChecksum(file.get());
  file.reset();  // Flush updates.

  std::unique_ptr<const PrefixSet> prefix_set = PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set);
}

// Test that a golden v2 file is discarded on read.  All platforms generating v2
// files are little-endian, so there is no point to testing this transition
// if/when a big-endian port is added.
#if defined(ARCH_CPU_LITTLE_ENDIAN)
TEST_F(PrefixSetTest, Version2) {
  std::vector<SBPrefix> ref_prefixes;
  ASSERT_TRUE(ReadReferencePrefixes(&ref_prefixes));

  const char kBasename[] = "PrefixSetVersion2";
  base::FilePath golden_path = TestFilePath();
  golden_path = golden_path.AppendASCII(kBasename);

  std::unique_ptr<const PrefixSet> prefix_set(PrefixSet::LoadFile(golden_path));
  ASSERT_FALSE(prefix_set);
}
#endif

// Test that a golden v3 file can be read by the current code.  All platforms
// generating v3 files are little-endian, so there is no point to testing this
// transition if/when a big-endian port is added.
#if defined(ARCH_CPU_LITTLE_ENDIAN)
TEST_F(PrefixSetTest, Version3) {
  std::vector<SBPrefix> ref_prefixes;
  ASSERT_TRUE(ReadReferencePrefixes(&ref_prefixes));

  const char kBasename[] = "PrefixSetVersion3";
  base::FilePath golden_path = TestFilePath();
  golden_path = golden_path.AppendASCII(kBasename);

  std::unique_ptr<const PrefixSet> prefix_set(PrefixSet::LoadFile(golden_path));
  ASSERT_TRUE(prefix_set.get());
  CheckPrefixes(*prefix_set, ref_prefixes);

  const SBFullHash kHash1 = SBFullHashForString("www.evil.com/malware.html");
  const SBFullHash kHash2 = SBFullHashForString("www.evil.com/phishing.html");

  EXPECT_TRUE(prefix_set->Exists(kHash1));
  EXPECT_TRUE(prefix_set->Exists(kHash2));
  EXPECT_FALSE(prefix_set->PrefixExists(kHash1.prefix));
  EXPECT_FALSE(prefix_set->PrefixExists(kHash2.prefix));
}
#endif

}  // namespace safe_browsing
