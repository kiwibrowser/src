// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_response.h"

#include "base/time/time.h"
#include "net/base/address_list.h"
#include "net/base/io_buffer.h"
#include "net/dns/dns_protocol.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

TEST(DnsRecordParserTest, Constructor) {
  const char data[] = { 0 };

  EXPECT_FALSE(DnsRecordParser().IsValid());
  EXPECT_TRUE(DnsRecordParser(data, 1, 0).IsValid());
  EXPECT_TRUE(DnsRecordParser(data, 1, 1).IsValid());

  EXPECT_FALSE(DnsRecordParser(data, 1, 0).AtEnd());
  EXPECT_TRUE(DnsRecordParser(data, 1, 1).AtEnd());
}

TEST(DnsRecordParserTest, ReadName) {
  const uint8_t data[] = {
      // all labels "foo.example.com"
      0x03, 'f', 'o', 'o', 0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 0x03, 'c',
      'o', 'm',
      // byte 0x10
      0x00,
      // byte 0x11
      // part label, part pointer, "bar.example.com"
      0x03, 'b', 'a', 'r', 0xc0, 0x04,
      // byte 0x17
      // all pointer to "bar.example.com", 2 jumps
      0xc0, 0x11,
      // byte 0x1a
  };

  std::string out;
  DnsRecordParser parser(data, sizeof(data), 0);
  ASSERT_TRUE(parser.IsValid());

  EXPECT_EQ(0x11u, parser.ReadName(data + 0x00, &out));
  EXPECT_EQ("foo.example.com", out);
  // Check that the last "." is never stored.
  out.clear();
  EXPECT_EQ(0x1u, parser.ReadName(data + 0x10, &out));
  EXPECT_EQ("", out);
  out.clear();
  EXPECT_EQ(0x6u, parser.ReadName(data + 0x11, &out));
  EXPECT_EQ("bar.example.com", out);
  out.clear();
  EXPECT_EQ(0x2u, parser.ReadName(data + 0x17, &out));
  EXPECT_EQ("bar.example.com", out);

  // Parse name without storing it.
  EXPECT_EQ(0x11u, parser.ReadName(data + 0x00, NULL));
  EXPECT_EQ(0x1u, parser.ReadName(data + 0x10, NULL));
  EXPECT_EQ(0x6u, parser.ReadName(data + 0x11, NULL));
  EXPECT_EQ(0x2u, parser.ReadName(data + 0x17, NULL));

  // Check that it works even if initial position is different.
  parser = DnsRecordParser(data, sizeof(data), 0x12);
  EXPECT_EQ(0x6u, parser.ReadName(data + 0x11, NULL));
}

TEST(DnsRecordParserTest, ReadNameFail) {
  const uint8_t data[] = {
      // label length beyond packet
      0x30, 'x', 'x', 0x00,
      // pointer offset beyond packet
      0xc0, 0x20,
      // pointer loop
      0xc0, 0x08, 0xc0, 0x06,
      // incorrect label type (currently supports only direct and pointer)
      0x80, 0x00,
      // truncated name (missing root label)
      0x02, 'x', 'x',
  };

  DnsRecordParser parser(data, sizeof(data), 0);
  ASSERT_TRUE(parser.IsValid());

  std::string out;
  EXPECT_EQ(0u, parser.ReadName(data + 0x00, &out));
  EXPECT_EQ(0u, parser.ReadName(data + 0x04, &out));
  EXPECT_EQ(0u, parser.ReadName(data + 0x08, &out));
  EXPECT_EQ(0u, parser.ReadName(data + 0x0a, &out));
  EXPECT_EQ(0u, parser.ReadName(data + 0x0c, &out));
  EXPECT_EQ(0u, parser.ReadName(data + 0x0e, &out));
}

