// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_certificate_chain.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/optional.h"
#include "base/path_service.h"
#include "base/strings/string_piece.h"
#include "components/cbor/cbor_values.h"
#include "components/cbor/cbor_writer.h"
#include "content/public/common/content_paths.h"
#include "net/cert/x509_util.h"
#include "net/test/cert_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

base::Optional<std::vector<base::StringPiece>> GetCertChain(
    const uint8_t* input,
    size_t input_size) {
  return SignedExchangeCertificateChain::GetCertChainFromMessage(
      base::make_span(input, input_size));
}

cbor::CBORValue CBORByteString(base::StringPiece str) {
  return cbor::CBORValue(str, cbor::CBORValue::Type::BYTE_STRING);
}

}  // namespace

TEST(SignedExchangeCertificateParseB0Test, OneCert) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x07, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x00, // extensions size
      // clang-format on
  };
  base::Optional<std::vector<base::StringPiece>> certs =
      GetCertChain(input, arraysize(input));
  ASSERT_TRUE(certs);
  ASSERT_EQ(1u, certs->size());
  const uint8_t kExpected[] = {
      // clang-format off
      0x11, 0x22, // cert data
      // clang-format on
  };
  EXPECT_THAT((*certs)[0],
              testing::ElementsAreArray(kExpected, arraysize(kExpected)));
}

TEST(SignedExchangeCertificateParseB0Test, OneCertWithExtension) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x0A, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x03, // extensions size
      0xE1, 0xE2, 0xE3, // extensions data
      // clang-format on
  };
  base::Optional<std::vector<base::StringPiece>> certs =
      GetCertChain(input, arraysize(input));
  ASSERT_TRUE(certs);
  ASSERT_EQ(1u, certs->size());
  const uint8_t kExpected[] = {
      // clang-format off
      0x11, 0x22, // cert data
      // clang-format on
  };
  EXPECT_THAT((*certs)[0],
              testing::ElementsAreArray(kExpected, arraysize(kExpected)));
}

TEST(SignedExchangeCertificateParseB0Test, TwoCerts) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x01, 0x13, // certificate list size

      0x00, 0x01, 0x04, // cert data size

      // cert data
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,

      0x00, 0x00, // extensions size

      0x00, 0x00, 0x05, // cert data size
      0x33, 0x44, 0x55, 0x66, 0x77, // cert data
      0x00, 0x00, // extensions size

      // clang-format on
  };

  const uint8_t kExpected1[] = {
      // clang-format off
      // cert data
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      // clang-format on
  };
  const uint8_t kExpected2[] = {
      // clang-format off
      0x33, 0x44, 0x55, 0x66, 0x77, // cert data
      // clang-format on
  };

  base::Optional<std::vector<base::StringPiece>> certs =
      GetCertChain(input, sizeof(input));
  ASSERT_TRUE(certs);
  ASSERT_EQ(2u, certs->size());
  EXPECT_THAT((*certs)[0],
              testing::ElementsAreArray(kExpected1, arraysize(kExpected1)));
  EXPECT_THAT((*certs)[1],
              testing::ElementsAreArray(kExpected2, arraysize(kExpected2)));
}

TEST(SignedExchangeCertificateParseB0Test, Empty) {
  EXPECT_FALSE(GetCertChain(nullptr, 0));
}

TEST(SignedExchangeCertificateParseB0Test, InvalidRequestContextSize) {
  const uint8_t input[] = {
      // clang-format off
      0x01, // request context size: must be zero
      0x20, // request context
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB0Test, CanNotReadCertListSize1) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x01, // certificate list size: must be 3 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB0Test, CanNotReadCertListSize2) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x01, // certificate list size: must be 3 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB0Test, CertListSizeError) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x01, 0x01, // certificate list size: 257 (This must be 7)

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x00, // extensions size
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB0Test, CanNotReadCertDataSize) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x02, // certificate list size

      0x00, 0x01, // cert data size: must be 3 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB0Test, CertDataSizeError) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x04, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, // cert data: Need 2 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB0Test, CanNotReadExtensionsSize) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x06, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, // extensions size : must be 2 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB0Test, ExtensionsSizeError) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x07, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x01, // extensions size
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertificateParseB1Test, Empty) {
  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::span<const uint8_t>(), nullptr);
  EXPECT_FALSE(parsed);
}

