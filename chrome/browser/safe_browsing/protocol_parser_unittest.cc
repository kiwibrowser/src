// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "chrome/browser/safe_browsing/protocol_parser.h"
#include "components/safe_browsing/db/metadata.pb.h"
#include "components/safe_browsing/db/util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

namespace {

const char kDefaultPhishList[] = "goog-phish-shavar";
const char kDefaultMalwareList[] = "goog-malware-shavar";
const char kDefaultUwSList[] = "goog-unwanted-shavar";

}  // namespace

// Test parsing one add chunk.
TEST(SafeBrowsingProtocolParsingTest, TestAddChunk) {
  const char kRawAddChunk[] = {
    '\0', '\0', '\0', '\x1C',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x22',                    // field 4, wire format length-delimited
    '\x18',                    // varint length 24
    '1', '1', '1', '1',        // 4-byte prefixes
    '2', '2', '2', '2',
    '3', '3', '3', '3',
    '4', '4', '4', '4',
    '8', '8', '8', '8',
    '9', '9', '9', '9',
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_TRUE(ParseChunk(kRawAddChunk, sizeof(kRawAddChunk), &chunks));
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(1, chunks[0]->ChunkNumber());
  EXPECT_TRUE(chunks[0]->IsAdd());
  EXPECT_FALSE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[0]->IsPrefix());
  EXPECT_FALSE(chunks[0]->IsFullHash());
  ASSERT_EQ(6U, chunks[0]->PrefixCount());
  EXPECT_EQ(0x31313131U, chunks[0]->PrefixAt(0));  // 1111
  EXPECT_EQ(0x32323232U, chunks[0]->PrefixAt(1));  // 2222
  EXPECT_EQ(0x33333333U, chunks[0]->PrefixAt(2));  // 3333
  EXPECT_EQ(0x34343434U, chunks[0]->PrefixAt(3));  // 4444
  EXPECT_EQ(0x38383838U, chunks[0]->PrefixAt(4));  // 8888
  EXPECT_EQ(0x39393939U, chunks[0]->PrefixAt(5));  // 9999
}

// Test parsing one add chunk with full hashes.
TEST(SafeBrowsingProtocolParsingTest, TestAddFullChunk) {
  const char kRawAddChunk[] = {
    '\0', '\0', '\0', '\x46',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x18',                    // field 3, wire format varint
    '\x01',                    // enum PrefixType == FULL_32B
    '\x22',                    // field 4, wire format length-delimited
    '\x40',                    // varint length 64 (2 full hashes)

    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',

    '2', '3', '2', '3', '2', '3', '2', '3',
    '2', '3', '2', '3', '2', '3', '2', '3',
    '2', '3', '2', '3', '2', '3', '2', '3',
    '2', '3', '2', '3', '2', '3', '2', '3',
  };

  SBFullHash full_hash1, full_hash2;
  for (int i = 0; i < 32; ++i) {
    full_hash1.full_hash[i] = (i % 2) ? '1' : '0';
    full_hash2.full_hash[i] = (i % 2) ? '3' : '2';
  }

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_TRUE(ParseChunk(kRawAddChunk, sizeof(kRawAddChunk), &chunks));
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(1, chunks[0]->ChunkNumber());
  EXPECT_TRUE(chunks[0]->IsAdd());
  EXPECT_FALSE(chunks[0]->IsSub());
  EXPECT_FALSE(chunks[0]->IsPrefix());
  EXPECT_TRUE(chunks[0]->IsFullHash());

  ASSERT_EQ(2U, chunks[0]->FullHashCount());
  EXPECT_TRUE(SBFullHashEqual(chunks[0]->FullHashAt(0), full_hash1));
  EXPECT_TRUE(SBFullHashEqual(chunks[0]->FullHashAt(1), full_hash2));
}