TEST(DnsRecordParserTest, ReadRecord) {
  const uint8_t data[] = {
      // Type CNAME record.
      0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 0x03, 'c', 'o', 'm', 0x00, 0x00,
      0x05,                    // TYPE is CNAME.
      0x00, 0x01,              // CLASS is IN.
      0x00, 0x01, 0x24, 0x74,  // TTL is 0x00012474.
      0x00, 0x06,              // RDLENGTH is 6 bytes.
      0x03, 'f', 'o', 'o',     // compressed name in record
      0xc0, 0x00,
      // Type A record.
      0x03, 'b', 'a', 'r',     // compressed owner name
      0xc0, 0x00, 0x00, 0x01,  // TYPE is A.
      0x00, 0x01,              // CLASS is IN.
      0x00, 0x20, 0x13, 0x55,  // TTL is 0x00201355.
      0x00, 0x04,              // RDLENGTH is 4 bytes.
      0x7f, 0x02, 0x04, 0x01,  // IP is 127.2.4.1
  };

  std::string out;
  DnsRecordParser parser(data, sizeof(data), 0);

  DnsResourceRecord record;
  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_EQ("example.com", record.name);
  EXPECT_EQ(dns_protocol::kTypeCNAME, record.type);
  EXPECT_EQ(dns_protocol::kClassIN, record.klass);
  EXPECT_EQ(0x00012474u, record.ttl);
  EXPECT_EQ(6u, record.rdata.length());
  EXPECT_EQ(6u, parser.ReadName(record.rdata.data(), &out));
  EXPECT_EQ("foo.example.com", out);
  EXPECT_FALSE(parser.AtEnd());

  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_EQ("bar.example.com", record.name);
  EXPECT_EQ(dns_protocol::kTypeA, record.type);
  EXPECT_EQ(dns_protocol::kClassIN, record.klass);
  EXPECT_EQ(0x00201355u, record.ttl);
  EXPECT_EQ(4u, record.rdata.length());
  EXPECT_EQ(base::StringPiece("\x7f\x02\x04\x01"), record.rdata);
  EXPECT_TRUE(parser.AtEnd());

  // Test truncated record.
  parser = DnsRecordParser(data, sizeof(data) - 2, 0);
  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_FALSE(parser.AtEnd());
  EXPECT_FALSE(parser.ReadRecord(&record));
}