TEST(SignedExchangeCertificateParseB1Test, EmptyChain) {
  cbor::CBORValue::ArrayValue cbor_array;
  cbor_array.push_back(cbor::CBORValue(u8"\U0001F4DC\u26D3"));

  auto serialized =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_array)));
  ASSERT_TRUE(serialized.has_value());

  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::make_span(*serialized), nullptr);
  EXPECT_FALSE(parsed);
}

TEST(SignedExchangeCertificateParseB1Test, MissingCert) {
  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue("sct")] = CBORByteString("SCT");
  cbor_map[cbor::CBORValue("ocsp")] = CBORByteString("OCSP");

  cbor::CBORValue::ArrayValue cbor_array;
  cbor_array.push_back(cbor::CBORValue(u8"\U0001F4DC\u26D3"));
  cbor_array.push_back(cbor::CBORValue(std::move(cbor_map)));

  auto serialized =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_array)));
  ASSERT_TRUE(serialized.has_value());

  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::make_span(*serialized), nullptr);
  EXPECT_FALSE(parsed);
}

TEST(SignedExchangeCertificateParseB1Test, OneCert) {
  net::CertificateList certs;
  ASSERT_TRUE(
      net::LoadCertificateFiles({"subjectAltName_sanity_check.pem"}, &certs));
  ASSERT_EQ(1U, certs.size());
  base::StringPiece cert_der =
      net::x509_util::CryptoBufferAsStringPiece(certs[0]->cert_buffer());

  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue("sct")] = CBORByteString("SCT");
  cbor_map[cbor::CBORValue("cert")] = CBORByteString(cert_der);
  cbor_map[cbor::CBORValue("ocsp")] = CBORByteString("OCSP");

  cbor::CBORValue::ArrayValue cbor_array;
  cbor_array.push_back(cbor::CBORValue(u8"\U0001F4DC\u26D3"));
  cbor_array.push_back(cbor::CBORValue(std::move(cbor_map)));

  auto serialized =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_array)));
  ASSERT_TRUE(serialized.has_value());

  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::make_span(*serialized), nullptr);
  ASSERT_TRUE(parsed);
  EXPECT_EQ(cert_der, net::x509_util::CryptoBufferAsStringPiece(
                          parsed->cert()->cert_buffer()));
  ASSERT_EQ(0U, parsed->cert()->intermediate_buffers().size());
  EXPECT_EQ(parsed->ocsp(), base::make_optional<std::string>("OCSP"));
  EXPECT_EQ(parsed->sct(), base::make_optional<std::string>("SCT"));
}

TEST(SignedExchangeCertificateParseB1Test, MissingOCSPInFirstCert) {
  net::CertificateList certs;
  ASSERT_TRUE(
      net::LoadCertificateFiles({"subjectAltName_sanity_check.pem"}, &certs));
  ASSERT_EQ(1U, certs.size());
  base::StringPiece cert_der =
      net::x509_util::CryptoBufferAsStringPiece(certs[0]->cert_buffer());

  cbor::CBORValue::MapValue cbor_map;
  cbor_map[cbor::CBORValue("sct")] = CBORByteString("SCT");
  cbor_map[cbor::CBORValue("cert")] = CBORByteString(cert_der);

  cbor::CBORValue::ArrayValue cbor_array;
  cbor_array.push_back(cbor::CBORValue(u8"\U0001F4DC\u26D3"));
  cbor_array.push_back(cbor::CBORValue(std::move(cbor_map)));

  auto serialized =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_array)));
  ASSERT_TRUE(serialized.has_value());

  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::make_span(*serialized), nullptr);
  EXPECT_FALSE(parsed);
}

