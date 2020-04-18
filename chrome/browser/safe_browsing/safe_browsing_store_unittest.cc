// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>

#include "chrome/browser/safe_browsing/safe_browsing_store.h"
#include "components/safe_browsing/db/util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

namespace {

const SBFullHash kHash1 = SBFullHashForString("one");
const SBFullHash kHash2 = SBFullHashForString("two");
const SBFullHash kHash3 = SBFullHashForString("three");
const SBFullHash kHash4 = SBFullHashForString("four");
const SBFullHash kHash5 = SBFullHashForString("five");
const SBFullHash kHash6 = SBFullHashForString("six");
const SBFullHash kHash7 = SBFullHashForString("seven");

const int kAddChunk1 = 1;  // Use different chunk numbers just in case.
const int kSubChunk1 = 2;
const int kAddChunk2 = 3;
const int kSubChunk2 = 4;
const int kAddChunk3 = 5;
const int kSubChunk3 = 6;
const int kAddChunk4 = 7;
const int kSubChunk4 = 8;
const int kAddChunk5 = 9;
const int kSubChunk5 = 10;
const int kAddChunk6 = 11;
const int kAddChunk7 = 12;

SBFullHash ModifyHashAfterPrefix(SBFullHash hash, unsigned char mask) {
  hash.full_hash[sizeof(hash.full_hash) - 1] ^= mask;
  return hash;
}

void ProcessHelper(SBAddPrefixes* add_prefixes,
                   SBSubPrefixes* sub_prefixes,
                   std::vector<SBAddFullHash>* add_full_hashes,
                   std::vector<SBSubFullHash>* sub_full_hashes,
                   const base::hash_set<int32_t>& add_chunks_deleted,
                   const base::hash_set<int32_t>& sub_chunks_deleted) {
  std::sort(add_prefixes->begin(), add_prefixes->end(),
            SBAddPrefixLess<SBAddPrefix,SBAddPrefix>);
  std::sort(sub_prefixes->begin(), sub_prefixes->end(),
            SBAddPrefixLess<SBSubPrefix,SBSubPrefix>);
  std::sort(add_full_hashes->begin(), add_full_hashes->end(),
            SBAddPrefixHashLess<SBAddFullHash,SBAddFullHash>);
  std::sort(sub_full_hashes->begin(), sub_full_hashes->end(),
            SBAddPrefixHashLess<SBSubFullHash,SBSubFullHash>);

  SBProcessSubs(add_prefixes, sub_prefixes, add_full_hashes, sub_full_hashes,
                add_chunks_deleted, sub_chunks_deleted);
}

}  // namespace

TEST(SafeBrowsingStoreTest, SBAddPrefixLess) {
  // prefix dominates.
  EXPECT_TRUE(SBAddPrefixLess(SBAddPrefix(11, 1), SBAddPrefix(10, 2)));
  EXPECT_FALSE(SBAddPrefixLess(SBAddPrefix(10, 2), SBAddPrefix(11, 1)));

  // After prefix, chunk_id.
  EXPECT_TRUE(SBAddPrefixLess(SBAddPrefix(10, 1), SBAddPrefix(11, 1)));
  EXPECT_FALSE(SBAddPrefixLess(SBAddPrefix(11, 1), SBAddPrefix(10, 1)));

  // Equal is not less-than.
  EXPECT_FALSE(SBAddPrefixLess(SBAddPrefix(10, 1), SBAddPrefix(10, 1)));
}