TEST(DnsResponseTest, InitParse) {
  // This includes \0 at the end.
  const char qname_data[] = "\x0A""codereview""\x08""chromium""\x03""org";
  const base::StringPiece qname(qname_data, sizeof(qname_data));
  // Compilers want to copy when binding temporary to const &, so must use heap.
  std::unique_ptr<DnsQuery> query(
      new DnsQuery(0xcafe, qname, dns_protocol::kTypeA));

  const uint8_t response_data[] = {
      // Header
      0xca, 0xfe,  // ID
      0x81, 0x80,  // Standard query response, RA, no error
      0x00, 0x01,  // 1 question
      0x00, 0x02,  // 2 RRs (answers)
      0x00, 0x00,  // 0 authority RRs
      0x00, 0x01,  // 1 additional RRs

      // Question
      // This part is echoed back from the respective query.
      0x0a, 'c', 'o', 'd', 'e', 'r', 'e', 'v', 'i', 'e', 'w', 0x08, 'c', 'h',
      'r', 'o', 'm', 'i', 'u', 'm', 0x03, 'o', 'r', 'g', 0x00, 0x00,
      0x01,        // TYPE is A.
      0x00, 0x01,  // CLASS is IN.

      // Answer 1
      0xc0, 0x0c,  // NAME is a pointer to name in Question section.
      0x00, 0x05,  // TYPE is CNAME.
      0x00, 0x01,  // CLASS is IN.
      0x00, 0x01,  // TTL (4 bytes) is 20 hours, 47 minutes, 48 seconds.
      0x24, 0x74, 0x00, 0x12,  // RDLENGTH is 18 bytes.
      // ghs.l.google.com in DNS format.
      0x03, 'g', 'h', 's', 0x01, 'l', 0x06, 'g', 'o', 'o', 'g', 'l', 'e', 0x03,
      'c', 'o', 'm', 0x00,

      // Answer 2
      0xc0, 0x35,              // NAME is a pointer to name in Answer 1.
      0x00, 0x01,              // TYPE is A.
      0x00, 0x01,              // CLASS is IN.
      0x00, 0x00,              // TTL (4 bytes) is 53 seconds.
      0x00, 0x35, 0x00, 0x04,  // RDLENGTH is 4 bytes.
      0x4a, 0x7d,              // RDATA is the IP: 74.125.95.121
      0x5f, 0x79,

      // Additional 1
      0x00,                    // NAME is empty (root domain).
      0x00, 0x29,              // TYPE is OPT.
      0x10, 0x00,              // CLASS is max UDP payload size (4096).
      0x00, 0x00, 0x00, 0x00,  // TTL (4 bytes) is rcode, version and flags.
      0x00, 0x08,              // RDLENGTH
      0x00, 0xFF,              // OPT code
      0x00, 0x04,              // OPT data size
      0xDE, 0xAD, 0xBE, 0xEF   // OPT data
  };

  DnsResponse resp;
  memcpy(resp.io_buffer()->data(), response_data, sizeof(response_data));

  // Reject too short.
  EXPECT_FALSE(resp.InitParse(query->io_buffer()->size() - 1, *query));
  EXPECT_FALSE(resp.IsValid());

  // Reject wrong id.
  std::unique_ptr<DnsQuery> other_query = query->CloneWithNewId(0xbeef);
  EXPECT_FALSE(resp.InitParse(sizeof(response_data), *other_query));
  EXPECT_FALSE(resp.IsValid());

  // Reject wrong question.
  std::unique_ptr<DnsQuery> wrong_query(
      new DnsQuery(0xcafe, qname, dns_protocol::kTypeCNAME));
  EXPECT_FALSE(resp.InitParse(sizeof(response_data), *wrong_query));
  EXPECT_FALSE(resp.IsValid());

  // Accept matching question.
  EXPECT_TRUE(resp.InitParse(sizeof(response_data), *query));
  EXPECT_TRUE(resp.IsValid());

  // Check header access.
  EXPECT_EQ(0x8180, resp.flags());
  EXPECT_EQ(0x0, resp.rcode());
  EXPECT_EQ(2u, resp.answer_count());
  EXPECT_EQ(1u, resp.additional_answer_count());

  // Check question access.
  EXPECT_EQ(query->qname(), resp.qname());
  EXPECT_EQ(query->qtype(), resp.qtype());
  EXPECT_EQ("codereview.chromium.org", resp.GetDottedName());

  DnsResourceRecord record;
  DnsRecordParser parser = resp.Parser();
  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_FALSE(parser.AtEnd());
  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_FALSE(parser.AtEnd());
  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_TRUE(parser.AtEnd());
  EXPECT_FALSE(parser.ReadRecord(&record));
}

TEST(DnsResponseTest, InitParseWithoutQuery) {
  DnsResponse resp;
  memcpy(resp.io_buffer()->data(), kT0ResponseDatagram,
         sizeof(kT0ResponseDatagram));

  // Accept matching question.
  EXPECT_TRUE(resp.InitParseWithoutQuery(sizeof(kT0ResponseDatagram)));
  EXPECT_TRUE(resp.IsValid());

  // Check header access.
  EXPECT_EQ(0x8180, resp.flags());
  EXPECT_EQ(0x0, resp.rcode());
  EXPECT_EQ(kT0RecordCount, resp.answer_count());

  // Check question access.
  EXPECT_EQ(kT0Qtype, resp.qtype());
  EXPECT_EQ(kT0HostName, resp.GetDottedName());

  DnsResourceRecord record;
  DnsRecordParser parser = resp.Parser();
  for (unsigned i = 0; i < kT0RecordCount; i ++) {
    EXPECT_FALSE(parser.AtEnd());
    EXPECT_TRUE(parser.ReadRecord(&record));
  }
  EXPECT_TRUE(parser.AtEnd());
  EXPECT_FALSE(parser.ReadRecord(&record));
}

