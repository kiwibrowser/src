// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_header.h"

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "components/cbor/cbor_values.h"
#include "components/cbor/cbor_writer.h"
#include "content/browser/web_package/signed_exchange_consts.h"
#include "content/public/common/content_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

const char kSignatureString[] =
    "sig1;"
    " sig=*MEUCIQDXlI2gN3RNBlgFiuRNFpZXcDIaUpX6HIEwcZEc0cZYLAIga9DsVOMM+"
    "g5YpwEBdGW3sS+bvnmAJJiSMwhuBdqp5UY;"
    " integrity=\"mi\";"
    " validityUrl=\"https://example.com/resource.validity.1511128380\";"
    " certUrl=\"https://example.com/oldcerts\";"
    " certSha256=*W7uB969dFW3Mb5ZefPS9Tq5ZbH5iSmOILpjv2qEArmI;"
    " date=1511128380; expires=1511733180";

cbor::CBORValue CBORByteString(const char* str) {
  return cbor::CBORValue(str, cbor::CBORValue::Type::BYTE_STRING);
}

base::Optional<SignedExchangeHeader> GenerateHeaderAndParse(
    const std::map<const char*, const char*>& request_map,
    const std::map<const char*, const char*>& response_map) {
  cbor::CBORValue::MapValue request_cbor_map;
  cbor::CBORValue::MapValue response_cbor_map;
  for (auto& pair : request_map)
    request_cbor_map[CBORByteString(pair.first)] = CBORByteString(pair.second);
  for (auto& pair : response_map)
    response_cbor_map[CBORByteString(pair.first)] = CBORByteString(pair.second);

  cbor::CBORValue::ArrayValue array;
  array.push_back(cbor::CBORValue(std::move(request_cbor_map)));
  array.push_back(cbor::CBORValue(std::move(response_cbor_map)));

  auto serialized = cbor::CBORWriter::Write(cbor::CBORValue(std::move(array)));
  return SignedExchangeHeader::Parse(
      base::make_span(serialized->data(), serialized->size()),
      nullptr /* devtools_proxy */);
}

}  // namespace

TEST(SignedExchangeHeaderTest, ParseEncodedLength) {
  constexpr struct {
    uint8_t bytes[SignedExchangeHeader::kEncodedLengthInBytes];
    size_t expected;
  } kTestCases[] = {
      {{0x00, 0x00, 0x01}, 1u}, {{0x01, 0xe2, 0x40}, 123456u},
  };

  int test_element_index = 0;
  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message() << "testing case " << test_element_index++);
    EXPECT_EQ(SignedExchangeHeader::ParseEncodedLength(test_case.bytes),
              test_case.expected);
  }
}

TEST(SignedExchangeHeaderTest, ParseGoldenFile) {
  base::FilePath test_htxg_path;
  base::PathService::Get(content::DIR_TEST_DATA, &test_htxg_path);
  test_htxg_path = test_htxg_path.AppendASCII("htxg").AppendASCII(
      "test.example.org_test.htxg");

  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(test_htxg_path, &contents));
  auto* contents_bytes = reinterpret_cast<const uint8_t*>(contents.data());

  ASSERT_GT(contents.size(), SignedExchangeHeader::kEncodedLengthInBytes);
  size_t header_size = SignedExchangeHeader::ParseEncodedLength(base::make_span(
      contents_bytes, SignedExchangeHeader::kEncodedLengthInBytes));
  ASSERT_GT(contents.size(),
            SignedExchangeHeader::kEncodedLengthInBytes + header_size);

  const auto cbor_bytes = base::make_span<const uint8_t>(
      contents_bytes + SignedExchangeHeader::kEncodedLengthInBytes,
      header_size);
  const base::Optional<SignedExchangeHeader> header =
      SignedExchangeHeader::Parse(cbor_bytes, nullptr /* devtools_proxy */);
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->request_url(), GURL("https://test.example.org/test/"));
  EXPECT_EQ(header->request_method(), "GET");
  EXPECT_EQ(header->response_code(), static_cast<net::HttpStatusCode>(200u));
  EXPECT_EQ(header->response_headers().size(), 4u);
  EXPECT_EQ(header->response_headers().find("content-encoding")->second,
            "mi-sha256");
}

TEST(SignedExchangeHeaderTest, ValidHeader) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "https://test.example.org/test/"}, {kMethodKey, "GET"},
      },
      {
          {kStatusKey, "200"}, {kSignature, kSignatureString},
      });
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->request_url(), GURL("https://test.example.org/test/"));
  EXPECT_EQ(header->request_method(), "GET");
  EXPECT_EQ(header->response_code(), static_cast<net::HttpStatusCode>(200u));
  EXPECT_EQ(header->response_headers().size(), 1u);
}

TEST(SignedExchangeHeaderTest, UnsafeMethod) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "https://test.example.org/test/"}, {kMethodKey, "POST"},
      },
      {
          {kStatusKey, "200"}, {kSignature, kSignatureString},
      });
  ASSERT_FALSE(header.has_value());
}

TEST(SignedExchangeHeaderTest, InvalidURL) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "https:://test.example.org/test/"}, {kMethodKey, "GET"},
      },
      {
          {kStatusKey, "200"}, {kSignature, kSignatureString},
      });
  ASSERT_FALSE(header.has_value());
}

TEST(SignedExchangeHeaderTest, URLWithFragment) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "https://test.example.org/test/#foo"}, {kMethodKey, "GET"},
      },
      {
          {kStatusKey, "200"}, {kSignature, kSignatureString},
      });
  ASSERT_FALSE(header.has_value());
}

TEST(SignedExchangeHeaderTest, RelativeURL) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "test/"}, {kMethodKey, "GET"},
      },
      {
          {kStatusKey, "200"}, {kSignature, kSignatureString},
      });
  ASSERT_FALSE(header.has_value());
}

TEST(SignedExchangeHeaderTest, StatefulRequestHeader) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "https://test.example.org/test/"},
          {kMethodKey, "GET"},
          {"authorization", "Basic Zm9vOmJhcg=="},
      },
      {
          {kStatusKey, "200"}, {kSignature, kSignatureString},
      });
  ASSERT_FALSE(header.has_value());
}

TEST(SignedExchangeHeaderTest, StatefulResponseHeader) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "https://test.example.org/test/"}, {kMethodKey, "GET"},
      },
      {
          {kStatusKey, "200"},
          {kSignature, kSignatureString},
          {"set-cookie", "foo=bar"},
      });
  ASSERT_FALSE(header.has_value());
}

TEST(SignedExchangeHeaderTest, UppercaseRequestMap) {
  auto header = GenerateHeaderAndParse(
      {{kUrlKey, "https://test.example.org/test/"},
       {kMethodKey, "GET"},
       {"Accept-Language", "en-us"}},
      {
          {kStatusKey, "200"}, {kSignature, kSignatureString},
      });
  ASSERT_FALSE(header.has_value());
}

TEST(SignedExchangeHeaderTest, UppercaseResponseMap) {
  auto header = GenerateHeaderAndParse(
      {
          {kUrlKey, "https://test.example.org/test/"}, {kMethodKey, "GET"},
      },
      {{kStatusKey, "200"},
       {kSignature, kSignatureString},
       {"Content-Length", "123"}});
  ASSERT_FALSE(header.has_value());
}

}  // namespace content