TEST(SignedExchangeCertificateParseB1Test, TwoCerts) {
  net::CertificateList certs;
  ASSERT_TRUE(net::LoadCertificateFiles(
      {"subjectAltName_sanity_check.pem", "root_ca_cert.pem"}, &certs));
  ASSERT_EQ(2U, certs.size());
  base::StringPiece cert1_der =
      net::x509_util::CryptoBufferAsStringPiece(certs[0]->cert_buffer());
  base::StringPiece cert2_der =
      net::x509_util::CryptoBufferAsStringPiece(certs[1]->cert_buffer());

  cbor::CBORValue::MapValue cbor_map1;
  cbor_map1[cbor::CBORValue("sct")] = CBORByteString("SCT");
  cbor_map1[cbor::CBORValue("cert")] = CBORByteString(cert1_der);
  cbor_map1[cbor::CBORValue("ocsp")] = CBORByteString("OCSP");

  cbor::CBORValue::MapValue cbor_map2;
  cbor_map2[cbor::CBORValue("cert")] = CBORByteString(cert2_der);

  cbor::CBORValue::ArrayValue cbor_array;
  cbor_array.push_back(cbor::CBORValue(u8"\U0001F4DC\u26D3"));
  cbor_array.push_back(cbor::CBORValue(std::move(cbor_map1)));
  cbor_array.push_back(cbor::CBORValue(std::move(cbor_map2)));

  auto serialized =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_array)));
  ASSERT_TRUE(serialized.has_value());

  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::make_span(*serialized), nullptr);
  ASSERT_TRUE(parsed);
  EXPECT_EQ(cert1_der, net::x509_util::CryptoBufferAsStringPiece(
                           parsed->cert()->cert_buffer()));
  ASSERT_EQ(1U, parsed->cert()->intermediate_buffers().size());
  EXPECT_EQ(cert2_der, net::x509_util::CryptoBufferAsStringPiece(
                           parsed->cert()->intermediate_buffers()[0].get()));
  EXPECT_EQ(parsed->ocsp(), base::make_optional<std::string>("OCSP"));
  EXPECT_EQ(parsed->sct(), base::make_optional<std::string>("SCT"));
}

TEST(SignedExchangeCertificateParseB1Test, HavingOCSPInSecnodCert) {
  net::CertificateList certs;
  ASSERT_TRUE(net::LoadCertificateFiles(
      {"subjectAltName_sanity_check.pem", "root_ca_cert.pem"}, &certs));
  ASSERT_EQ(2U, certs.size());
  base::StringPiece cert1_der =
      net::x509_util::CryptoBufferAsStringPiece(certs[0]->cert_buffer());
  base::StringPiece cert2_der =
      net::x509_util::CryptoBufferAsStringPiece(certs[1]->cert_buffer());

  cbor::CBORValue::MapValue cbor_map1;
  cbor_map1[cbor::CBORValue("sct")] = CBORByteString("SCT");
  cbor_map1[cbor::CBORValue("cert")] = CBORByteString(cert1_der);
  cbor_map1[cbor::CBORValue("ocsp")] = CBORByteString("OCSP1");

  cbor::CBORValue::MapValue cbor_map2;
  cbor_map2[cbor::CBORValue("cert")] = CBORByteString(cert2_der);
  cbor_map2[cbor::CBORValue("ocsp")] = CBORByteString("OCSP2");

  cbor::CBORValue::ArrayValue cbor_array;
  cbor_array.push_back(cbor::CBORValue(u8"\U0001F4DC\u26D3"));
  cbor_array.push_back(cbor::CBORValue(std::move(cbor_map1)));
  cbor_array.push_back(cbor::CBORValue(std::move(cbor_map2)));

  auto serialized =
      cbor::CBORWriter::Write(cbor::CBORValue(std::move(cbor_array)));
  ASSERT_TRUE(serialized.has_value());

  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::make_span(*serialized), nullptr);
  EXPECT_FALSE(parsed);
}

TEST(SignedExchangeCertificateParseB1Test, ParseGoldenFile) {
  base::FilePath path;
  base::PathService::Get(content::DIR_TEST_DATA, &path);
  path = path.AppendASCII("htxg").AppendASCII(
      "wildcard_example.org.public.pem.cbor");
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(path, &contents));

  auto parsed = SignedExchangeCertificateChain::Parse(
      SignedExchangeVersion::kB1, base::as_bytes(base::make_span(contents)),
      nullptr);
  ASSERT_TRUE(parsed);
}

}  // namespace content