TEST(SafeBrowsingStoreTest, SBAddPrefixHashLess) {
  // The first four bytes of SBFullHash can be read as a SBPrefix, which
  // means that byte-ordering issues can come up.  To test this, |one|
  // and |two| differ in the prefix, while |one| and |onetwo| have the
  // same prefix, but differ in the byte after the prefix.
  SBFullHash one, onetwo, two;
  memset(&one, 0, sizeof(one));
  memset(&onetwo, 0, sizeof(onetwo));
  memset(&two, 0, sizeof(two));
  one.prefix = 1;
  one.full_hash[sizeof(SBPrefix)] = 1;
  onetwo.prefix = 1;
  onetwo.full_hash[sizeof(SBPrefix)] = 2;
  two.prefix = 2;

  // prefix dominates.
  EXPECT_TRUE(SBAddPrefixHashLess(SBAddFullHash(11, one),
                                  SBAddFullHash(10, two)));
  EXPECT_FALSE(SBAddPrefixHashLess(SBAddFullHash(11, two),
                                   SBAddFullHash(10, one)));

  // After prefix, add_id.
  EXPECT_TRUE(SBAddPrefixHashLess(SBAddFullHash(10, one),
                                  SBAddFullHash(11, onetwo)));
  EXPECT_FALSE(SBAddPrefixHashLess(SBAddFullHash(11, one),
                                   SBAddFullHash(10, onetwo)));

  // After add_id, full hash.
  EXPECT_TRUE(SBAddPrefixHashLess(SBAddFullHash(10, one),
                                  SBAddFullHash(10, onetwo)));
  EXPECT_FALSE(SBAddPrefixHashLess(SBAddFullHash(10, onetwo),
                                   SBAddFullHash(10, one)));

  // Equal is not less-than.
  EXPECT_FALSE(SBAddPrefixHashLess(SBAddFullHash(10, one),
                                   SBAddFullHash(10, one)));
}

TEST(SafeBrowsingStoreTest, SBSubPrefixLess) {
  // prefix dominates.
  EXPECT_TRUE(SBAddPrefixLess(SBSubPrefix(12, 11, 1), SBSubPrefix(9, 10, 2)));
  EXPECT_FALSE(SBAddPrefixLess(SBSubPrefix(12, 11, 2), SBSubPrefix(9, 10, 1)));

  // After prefix, add_id.
  EXPECT_TRUE(SBAddPrefixLess(SBSubPrefix(12, 9, 1), SBSubPrefix(9, 10, 1)));
  EXPECT_FALSE(SBAddPrefixLess(SBSubPrefix(12, 10, 1), SBSubPrefix(9, 9, 1)));

  // Equal is not less-than.
  EXPECT_FALSE(SBAddPrefixLess(SBSubPrefix(12, 10, 1), SBSubPrefix(12, 10, 1)));

  // chunk_id doesn't matter.
}

TEST(SafeBrowsingStoreTest, SBSubFullHashLess) {
  SBFullHash one, onetwo, two;
  memset(&one, 0, sizeof(one));
  memset(&onetwo, 0, sizeof(onetwo));
  memset(&two, 0, sizeof(two));
  one.prefix = 1;
  one.full_hash[sizeof(SBPrefix)] = 1;
  onetwo.prefix = 1;
  onetwo.full_hash[sizeof(SBPrefix)] = 2;
  two.prefix = 2;

  // prefix dominates.
  EXPECT_TRUE(SBAddPrefixHashLess(SBSubFullHash(12, 11, one),
                                  SBSubFullHash(9, 10, two)));
  EXPECT_FALSE(SBAddPrefixHashLess(SBSubFullHash(12, 11, two),
                                   SBSubFullHash(9, 10, one)));

  // After prefix, add_id.
  EXPECT_TRUE(SBAddPrefixHashLess(SBSubFullHash(12, 10, one),
                                  SBSubFullHash(9, 11, onetwo)));
  EXPECT_FALSE(SBAddPrefixHashLess(SBSubFullHash(12, 11, one),
                                   SBSubFullHash(9, 10, onetwo)));

  // After add_id, full_hash.
  EXPECT_TRUE(SBAddPrefixHashLess(SBSubFullHash(12, 10, one),
                                  SBSubFullHash(9, 10, onetwo)));
  EXPECT_FALSE(SBAddPrefixHashLess(SBSubFullHash(12, 10, onetwo),
                                   SBSubFullHash(9, 10, one)));

  // Equal is not less-than.
  EXPECT_FALSE(SBAddPrefixHashLess(SBSubFullHash(12, 10, one),
                                   SBSubFullHash(9, 10, one)));
}