// Test parsing multiple add chunks. We'll use the same chunk as above, and add
// one more after it.
TEST(SafeBrowsingProtocolParsingTest, TestAddChunks) {
  const char kRawAddChunk[] = {
    '\0', '\0', '\0', '\x1C',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x22',                    // field 4, wire format length-delimited
    '\x18',                    // varint length 24

    '1', '1', '1', '1',        // 4-byte prefixes
    '2', '2', '2', '2',
    '3', '3', '3', '3',
    '4', '4', '4', '4',
    '8', '8', '8', '8',
    '9', '9', '9', '9',

    '\0', '\0', '\0', '\x0C',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x02',                    // chunk_number varint 1
    '\x22',                    // field 4, wire format length-delimited
    '\x08',                    // varint length 8
    'p', 'p', 'p', 'p',        // 4-byte prefixes
    'g', 'g', 'g', 'g',
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_TRUE(ParseChunk(kRawAddChunk, sizeof(kRawAddChunk), &chunks));
  ASSERT_EQ(2U, chunks.size());

  EXPECT_EQ(1, chunks[0]->ChunkNumber());
  EXPECT_TRUE(chunks[0]->IsAdd());
  EXPECT_FALSE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[0]->IsPrefix());
  EXPECT_FALSE(chunks[0]->IsFullHash());
  ASSERT_EQ(6U, chunks[0]->PrefixCount());
  EXPECT_EQ(0x31313131U, chunks[0]->PrefixAt(0));  // 1111
  EXPECT_EQ(0x32323232U, chunks[0]->PrefixAt(1));  // 2222
  EXPECT_EQ(0x33333333U, chunks[0]->PrefixAt(2));  // 3333
  EXPECT_EQ(0x34343434U, chunks[0]->PrefixAt(3));  // 4444
  EXPECT_EQ(0x38383838U, chunks[0]->PrefixAt(4));  // 8888
  EXPECT_EQ(0x39393939U, chunks[0]->PrefixAt(5));  // 9999

  EXPECT_EQ(2, chunks[1]->ChunkNumber());
  EXPECT_TRUE(chunks[1]->IsAdd());
  EXPECT_FALSE(chunks[1]->IsSub());
  EXPECT_TRUE(chunks[1]->IsPrefix());
  EXPECT_FALSE(chunks[1]->IsFullHash());
  ASSERT_EQ(2U, chunks[1]->PrefixCount());
  EXPECT_EQ(0x70707070U, chunks[1]->PrefixAt(0));  // pppp
  EXPECT_EQ(0x67676767U, chunks[1]->PrefixAt(1));  // gggg
}

