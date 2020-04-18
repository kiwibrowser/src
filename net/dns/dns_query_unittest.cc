// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_query.h"

#include "net/base/io_buffer.h"
#include "net/dns/dns_protocol.h"
#include "net/dns/record_rdata.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

using ::testing::ElementsAreArray;

std::tuple<char*, size_t> AsTuple(const IOBufferWithSize* buf) {
  return std::make_tuple(buf->data(), buf->size());
}

TEST(DnsQueryTest, Constructor) {
  // This includes \0 at the end.
  const char qname_data[] = "\x03""www""\x07""example""\x03""com";
  const uint8_t query_data[] = {
      // Header
      0xbe, 0xef, 0x01, 0x00,  // Flags -- set RD (recursion desired) bit.
      0x00, 0x01,              // Set QDCOUNT (question count) to 1, all the
                               // rest are 0 for a query.
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      // Question
      0x03, 'w', 'w', 'w',  // QNAME: www.example.com in DNS format.
      0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 0x03, 'c', 'o', 'm', 0x00,

      0x00, 0x01,  // QTYPE: A query.
      0x00, 0x01,  // QCLASS: IN class.
  };

  base::StringPiece qname(qname_data, sizeof(qname_data));
  DnsQuery q1(0xbeef, qname, dns_protocol::kTypeA);
  EXPECT_EQ(dns_protocol::kTypeA, q1.qtype());
  EXPECT_THAT(AsTuple(q1.io_buffer()), ElementsAreArray(query_data));
  EXPECT_EQ(qname, q1.qname());

  base::StringPiece question(reinterpret_cast<const char*>(query_data) + 12,
                             21);
  EXPECT_EQ(question, q1.question());
}

TEST(DnsQueryTest, Clone) {
  // This includes \0 at the end.
  const char qname_data[] = "\x03""www""\x07""example""\x03""com";
  base::StringPiece qname(qname_data, sizeof(qname_data));

  DnsQuery q1(0, qname, dns_protocol::kTypeA);
  EXPECT_EQ(0, q1.id());
  std::unique_ptr<DnsQuery> q2 = q1.CloneWithNewId(42);
  EXPECT_EQ(42, q2->id());
  EXPECT_EQ(q1.io_buffer()->size(), q2->io_buffer()->size());
  EXPECT_EQ(q1.qtype(), q2->qtype());
  EXPECT_EQ(q1.question(), q2->question());
}

TEST(DnsQueryTest, EDNS0) {
  // This includes \0 at the end.
  const char qname_data[] =
      "\x03"
      "www"
      "\x07"
      "example"
      "\x03"
      "com";
  const uint8_t query_data[] = {
      // Header
      0xbe, 0xef, 0x01, 0x00,  // Flags -- set RD (recursion desired) bit.
      // Set QDCOUNT (question count) and ARCOUNT (additional count) to 1, all
      // the rest are 0 for a query.
      0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      // Question
      0x03, 'w', 'w', 'w',  // QNAME: www.example.com in DNS format.
      0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 0x03, 'c', 'o', 'm', 0x00,

      0x00, 0x01,  // QTYPE: A query.
      0x00, 0x01,  // QCLASS: IN class.

      // Additional
      0x00,                    // QNAME: empty (root domain)
      0x00, 0x29,              // TYPE: OPT
      0x10, 0x00,              // CLASS: max UDP payload size
      0x00, 0x00, 0x00, 0x00,  // TTL: rcode, version and flags
      0x00, 0x08,              // RDATA length
      0x00, 0xFF,              // OPT code
      0x00, 0x04,              // OPT data size
      0xDE, 0xAD, 0xBE, 0xEF   // OPT data
  };

  base::StringPiece qname(qname_data, sizeof(qname_data));
  OptRecordRdata opt_rdata;
  opt_rdata.AddOpt(OptRecordRdata::Opt(255, "\xde\xad\xbe\xef"));
  DnsQuery q1(0xbeef, qname, dns_protocol::kTypeA, &opt_rdata);
  EXPECT_EQ(dns_protocol::kTypeA, q1.qtype());

  EXPECT_THAT(AsTuple(q1.io_buffer()), ElementsAreArray(query_data));

  base::StringPiece question(reinterpret_cast<const char*>(query_data) + 12,
                             21);
  EXPECT_EQ(question, q1.question());
}

}  // namespace

}  // namespace net