TEST(DnsResponseTest, InitParseWithoutQueryNoQuestions) {
  const uint8_t response_data[] = {
      // Header
      0xca, 0xfe,  // ID
      0x81, 0x80,  // Standard query response, RA, no error
      0x00, 0x00,  // No question
      0x00, 0x01,  // 2 RRs (answers)
      0x00, 0x00,  // 0 authority RRs
      0x00, 0x00,  // 0 additional RRs

      // Answer 1
      0x0a, 'c', 'o', 'd', 'e', 'r', 'e', 'v', 'i', 'e', 'w', 0x08, 'c', 'h',
      'r', 'o', 'm', 'i', 'u', 'm', 0x03, 'o', 'r', 'g', 0x00, 0x00,
      0x01,                    // TYPE is A.
      0x00, 0x01,              // CLASS is IN.
      0x00, 0x00,              // TTL (4 bytes) is 53 seconds.
      0x00, 0x35, 0x00, 0x04,  // RDLENGTH is 4 bytes.
      0x4a, 0x7d,              // RDATA is the IP: 74.125.95.121
      0x5f, 0x79,
  };

  DnsResponse resp;
  memcpy(resp.io_buffer()->data(), response_data, sizeof(response_data));

  EXPECT_TRUE(resp.InitParseWithoutQuery(sizeof(response_data)));

  // Check header access.
  EXPECT_EQ(0x8180, resp.flags());
  EXPECT_EQ(0x0, resp.rcode());
  EXPECT_EQ(0x1u, resp.answer_count());

  DnsResourceRecord record;
  DnsRecordParser parser = resp.Parser();

  EXPECT_FALSE(parser.AtEnd());
  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_EQ("codereview.chromium.org", record.name);
  EXPECT_EQ(0x00000035u, record.ttl);
  EXPECT_EQ(dns_protocol::kTypeA, record.type);

  EXPECT_TRUE(parser.AtEnd());
  EXPECT_FALSE(parser.ReadRecord(&record));
}

TEST(DnsResponseTest, InitParseWithoutQueryTwoQuestions) {
  const uint8_t response_data[] = {
      // Header
      0xca, 0xfe,  // ID
      0x81, 0x80,  // Standard query response, RA, no error
      0x00, 0x02,  // 2 questions
      0x00, 0x01,  // 2 RRs (answers)
      0x00, 0x00,  // 0 authority RRs
      0x00, 0x00,  // 0 additional RRs

      // Question 1
      0x0a, 'c', 'o', 'd', 'e', 'r', 'e', 'v', 'i', 'e', 'w', 0x08, 'c', 'h',
      'r', 'o', 'm', 'i', 'u', 'm', 0x03, 'o', 'r', 'g', 0x00, 0x00,
      0x01,        // TYPE is A.
      0x00, 0x01,  // CLASS is IN.

      // Question 2
      0x0b, 'c', 'o', 'd', 'e', 'r', 'e', 'v', 'i', 'e', 'w', '2', 0xc0,
      0x18,        // pointer to "chromium.org"
      0x00, 0x01,  // TYPE is A.
      0x00, 0x01,  // CLASS is IN.

      // Answer 1
      0xc0, 0x0c,              // NAME is a pointer to name in Question section.
      0x00, 0x01,              // TYPE is A.
      0x00, 0x01,              // CLASS is IN.
      0x00, 0x00,              // TTL (4 bytes) is 53 seconds.
      0x00, 0x35, 0x00, 0x04,  // RDLENGTH is 4 bytes.
      0x4a, 0x7d,              // RDATA is the IP: 74.125.95.121
      0x5f, 0x79,
  };

  DnsResponse resp;
  memcpy(resp.io_buffer()->data(), response_data, sizeof(response_data));

  EXPECT_TRUE(resp.InitParseWithoutQuery(sizeof(response_data)));

  // Check header access.
  EXPECT_EQ(0x8180, resp.flags());
  EXPECT_EQ(0x0, resp.rcode());
  EXPECT_EQ(0x01u, resp.answer_count());

  DnsResourceRecord record;
  DnsRecordParser parser = resp.Parser();

  EXPECT_FALSE(parser.AtEnd());
  EXPECT_TRUE(parser.ReadRecord(&record));
  EXPECT_EQ("codereview.chromium.org", record.name);
  EXPECT_EQ(0x35u, record.ttl);
  EXPECT_EQ(dns_protocol::kTypeA, record.type);

  EXPECT_TRUE(parser.AtEnd());
  EXPECT_FALSE(parser.ReadRecord(&record));
}

