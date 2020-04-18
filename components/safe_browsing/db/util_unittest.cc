// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <algorithm>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "components/safe_browsing/db/util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace safe_browsing {

TEST(SafeBrowsingDbUtilTest, UrlToFullHashes) {
  std::vector<SBFullHash> results;
  GURL url("http://www.evil.com/evil1/evilness.html");
  UrlToFullHashes(url, false, &results);

  EXPECT_EQ(6UL, results.size());
  EXPECT_TRUE(SBFullHashEqual(SBFullHashForString("evil.com/"), results[0]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("evil.com/evil1/"), results[1]));
  EXPECT_TRUE(SBFullHashEqual(
      SBFullHashForString("evil.com/evil1/evilness.html"), results[2]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("www.evil.com/"), results[3]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("www.evil.com/evil1/"), results[4]));
  EXPECT_TRUE(SBFullHashEqual(
      SBFullHashForString("www.evil.com/evil1/evilness.html"), results[5]));

  results.clear();
  GURL url2("http://www.evil.com/evil1/evilness.html");
  UrlToFullHashes(url2, true, &results);

  EXPECT_EQ(8UL, results.size());
  EXPECT_TRUE(SBFullHashEqual(SBFullHashForString("evil.com/"), results[0]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("evil.com/evil1/"), results[1]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("evil.com/evil1"), results[2]));
  EXPECT_TRUE(SBFullHashEqual(
      SBFullHashForString("evil.com/evil1/evilness.html"), results[3]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("www.evil.com/"), results[4]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("www.evil.com/evil1/"), results[5]));
  EXPECT_TRUE(
      SBFullHashEqual(SBFullHashForString("www.evil.com/evil1"), results[6]));
  EXPECT_TRUE(SBFullHashEqual(
      SBFullHashForString("www.evil.com/evil1/evilness.html"), results[7]));
}

TEST(SafeBrowsingDbUtilTest, ListIdListNameConversion) {
  std::string list_name;
  EXPECT_FALSE(GetListName(INVALID, &list_name));
  EXPECT_TRUE(GetListName(MALWARE, &list_name));
  EXPECT_EQ(list_name, std::string(kMalwareList));
  EXPECT_EQ(MALWARE, GetListId(list_name));

  EXPECT_TRUE(GetListName(PHISH, &list_name));
  EXPECT_EQ(list_name, std::string(kPhishingList));
  EXPECT_EQ(PHISH, GetListId(list_name));

  EXPECT_TRUE(GetListName(BINURL, &list_name));
  EXPECT_EQ(list_name, std::string(kBinUrlList));
  EXPECT_EQ(BINURL, GetListId(list_name));
}

// Since the ids are saved in file, we need to make sure they don't change.
// Since only the last bit of each id is saved in file together with
// chunkids, this checks only last bit.
TEST(SafeBrowsingDbUtilTest, ListIdVerification) {
  EXPECT_EQ(0, MALWARE % 2);
  EXPECT_EQ(1, PHISH % 2);
  EXPECT_EQ(0, BINURL % 2);
}

TEST(SafeBrowsingDbUtilTest, StringToSBFullHashAndSBFullHashToString) {
  // 31 chars plus the last \0 as full_hash.
  const std::string hash_in = "12345678902234567890323456789012";
  SBFullHash hash_out = StringToSBFullHash(hash_in);
  EXPECT_EQ(0x34333231U, hash_out.prefix);
  EXPECT_EQ(0, memcmp(hash_in.data(), hash_out.full_hash, sizeof(SBFullHash)));

  std::string hash_final = SBFullHashToString(hash_out);
  EXPECT_EQ(hash_in, hash_final);
}

TEST(SafeBrowsingDbUtilTest, FullHashOperators) {
  const SBFullHash kHash1 = SBFullHashForString("one");
  const SBFullHash kHash2 = SBFullHashForString("two");

  EXPECT_TRUE(SBFullHashEqual(kHash1, kHash1));
  EXPECT_TRUE(SBFullHashEqual(kHash2, kHash2));
  EXPECT_FALSE(SBFullHashEqual(kHash1, kHash2));
  EXPECT_FALSE(SBFullHashEqual(kHash2, kHash1));

  EXPECT_FALSE(SBFullHashLess(kHash1, kHash2));
  EXPECT_TRUE(SBFullHashLess(kHash2, kHash1));

  EXPECT_FALSE(SBFullHashLess(kHash1, kHash1));
  EXPECT_FALSE(SBFullHashLess(kHash2, kHash2));
}

}  // namespace safe_browsing