// SBProcessSubs does a lot of iteration, run through empty just to
// make sure degenerate cases work.
TEST(SafeBrowsingStoreTest, SBProcessSubsEmpty) {
  SBAddPrefixes add_prefixes;
  std::vector<SBAddFullHash> add_hashes;
  SBSubPrefixes sub_prefixes;
  std::vector<SBSubFullHash> sub_hashes;

  const base::hash_set<int32_t> no_deletions;
  SBProcessSubs(&add_prefixes, &sub_prefixes, &add_hashes, &sub_hashes,
                no_deletions, no_deletions);
  EXPECT_TRUE(add_prefixes.empty());
  EXPECT_TRUE(sub_prefixes.empty());
  EXPECT_TRUE(add_hashes.empty());
  EXPECT_TRUE(sub_hashes.empty());
}

// Test that subs knock out adds.
TEST(SafeBrowsingStoreTest, SBProcessSubsKnockout) {
  // A full hash which shares prefix with another.
  const SBFullHash kHash1mod = ModifyHashAfterPrefix(kHash1, 1);

  // A second full-hash for the full-hash-sub test.
  const SBFullHash kHash4mod = ModifyHashAfterPrefix(kHash4, 1);

  SBAddPrefixes add_prefixes;
  std::vector<SBAddFullHash> add_hashes;
  SBSubPrefixes sub_prefixes;
  std::vector<SBSubFullHash> sub_hashes;

  // An add prefix plus a sub to knock it out.
  add_prefixes.push_back(SBAddPrefix(kAddChunk1, kHash5.prefix));
  sub_prefixes.push_back(SBSubPrefix(kSubChunk1, kAddChunk1, kHash5.prefix));

  // Add hashes with same prefix, plus subs to knock them out.
  add_hashes.push_back(SBAddFullHash(kAddChunk2, kHash1));
  add_hashes.push_back(SBAddFullHash(kAddChunk2, kHash1mod));
  sub_hashes.push_back(SBSubFullHash(kSubChunk2, kAddChunk2, kHash1));
  sub_hashes.push_back(SBSubFullHash(kSubChunk2, kAddChunk2, kHash1mod));

  // Adds with no corresponding sub.  Both items should be retained.
  add_hashes.push_back(SBAddFullHash(kAddChunk6, kHash6));
  add_prefixes.push_back(SBAddPrefix(kAddChunk7, kHash2.prefix));

  // Subs with no corresponding add.  Both items should be retained.
  sub_hashes.push_back(SBSubFullHash(kSubChunk3, kAddChunk3, kHash7));
  sub_prefixes.push_back(SBSubPrefix(kSubChunk4, kAddChunk4, kHash3.prefix));

  // Add hashes with the same prefix, with a sub that will knock one of them
  // out.
  add_hashes.push_back(SBAddFullHash(kAddChunk5, kHash4));
  add_hashes.push_back(SBAddFullHash(kAddChunk5, kHash4mod));
  sub_hashes.push_back(SBSubFullHash(kSubChunk5, kAddChunk5, kHash4mod));

  const base::hash_set<int32_t> no_deletions;
  ProcessHelper(&add_prefixes, &sub_prefixes, &add_hashes, &sub_hashes,
                no_deletions, no_deletions);

  ASSERT_EQ(1U, add_prefixes.size());
  EXPECT_EQ(kAddChunk7, add_prefixes[0].chunk_id);
  EXPECT_EQ(kHash2.prefix, add_prefixes[0].prefix);

  ASSERT_EQ(2U, add_hashes.size());
  EXPECT_EQ(kAddChunk5, add_hashes[0].chunk_id);
  EXPECT_TRUE(SBFullHashEqual(kHash4, add_hashes[0].full_hash));
  EXPECT_EQ(kAddChunk6, add_hashes[1].chunk_id);
  EXPECT_TRUE(SBFullHashEqual(kHash6, add_hashes[1].full_hash));

  ASSERT_EQ(1U, sub_prefixes.size());
  EXPECT_EQ(kSubChunk4, sub_prefixes[0].chunk_id);
  EXPECT_EQ(kAddChunk4, sub_prefixes[0].add_chunk_id);
  EXPECT_EQ(kHash3.prefix, sub_prefixes[0].add_prefix);

  ASSERT_EQ(1U, sub_hashes.size());
  EXPECT_EQ(kSubChunk3, sub_hashes[0].chunk_id);
  EXPECT_EQ(kAddChunk3, sub_hashes[0].add_chunk_id);
  EXPECT_TRUE(SBFullHashEqual(kHash7, sub_hashes[0].full_hash));
}