TEST(DnsResponseTest, InitParseWithoutQueryPacketTooShort) {
  const uint8_t response_data[] = {
      // Header
      0xca, 0xfe,  // ID
      0x81, 0x80,  // Standard query response, RA, no error
      0x00, 0x00,  // No question
  };

  DnsResponse resp;
  memcpy(resp.io_buffer()->data(), response_data, sizeof(response_data));

  EXPECT_FALSE(resp.InitParseWithoutQuery(sizeof(response_data)));
}

void VerifyAddressList(const std::vector<const char*>& ip_addresses,
                       const AddressList& addrlist) {
  ASSERT_EQ(ip_addresses.size(), addrlist.size());

  for (size_t i = 0; i < addrlist.size(); ++i) {
    EXPECT_EQ(ip_addresses[i], addrlist[i].ToStringWithoutPort());
  }
}

TEST(DnsResponseTest, ParseToAddressList) {
  const struct TestCase {
    size_t query_size;
    const uint8_t* response_data;
    size_t response_size;
    const char* const* expected_addresses;
    size_t num_expected_addresses;
    const char* expected_cname;
    int expected_ttl_sec;
  } cases[] = {
      {
        kT0QuerySize,
        kT0ResponseDatagram, arraysize(kT0ResponseDatagram),
        kT0IpAddresses, arraysize(kT0IpAddresses),
        kT0CanonName,
        kT0TTL,
      },
      {
        kT1QuerySize,
        kT1ResponseDatagram, arraysize(kT1ResponseDatagram),
        kT1IpAddresses, arraysize(kT1IpAddresses),
        kT1CanonName,
        kT1TTL,
      },
      {
        kT2QuerySize,
        kT2ResponseDatagram, arraysize(kT2ResponseDatagram),
        kT2IpAddresses, arraysize(kT2IpAddresses),
        kT2CanonName,
        kT2TTL,
      },
      {
        kT3QuerySize,
        kT3ResponseDatagram, arraysize(kT3ResponseDatagram),
        kT3IpAddresses, arraysize(kT3IpAddresses),
        kT3CanonName,
        kT3TTL,
      },
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    const TestCase& t = cases[i];
    DnsResponse response(t.response_data, t.response_size, t.query_size);
    AddressList addr_list;
    base::TimeDelta ttl;
    EXPECT_EQ(DnsResponse::DNS_PARSE_OK,
              response.ParseToAddressList(&addr_list, &ttl));
    std::vector<const char*> expected_addresses(
        t.expected_addresses,
        t.expected_addresses + t.num_expected_addresses);
    VerifyAddressList(expected_addresses, addr_list);
    EXPECT_EQ(t.expected_cname, addr_list.canonical_name());
    EXPECT_EQ(base::TimeDelta::FromSeconds(t.expected_ttl_sec), ttl);
  }
}

const uint8_t kResponseTruncatedRecord[] = {
    // Header: 1 question, 1 answer RR
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    // Question: name = 'a', type = A (0x1)
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01,
    // Answer: name = 'a', type = A, TTL = 0xFF, RDATA = 10.10.10.10
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04,
    0x0A, 0x0A, 0x0A,  // Truncated RDATA.
};

const uint8_t kResponseTruncatedCNAME[] = {
    // Header: 1 question, 1 answer RR
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    // Question: name = 'a', type = A (0x1)
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01,
    // Answer: name = 'a', type = CNAME, TTL = 0xFF, RDATA = 'foo' (truncated)
    0x01, 'a', 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x03,
    0x03, 'f', 'o',  // Truncated name.
};

const uint8_t kResponseNameMismatch[] = {
    // Header: 1 question, 1 answer RR
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    // Question: name = 'a', type = A (0x1)
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01,
    // Answer: name = 'b', type = A, TTL = 0xFF, RDATA = 10.10.10.10
    0x01, 'b', 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04,
    0x0A, 0x0A, 0x0A, 0x0A,
};