TEST(SafeBrowsingProtocolParsingTest, TestTruncatedPrefixChunk) {
  // This chunk delares there are 6 prefixes but actually only contains 3.
  const char kRawAddChunk[] = {
    '\0', '\0', '\0', '\x1C',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x22',                    // field 4, wire format length-delimited
    '\x18',                    // varint length 24
    '1', '1', '1', '1',        // 4-byte prefixes
    '2', '2', '2', '2',
    '3', '3', '3', '3',
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_FALSE(ParseChunk(kRawAddChunk, sizeof(kRawAddChunk), &chunks));
}

TEST(SafeBrowsingProtocolParsingTest, TestTruncatedFullHashChunk) {
  // This chunk delares there are two full hashes but there is only one.
  const char kRawAddChunk[] = {
    '\0', '\0', '\0', '\x46',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x18',                    // field 3, wire format varint
    '\x01',                    // enum PrefixType == FULL_32B
    '\x22',                    // field 4, wire format length-delimited
    '\x40',                    // varint length 64 (2 full hashes)

    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_FALSE(ParseChunk(kRawAddChunk, sizeof(kRawAddChunk), &chunks));
}

TEST(SafeBrowsingProtocolParsingTest, TestHugeChunk) {
  // This chunk delares there are 6 prefixes but actually only contains 3.
  const char kRawAddChunk[] = {
    '\x1', '\0', '\0', '\0',   // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x22',                    // field 4, wire format length-delimited
    '\x18',                    // varint length 24
    '1', '1', '1', '1',        // 4-byte prefixes
    '2', '2', '2', '2',
    '3', '3', '3', '3',
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_FALSE(ParseChunk(kRawAddChunk, sizeof(kRawAddChunk), &chunks));
}

// Test parsing one sub chunk.
TEST(SafeBrowsingProtocolParsingTest, TestSubChunk) {
  const char kRawSubChunk[] = {
    '\0', '\0', '\0', '\x12',  // 32-bit payload length in network byte order
    '\x08',                    // field 1, wire format varint
    '\x03',                    // chunk_number varint 3
    '\x10',                    // field 2, wire format varint
    '\x01',                    // enum ChunkType == SUB
    '\x22',                    // field 4, wire format length-delimited
    '\x08',                    // varint length 8 (2 prefixes)
    '1', '1', '1', '1',        // 4-byte prefixes
    '2', '2', '2', '2',
    '\x2a',                    // field 5, wire format length-delimited
    '\x02',                    // varint length 2 (2 add-chunk numbers)
    '\x07', '\x09',            // varint 7, varint 9
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_TRUE(ParseChunk(kRawSubChunk, sizeof(kRawSubChunk), &chunks));
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(3, chunks[0]->ChunkNumber());
  EXPECT_FALSE(chunks[0]->IsAdd());
  EXPECT_TRUE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[0]->IsPrefix());
  EXPECT_FALSE(chunks[0]->IsFullHash());
  ASSERT_EQ(2U, chunks[0]->PrefixCount());
  EXPECT_EQ(0x31313131U, chunks[0]->PrefixAt(0));  // 1111
  EXPECT_EQ(7, chunks[0]->AddChunkNumberAt(0));
  EXPECT_EQ(0x32323232U, chunks[0]->PrefixAt(1));  // 2222
  EXPECT_EQ(9, chunks[0]->AddChunkNumberAt(1));
}

// Test parsing one sub chunk with full hashes.
TEST(SafeBrowsingProtocolParsingTest, TestSubFullChunk) {
  const char kRawSubChunk[] = {
    '\0', '\0', '\0', '\x4C',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x02',                    // chunk_number varint 2
    '\x10',                    // field 2, wire format varint
    '\x01',                    // enum ChunkType == SUB
    '\x18',                    // field 3, wire format varint
    '\x01',                    // enum PrefixType == FULL_32B
    '\x22',                    // field 4, wire format length-delimited
    '\x40',                    // varint length 64 (2 full hashes)

    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',
    '0', '1', '0', '1', '0', '1', '0', '1',

    '2', '3', '2', '3', '2', '3', '2', '3',
    '2', '3', '2', '3', '2', '3', '2', '3',
    '2', '3', '2', '3', '2', '3', '2', '3',
    '2', '3', '2', '3', '2', '3', '2', '3',

    '\x2a',                    // field 5, wire format length-delimited
    '\x02',                    // varint length 2 (2 add-chunk numbers)
    '\x07', '\x09',            // varint 7, varint 9
  };

  SBFullHash full_hash1, full_hash2;
  for (int i = 0; i < 32; ++i) {
    full_hash1.full_hash[i] = i % 2 ? '1' : '0';
    full_hash2.full_hash[i] = i % 2 ? '3' : '2';
  }

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_TRUE(ParseChunk(kRawSubChunk, sizeof(kRawSubChunk), &chunks));
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(2, chunks[0]->ChunkNumber());
  EXPECT_FALSE(chunks[0]->IsAdd());
  EXPECT_TRUE(chunks[0]->IsSub());
  EXPECT_FALSE(chunks[0]->IsPrefix());
  EXPECT_TRUE(chunks[0]->IsFullHash());

  ASSERT_EQ(2U, chunks[0]->FullHashCount());
  EXPECT_TRUE(SBFullHashEqual(chunks[0]->FullHashAt(0), full_hash1));
  EXPECT_EQ(7, chunks[0]->AddChunkNumberAt(0));
  EXPECT_TRUE(SBFullHashEqual(chunks[0]->FullHashAt(1), full_hash2));
  EXPECT_EQ(9, chunks[0]->AddChunkNumberAt(1));
}

// Test parsing the SafeBrowsing update response.
TEST(SafeBrowsingProtocolParsingTest, TestChunkDelete) {
  std::string add_del("n:1700\ni:phishy\nad:1-7,43-597,44444,99999\n"
                      "i:malware\nsd:21-27,42,171717\n");

  size_t next_query_sec = 0;
  bool reset = false;
  std::vector<SBChunkDelete> deletes;
  std::vector<ChunkUrl> urls;
  EXPECT_TRUE(ParseUpdate(add_del.data(), add_del.length(),
                                         &next_query_sec, &reset,
                                         &deletes, &urls));

  EXPECT_TRUE(urls.empty());
  EXPECT_FALSE(reset);
  EXPECT_EQ(1700U, next_query_sec);
  ASSERT_EQ(2U, deletes.size());

  ASSERT_EQ(4U, deletes[0].chunk_del.size());
  EXPECT_TRUE(deletes[0].chunk_del[0] == ChunkRange(1, 7));
  EXPECT_TRUE(deletes[0].chunk_del[1] == ChunkRange(43, 597));
  EXPECT_TRUE(deletes[0].chunk_del[2] == ChunkRange(44444));
  EXPECT_TRUE(deletes[0].chunk_del[3] == ChunkRange(99999));

  ASSERT_EQ(3U, deletes[1].chunk_del.size());
  EXPECT_TRUE(deletes[1].chunk_del[0] == ChunkRange(21, 27));
  EXPECT_TRUE(deletes[1].chunk_del[1] == ChunkRange(42));
  EXPECT_TRUE(deletes[1].chunk_del[2] == ChunkRange(171717));

  // An update response with missing list name.
  next_query_sec = 0;
  deletes.clear();
  urls.clear();
  add_del = "n:1700\nad:1-7,43-597,44444,99999\ni:malware\nsd:4,21-27171717\n";
  EXPECT_FALSE(ParseUpdate(add_del.data(), add_del.length(),
                                          &next_query_sec, &reset,
                                          &deletes, &urls));
}

// Test parsing the SafeBrowsing update response.
TEST(SafeBrowsingProtocolParsingTest, TestRedirects) {
  const std::string redirects(base::StringPrintf(
      "i:%s\n"
      "u:cache.googlevideo.com/safebrowsing/rd/goog-malware-shavar_s_1\n"
      "u:cache.googlevideo.com/safebrowsing/rd/goog-malware-shavar_s_2\n"
      "u:cache.googlevideo.com/safebrowsing/rd/goog-malware-shavar_s_3\n"
      "u:s.ytimg.com/safebrowsing/rd/goog-phish-shavar_a_8641-8800:8641-8689,"
      "8691-8731,8733-8786\n",
      kDefaultMalwareList));

  size_t next_query_sec = 0;
  bool reset = false;
  std::vector<SBChunkDelete> deletes;
  std::vector<ChunkUrl> urls;
  EXPECT_TRUE(ParseUpdate(redirects.data(), redirects.length(),
                                         &next_query_sec, &reset,
                                         &deletes, &urls));
  EXPECT_FALSE(reset);
  EXPECT_EQ(0U, next_query_sec);
  EXPECT_TRUE(deletes.empty());

  ASSERT_EQ(4U, urls.size());
  EXPECT_EQ("cache.googlevideo.com/safebrowsing/rd/goog-malware-shavar_s_1",
            urls[0].url);
  EXPECT_EQ("cache.googlevideo.com/safebrowsing/rd/goog-malware-shavar_s_2",
            urls[1].url);
  EXPECT_EQ("cache.googlevideo.com/safebrowsing/rd/goog-malware-shavar_s_3",
            urls[2].url);
  EXPECT_EQ("s.ytimg.com/safebrowsing/rd/goog-phish-shavar_a_8641-8800:"
            "8641-8689,8691-8731,8733-8786",
            urls[3].url);
}

// Test parsing various SafeBrowsing protocol headers.
TEST(SafeBrowsingProtocolParsingTest, TestNextQueryTime) {
  std::string headers("n:1800\ni:goog-white-shavar\n");
  size_t next_query_sec = 0;
  bool reset = false;
  std::vector<SBChunkDelete> deletes;
  std::vector<ChunkUrl> urls;
  EXPECT_TRUE(ParseUpdate(headers.data(), headers.length(),
                                         &next_query_sec, &reset,
                                         &deletes, &urls));

  EXPECT_EQ(1800U, next_query_sec);
  EXPECT_FALSE(reset);
  EXPECT_TRUE(deletes.empty());
  EXPECT_TRUE(urls.empty());
}

// Test parsing data from a GetHashRequest
TEST(SafeBrowsingProtocolParsingTest, TestGetHash) {
  const std::string get_hash(base::StringPrintf(
      "45\n"
      "%s:32:3\n"
      "00112233445566778899aabbccddeeff"
      "00001111222233334444555566667777"
      "ffffeeeeddddccccbbbbaaaa99998888",
      kDefaultPhishList));
  std::vector<SBFullHashResult> full_hashes;
  base::TimeDelta cache_lifetime;
  EXPECT_TRUE(ParseGetHash(get_hash.data(), get_hash.length(), &cache_lifetime,
                           &full_hashes));

  ASSERT_EQ(3U, full_hashes.size());
  EXPECT_EQ(memcmp(&full_hashes[0].hash,
                   "00112233445566778899aabbccddeeff",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(PHISH, full_hashes[0].list_id);
  EXPECT_EQ(memcmp(&full_hashes[1].hash,
                   "00001111222233334444555566667777",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(PHISH, full_hashes[1].list_id);
  EXPECT_EQ(memcmp(&full_hashes[2].hash,
                   "ffffeeeeddddccccbbbbaaaa99998888",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(PHISH, full_hashes[2].list_id);

  // Test multiple lists in the GetHash results.
  const std::string get_hash2(base::StringPrintf(
      "45\n"
      "%s:32:1\n"
      "00112233445566778899aabbccddeeff"
      "%s:32:2\n"
      "cafebeefcafebeefdeaddeaddeaddead"
      "zzzzyyyyxxxxwwwwvvvvuuuuttttssss",
      kDefaultPhishList,
      kDefaultMalwareList));
  EXPECT_TRUE(ParseGetHash(get_hash2.data(), get_hash2.length(),
                           &cache_lifetime, &full_hashes));

  ASSERT_EQ(3U, full_hashes.size());
  EXPECT_EQ(memcmp(&full_hashes[0].hash,
                   "00112233445566778899aabbccddeeff",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(PHISH, full_hashes[0].list_id);
  EXPECT_EQ(memcmp(&full_hashes[1].hash,
                   "cafebeefcafebeefdeaddeaddeaddead",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(MALWARE, full_hashes[1].list_id);
  EXPECT_EQ(memcmp(&full_hashes[2].hash,
                   "zzzzyyyyxxxxwwwwvvvvuuuuttttssss",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(MALWARE, full_hashes[2].list_id);

  // Test metadata parsing. Make some metadata protos to fill in the message.
  MalwarePatternType malware_proto;

  malware_proto.set_pattern_type(MalwarePatternType::LANDING);
  std::string metadata_pb_landing;
  ASSERT_TRUE(malware_proto.SerializeToString(&metadata_pb_landing));

  malware_proto.set_pattern_type(MalwarePatternType::DISTRIBUTION);
  std::string metadata_pb_distribution;
  ASSERT_TRUE(malware_proto.SerializeToString(&metadata_pb_distribution));

  SocialEngineeringPatternType se_proto;
  se_proto.set_pattern_type(
      SocialEngineeringPatternType::SOCIAL_ENGINEERING_ADS);
  std::string metadata_pb_se_ads;
  ASSERT_TRUE(se_proto.SerializeToString(&metadata_pb_se_ads));

  const std::string get_hash3(base::StringPrintf(
      "45\n"
      "%s:32:2:m\n"
      "zzzzyyyyxxxxwwwwvvvvuuuuttttssss"
      "00112233445566778899aabbccddeeff"
      "%zu\n%s"  // meta 1
      "%zu\n%s"  // meta 2
      "%s:32:1:m\n"
      "cafebeefcafebeefdeaddeaddeaddead"
      "%zu\n%s"  // meta 3
      "%s:32:1\n"
      "dafebeefcafebeefdeaddeaddeaddead",
      kDefaultMalwareList, metadata_pb_landing.size(),
      metadata_pb_landing.c_str(), metadata_pb_distribution.size(),
      metadata_pb_distribution.c_str(), kDefaultPhishList,
      metadata_pb_se_ads.size(), metadata_pb_se_ads.c_str(), kDefaultUwSList));
  EXPECT_TRUE(ParseGetHash(get_hash3.data(), get_hash3.length(),
                           &cache_lifetime, &full_hashes));

  ASSERT_EQ(4U, full_hashes.size());
  EXPECT_EQ(memcmp(&full_hashes[0].hash,
                   "zzzzyyyyxxxxwwwwvvvvuuuuttttssss",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(MALWARE, full_hashes[0].list_id);
  EXPECT_EQ(ThreatPatternType::MALWARE_LANDING,
            full_hashes[0].metadata.threat_pattern_type);

  EXPECT_EQ(memcmp(&full_hashes[1].hash,
                   "00112233445566778899aabbccddeeff",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(MALWARE, full_hashes[1].list_id);
  EXPECT_EQ(ThreatPatternType::MALWARE_DISTRIBUTION,
            full_hashes[1].metadata.threat_pattern_type);

  EXPECT_EQ(memcmp(&full_hashes[2].hash,
                   "cafebeefcafebeefdeaddeaddeaddead",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(PHISH, full_hashes[2].list_id);
  EXPECT_EQ(ThreatPatternType::SOCIAL_ENGINEERING_ADS,
            full_hashes[2].metadata.threat_pattern_type);
  EXPECT_EQ(memcmp(&full_hashes[3].hash, "dafebeefcafebeefdeaddeaddeaddead",
                   sizeof(SBFullHash)),
            0);
  EXPECT_EQ(UNWANTEDURL, full_hashes[3].list_id);
  EXPECT_EQ(ThreatPatternType::NONE,
            full_hashes[3].metadata.threat_pattern_type);
}

TEST(SafeBrowsingProtocolParsingTest, TestGetHashWithUnknownList) {
  std::string hash_response(base::StringPrintf(
      "45\n"
      "%s:32:1\n"
      "12345678901234567890123456789012"
      "googpub-phish-shavar:32:1\n"
      "09876543210987654321098765432109",
      kDefaultPhishList));
  std::vector<SBFullHashResult> full_hashes;
  base::TimeDelta cache_lifetime;
  EXPECT_TRUE(ParseGetHash(hash_response.data(), hash_response.size(),
                           &cache_lifetime, &full_hashes));

  ASSERT_EQ(1U, full_hashes.size());
  EXPECT_EQ(memcmp("12345678901234567890123456789012",
                   &full_hashes[0].hash, sizeof(SBFullHash)), 0);
  EXPECT_EQ(PHISH, full_hashes[0].list_id);

  hash_response += base::StringPrintf(
      "%s:32:1\n"
      "abcdefghijklmnopqrstuvwxyz123457",
      kDefaultMalwareList);
  full_hashes.clear();
  EXPECT_TRUE(ParseGetHash(hash_response.data(), hash_response.size(),
                           &cache_lifetime, &full_hashes));

  EXPECT_EQ(2U, full_hashes.size());
  EXPECT_EQ(memcmp("12345678901234567890123456789012",
                   &full_hashes[0].hash, sizeof(SBFullHash)), 0);
  EXPECT_EQ(PHISH, full_hashes[0].list_id);
  EXPECT_EQ(memcmp("abcdefghijklmnopqrstuvwxyz123457",
                   &full_hashes[1].hash, sizeof(SBFullHash)), 0);
  EXPECT_EQ(MALWARE, full_hashes[1].list_id);
}

TEST(SafeBrowsingProtocolParsingTest, TestGetHashWithUnknownListAndMetadata) {
  std::vector<SBFullHashResult> full_hashes;
  base::TimeDelta cache_lifetime;
  // Test skipping over a hashentry with an unrecognized listname that also has
  // metadata.
  const std::string get_hash3(base::StringPrintf(
      "600\n"
      "BADLISTNAME:32:1:m\n"
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
      "8\nMETADATA"  // not even parsed (lest the parser DCHECK's).
      "%s:32:1\n"
      "0123456789hashhashhashhashhashha",
      kDefaultMalwareList));
  EXPECT_TRUE(ParseGetHash(get_hash3.data(), get_hash3.length(),
                           &cache_lifetime, &full_hashes));
  ASSERT_EQ(1U, full_hashes.size());
  EXPECT_EQ(memcmp(&full_hashes[0].hash,
                   "0123456789hashhashhashhashhashha",
                   sizeof(SBFullHash)), 0);
  EXPECT_EQ(MALWARE, full_hashes[0].list_id);
  EXPECT_EQ(ThreatPatternType::NONE,
            full_hashes[0].metadata.threat_pattern_type);
}

TEST(SafeBrowsingProtocolParsingTest, TestFormatHash) {
  std::vector<SBPrefix> prefixes;
  prefixes.push_back(0x34333231);
  prefixes.push_back(0x64636261);
  prefixes.push_back(0x73727170);

  EXPECT_EQ("4:12\n1234abcdpqrs", FormatGetHash(prefixes));
}

TEST(SafeBrowsingProtocolParsingTest, TestReset) {
  std::string update("n:1800\ni:phishy\nr:pleasereset\n");

  bool reset = false;
  size_t next_update = 0;
  std::vector<SBChunkDelete> deletes;
  std::vector<ChunkUrl> urls;
  EXPECT_TRUE(ParseUpdate(update.data(), update.size(),
                                         &next_update, &reset,
                                         &deletes, &urls));
  EXPECT_TRUE(reset);
}

// The SafeBrowsing service will occasionally send zero length chunks so that
// client requests will have longer contiguous chunk number ranges, and thus
// reduce the request size.
TEST(SafeBrowsingProtocolParsingTest, TestZeroSizeAddChunk) {
  const char kEmptyAddChunk[] = {
    '\0', '\0', '\0', '\x02',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x02',                    // chunk_number varint 2
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_TRUE(ParseChunk(kEmptyAddChunk, sizeof(kEmptyAddChunk), &chunks));
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(2, chunks[0]->ChunkNumber());
  EXPECT_TRUE(chunks[0]->IsAdd());
  EXPECT_FALSE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[0]->IsPrefix());
  EXPECT_FALSE(chunks[0]->IsFullHash());
  EXPECT_EQ(0U, chunks[0]->PrefixCount());

  // Now test a zero size chunk in between normal chunks.
  chunks.clear();
  const char kAddChunks[] = {
    '\0', '\0', '\0', '\x0C',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x22',                    // field 4, wire format length-delimited
    '\x08',                    // varint length 8

    '1', '1', '1', '1',        // 4-byte prefixes
    '2', '2', '2', '2',

    '\0', '\0', '\0', '\x02',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x02',                    // chunk_number varint 2

    '\0', '\0', '\0', '\x08',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x03',                    // chunk_number varint 3
    '\x22',                    // field 4, wire format length-delimited
    '\x04',                    // varint length 8
    'p', 'p', 'p', 'p',        // 4-byte prefixes
  };
  EXPECT_TRUE(ParseChunk(kAddChunks, sizeof(kAddChunks), &chunks));
  ASSERT_EQ(3U, chunks.size());

  EXPECT_EQ(1, chunks[0]->ChunkNumber());
  EXPECT_TRUE(chunks[0]->IsAdd());
  EXPECT_FALSE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[0]->IsPrefix());
  EXPECT_FALSE(chunks[0]->IsFullHash());
  ASSERT_EQ(2U, chunks[0]->PrefixCount());
  EXPECT_EQ(0x31313131U, chunks[0]->PrefixAt(0));  // 1111
  EXPECT_EQ(0x32323232U, chunks[0]->PrefixAt(1));  // 2222

  EXPECT_EQ(2, chunks[1]->ChunkNumber());
  EXPECT_TRUE(chunks[1]->IsAdd());
  EXPECT_FALSE(chunks[1]->IsSub());
  EXPECT_TRUE(chunks[1]->IsPrefix());
  EXPECT_FALSE(chunks[1]->IsFullHash());
  EXPECT_EQ(0U, chunks[1]->PrefixCount());

  EXPECT_EQ(3, chunks[2]->ChunkNumber());
  EXPECT_TRUE(chunks[2]->IsAdd());
  EXPECT_FALSE(chunks[2]->IsSub());
  EXPECT_TRUE(chunks[2]->IsPrefix());
  EXPECT_FALSE(chunks[2]->IsFullHash());
  ASSERT_EQ(1U, chunks[2]->PrefixCount());
  EXPECT_EQ(0x70707070U, chunks[2]->PrefixAt(0));  // pppp
}

// Test parsing a zero sized sub chunk.
TEST(SafeBrowsingProtocolParsingTest, TestZeroSizeSubChunk) {
  const char kEmptySubChunk[] = {
    '\0', '\0', '\0', '\x04',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x02',                    // chunk_number varint 2
    '\x10',                    // field 2, wire format varint
    '\x01',                    // enum ChunkType == SUB
  };

  std::vector<std::unique_ptr<SBChunkData>> chunks;
  EXPECT_TRUE(ParseChunk(kEmptySubChunk, sizeof(kEmptySubChunk), &chunks));
  ASSERT_EQ(1U, chunks.size());
  EXPECT_EQ(2, chunks[0]->ChunkNumber());
  EXPECT_FALSE(chunks[0]->IsAdd());
  EXPECT_TRUE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[0]->IsPrefix());
  EXPECT_FALSE(chunks[0]->IsFullHash());
  EXPECT_EQ(0U, chunks[0]->PrefixCount());

  // Test parsing a zero sized sub chunk mixed in with content carrying chunks.
  chunks.clear();
  const char kSubChunks[] = {
    '\0', '\0', '\0', '\x12',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x01',                    // chunk_number varint 1
    '\x10',                    // field 2, wire format varint
    '\x01',                    // enum ChunkType == SUB
    '\x22',                    // field 4, wire format length-delimited
    '\x08',                    // varint length 8
    '1', '1', '1', '1',        // 4-byte prefixes
    '2', '2', '2', '2',
    '\x2a',                    // field 5, wire format length-delimited
    '\x02',                    // varint length 2 (2 add-chunk numbers)
    '\x07', '\x09',            // varint 7, varint 9

    '\0', '\0', '\0', '\x04',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x02',                    // chunk_number varint 2
    '\x10',                    // field 2, wire format varint
    '\x01',                    // enum ChunkType == SUB

    '\0', '\0', '\0', '\x0D',  // 32-bit payload length in network byte order.
    '\x08',                    // field 1, wire format varint
    '\x03',                    // chunk_number varint 3
    '\x10',                    // field 2, wire format varint
    '\x01',                    // enum ChunkType == SUB
    '\x22',                    // field 4, wire format length-delimited
    '\x04',                    // varint length 8
    'p', 'p', 'p', 'p',        // 4-byte prefix
    '\x2a',                    // field 5, wire format length-delimited
    '\x01',                    // varint length 1 (1 add-chunk numbers)
    '\x0B',                    // varint 11
  };

  EXPECT_TRUE(ParseChunk(kSubChunks, sizeof(kSubChunks), &chunks));
  ASSERT_EQ(3U, chunks.size());

  EXPECT_EQ(1, chunks[0]->ChunkNumber());
  EXPECT_FALSE(chunks[0]->IsAdd());
  EXPECT_TRUE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[0]->IsPrefix());
  EXPECT_FALSE(chunks[0]->IsFullHash());
  ASSERT_EQ(2U, chunks[0]->PrefixCount());
  EXPECT_EQ(0x31313131U, chunks[0]->PrefixAt(0));  // 1111
  EXPECT_EQ(7, chunks[0]->AddChunkNumberAt(0));
  EXPECT_EQ(0x32323232U, chunks[0]->PrefixAt(1));  // 2222
  EXPECT_EQ(9, chunks[0]->AddChunkNumberAt(1));

  EXPECT_EQ(2, chunks[1]->ChunkNumber());
  EXPECT_FALSE(chunks[0]->IsAdd());
  EXPECT_TRUE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[1]->IsPrefix());
  EXPECT_FALSE(chunks[1]->IsFullHash());
  EXPECT_EQ(0U, chunks[1]->PrefixCount());

  EXPECT_EQ(3, chunks[2]->ChunkNumber());
  EXPECT_FALSE(chunks[0]->IsAdd());
  EXPECT_TRUE(chunks[0]->IsSub());
  EXPECT_TRUE(chunks[2]->IsPrefix());
  EXPECT_FALSE(chunks[2]->IsFullHash());
  ASSERT_EQ(1U, chunks[2]->PrefixCount());
  EXPECT_EQ(0x70707070U, chunks[2]->PrefixAt(0));  // pppp
  EXPECT_EQ(11, chunks[2]->AddChunkNumberAt(0));
}

}  // namespace safe_browsing