// Test chunk deletions, and ordering of deletions WRT subs knocking
// out adds.
TEST(SafeBrowsingStoreTest, SBProcessSubsDeleteChunk) {
  // A full hash which shares prefix with another.
  const SBFullHash kHash1mod = ModifyHashAfterPrefix(kHash1, 1);

  SBAddPrefixes add_prefixes;
  std::vector<SBAddFullHash> add_hashes;
  SBSubPrefixes sub_prefixes;
  std::vector<SBSubFullHash> sub_hashes;

  // An add prefix plus a sub to knock it out.
  add_prefixes.push_back(SBAddPrefix(kAddChunk1, kHash5.prefix));
  sub_prefixes.push_back(SBSubPrefix(kSubChunk1, kAddChunk1, kHash5.prefix));

  // Add hashes with same prefix, plus subs to knock them out.
  add_hashes.push_back(SBAddFullHash(kAddChunk1, kHash1));
  add_hashes.push_back(SBAddFullHash(kAddChunk1, kHash1mod));
  sub_hashes.push_back(SBSubFullHash(kSubChunk1, kAddChunk1, kHash1));
  sub_hashes.push_back(SBSubFullHash(kSubChunk1, kAddChunk1, kHash1mod));

  // Adds with no corresponding sub.  Both items should be retained.
  add_hashes.push_back(SBAddFullHash(kAddChunk1, kHash6));
  add_prefixes.push_back(SBAddPrefix(kAddChunk1, kHash2.prefix));

  // Subs with no corresponding add.  Both items should be retained.
  sub_hashes.push_back(SBSubFullHash(kSubChunk1, kAddChunk1, kHash7));
  sub_prefixes.push_back(SBSubPrefix(kSubChunk1, kAddChunk1, kHash3.prefix));

  // Subs apply before being deleted.
  const base::hash_set<int32_t> no_deletions;
  base::hash_set<int32_t> sub_deletions;
  sub_deletions.insert(kSubChunk1);
  ProcessHelper(&add_prefixes, &sub_prefixes, &add_hashes, &sub_hashes,
                no_deletions, sub_deletions);

  ASSERT_EQ(1U, add_prefixes.size());
  EXPECT_EQ(kAddChunk1, add_prefixes[0].chunk_id);
  EXPECT_EQ(kHash2.prefix, add_prefixes[0].prefix);

  ASSERT_EQ(1U, add_hashes.size());
  EXPECT_EQ(kAddChunk1, add_hashes[0].chunk_id);
  EXPECT_TRUE(SBFullHashEqual(kHash6, add_hashes[0].full_hash));

  EXPECT_TRUE(sub_prefixes.empty());
  EXPECT_TRUE(sub_hashes.empty());

  // Delete the adds, also.
  base::hash_set<int32_t> add_deletions;
  add_deletions.insert(kAddChunk1);
  ProcessHelper(&add_prefixes, &sub_prefixes, &add_hashes, &sub_hashes,
                add_deletions, no_deletions);

  EXPECT_TRUE(add_prefixes.empty());
  EXPECT_TRUE(add_hashes.empty());
  EXPECT_TRUE(sub_prefixes.empty());
  EXPECT_TRUE(sub_hashes.empty());
}

TEST(SafeBrowsingStoreTest, Y2K38) {
  const base::Time now = base::Time::Now();
  const base::Time future = now + base::TimeDelta::FromDays(3*365);

  // TODO: Fix file format before 2035.
  EXPECT_GT(static_cast<int32_t>(future.ToTimeT()), 0)
      << " (int32_t)time_t is running out.";
}

}  // namespace safe_browsing