const uint8_t kResponseNameMismatchInChain[] = {
    // Header: 1 question, 3 answer RR
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
    // Question: name = 'a', type = A (0x1)
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01,
    // Answer: name = 'a', type = CNAME, TTL = 0xFF, RDATA = 'b'
    0x01, 'a', 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x03,
    0x01, 'b', 0x00,
    // Answer: name = 'b', type = A, TTL = 0xFF, RDATA = 10.10.10.10
    0x01, 'b', 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04,
    0x0A, 0x0A, 0x0A, 0x0A,
    // Answer: name = 'c', type = A, TTL = 0xFF, RDATA = 10.10.10.11
    0x01, 'c', 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04,
    0x0A, 0x0A, 0x0A, 0x0B,
};

const uint8_t kResponseSizeMismatch[] = {
    // Header: 1 answer RR
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    // Question: name = 'a', type = AAAA (0x1c)
    0x01, 'a', 0x00, 0x00, 0x1c, 0x00, 0x01,
    // Answer: name = 'a', type = AAAA, TTL = 0xFF, RDATA = 10.10.10.10
    0x01, 'a', 0x00, 0x00, 0x1c, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04,
    0x0A, 0x0A, 0x0A, 0x0A,
};

const uint8_t kResponseCNAMEAfterAddress[] = {
    // Header: 2 answer RR
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    // Question: name = 'a', type = A (0x1)
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01,
    // Answer: name = 'a', type = A, TTL = 0xFF, RDATA = 10.10.10.10.
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04,
    0x0A, 0x0A, 0x0A, 0x0A,
    // Answer: name = 'a', type = CNAME, TTL = 0xFF, RDATA = 'b'
    0x01, 'a', 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x03,
    0x01, 'b', 0x00,
};

const uint8_t kResponseNoAddresses[] = {
    // Header: 1 question, 1 answer RR, 1 authority RR
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    // Question: name = 'a', type = A (0x1)
    0x01, 'a', 0x00, 0x00, 0x01, 0x00, 0x01,
    // Answer: name = 'a', type = CNAME, TTL = 0xFF, RDATA = 'b'
    0x01, 'a', 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x03,
    0x01, 'b', 0x00,
    // Authority section
    // Answer: name = 'b', type = A, TTL = 0xFF, RDATA = 10.10.10.10
    0x01, 'b', 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x04,
    0x0A, 0x0A, 0x0A, 0x0A,
};

TEST(DnsResponseTest, ParseToAddressListFail) {
  const struct TestCase {
    const uint8_t* data;
    size_t size;
    DnsResponse::Result expected_result;
  } cases[] = {
    { kResponseTruncatedRecord, arraysize(kResponseTruncatedRecord),
      DnsResponse::DNS_MALFORMED_RESPONSE },
    { kResponseTruncatedCNAME, arraysize(kResponseTruncatedCNAME),
      DnsResponse::DNS_MALFORMED_CNAME },
    { kResponseNameMismatch, arraysize(kResponseNameMismatch),
      DnsResponse::DNS_NAME_MISMATCH },
    { kResponseNameMismatchInChain, arraysize(kResponseNameMismatchInChain),
      DnsResponse::DNS_NAME_MISMATCH },
    { kResponseSizeMismatch, arraysize(kResponseSizeMismatch),
      DnsResponse::DNS_SIZE_MISMATCH },
    { kResponseCNAMEAfterAddress, arraysize(kResponseCNAMEAfterAddress),
      DnsResponse::DNS_CNAME_AFTER_ADDRESS },
    // Not actually a failure, just an empty result.
    { kResponseNoAddresses, arraysize(kResponseNoAddresses),
      DnsResponse::DNS_PARSE_OK },
  };

  const size_t kQuerySize = 12 + 7;

  for (size_t i = 0; i < arraysize(cases); ++i) {
    const TestCase& t = cases[i];

    DnsResponse response(t.data, t.size, kQuerySize);
    AddressList addr_list;
    base::TimeDelta ttl;
    EXPECT_EQ(t.expected_result,
              response.ParseToAddressList(&addr_list, &ttl));
  }
}

}  // namespace

}  // namespace net
