// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/platform/impl/quic_url_utils_impl.h"

#include <cstdint>

#include "net/third_party/quic/platform/api/quic_arraysize.h"
#include "net/third_party/quic/platform/api/quic_test.h"

using std::string;

namespace net {
namespace test {
namespace {

using QuicUrlUtilsImplTest = QuicTest;

TEST_F(QuicUrlUtilsImplTest, GetPushPromiseUrl) {
  // Test rejection of various inputs.
  EXPECT_EQ("", QuicUrlUtilsImpl::GetPushPromiseUrl("file", "localhost",
                                                    "/etc/password"));
  EXPECT_EQ("", QuicUrlUtilsImpl::GetPushPromiseUrl(
                    "file", "", "/C:/Windows/System32/Config/"));
  EXPECT_EQ("", QuicUrlUtilsImpl::GetPushPromiseUrl(
                    "", "https://www.google.com", "/"));

  EXPECT_EQ("", QuicUrlUtilsImpl::GetPushPromiseUrl("https://www.google.com",
                                                    "www.google.com", "/"));
  EXPECT_EQ("", QuicUrlUtilsImpl::GetPushPromiseUrl("https://",
                                                    "www.google.com", "/"));
  EXPECT_EQ("", QuicUrlUtilsImpl::GetPushPromiseUrl("https", "", "/"));
  EXPECT_EQ(
      "", QuicUrlUtilsImpl::GetPushPromiseUrl("https", "", "www.google.com/"));
  EXPECT_EQ(
      "", QuicUrlUtilsImpl::GetPushPromiseUrl("https", "www.google.com/", "/"));
  EXPECT_EQ("",
            QuicUrlUtilsImpl::GetPushPromiseUrl("https", "www.google.com", ""));
  EXPECT_EQ(
      "", QuicUrlUtilsImpl::GetPushPromiseUrl("https", "www.google", ".com/"));

  // Test acception/rejection of various input combinations.
  // |input_headers| is an array of pairs. The first value of each pair is a
  // string that will be used as one of the inputs of GetPushPromiseUrl(). The
  // second value of each pair is a bitfield where the lowest 3 bits indicate
  // for which headers that string is valid (in a PUSH_PROMISE). For example,
  // the string "http" would be valid for both the ":scheme" and ":authority"
  // headers, so the bitfield paired with it is set to SCHEME | AUTH.
  const unsigned char SCHEME = (1u << 0);
  const unsigned char AUTH = (1u << 1);
  const unsigned char PATH = (1u << 2);
  const std::pair<const char*, unsigned char> input_headers[] = {
      {"http", SCHEME | AUTH},
      {"https", SCHEME | AUTH},
      {"hTtP", SCHEME | AUTH},
      {"HTTPS", SCHEME | AUTH},
      {"www.google.com", AUTH},
      {"90af90e0", AUTH},
      {"12foo%20-bar:00001233", AUTH},
      {"GOO\u200b\u2060\ufeffgoo", AUTH},
      {"192.168.0.5", AUTH},
      {"[::ffff:192.168.0.1.]", AUTH},
      {"http:", AUTH},
      {"bife l", AUTH},
      {"/", PATH},
      {"/foo/bar/baz", PATH},
      {"/%20-2DVdkj.cie/foe_.iif/", PATH},
      {"http://", 0},
      {":443", 0},
      {":80/eddd", 0},
      {"google.com:-0", 0},
      {"google.com:65536", 0},
      {"http://google.com", 0},
      {"http://google.com:39", 0},
      {"//google.com/foo", 0},
      {".com/", 0},
      {"http://www.google.com/", 0},
      {"http://foo:439", 0},
      {"[::ffff:192.168", 0},
      {"]/", 0},
      {"//", 0}};
  for (size_t i = 0; i < QUIC_ARRAYSIZE(input_headers); ++i) {
    bool should_accept = (input_headers[i].second & SCHEME);
    for (size_t j = 0; j < QUIC_ARRAYSIZE(input_headers); ++j) {
      bool should_accept_2 = should_accept && (input_headers[j].second & AUTH);
      for (size_t k = 0; k < QUIC_ARRAYSIZE(input_headers); ++k) {
        // |should_accept_3| indicates whether or not GetPushPromiseUrl() is
        // expected to accept this input combination.
        bool should_accept_3 =
            should_accept_2 && (input_headers[k].second & PATH);

        std::string url = QuicUrlUtilsImpl::GetPushPromiseUrl(
            input_headers[i].first, input_headers[j].first,
            input_headers[k].first);

        ::testing::AssertionResult result = ::testing::AssertionSuccess();
        if (url.empty() == should_accept_3) {
          result = ::testing::AssertionFailure()
                   << "GetPushPromiseUrl() accepted/rejected the inputs when "
                      "it shouldn't have."
                   << std::endl
                   << "     scheme: " << input_headers[i].first << std::endl
                   << "  authority: " << input_headers[j].first << std::endl
                   << "       path: " << input_headers[k].first << std::endl
                   << "Output: " << url << std::endl;
        }
        ASSERT_TRUE(result);
      }
    }
  }

  // Test canonicalization of various valid inputs.
  EXPECT_EQ("http://www.google.com/",
            QuicUrlUtilsImpl::GetPushPromiseUrl("http", "www.google.com", "/"));
  EXPECT_EQ("https://www.goo-gle.com/fOOo/baRR",
            QuicUrlUtilsImpl::GetPushPromiseUrl("hTtPs", "wWw.gOo-gLE.cOm",
                                                "/fOOo/baRR"));
  EXPECT_EQ("https://www.goo-gle.com:3278/pAth/To/reSOurce",
            QuicUrlUtilsImpl::GetPushPromiseUrl(
                "hTtPs", "Www.gOo-Gle.Com:000003278", "/pAth/To/reSOurce"));
  EXPECT_EQ(
      "https://foo%20bar/foo/bar/baz",
      QuicUrlUtilsImpl::GetPushPromiseUrl("https", "foo bar", "/foo/bar/baz"));
  EXPECT_EQ("http://foo.com:70/e/", QuicUrlUtilsImpl::GetPushPromiseUrl(
                                        "http", "foo.com:0000070", "/e/"));
  EXPECT_EQ("http://192.168.0.1:70/e/",
            QuicUrlUtilsImpl::GetPushPromiseUrl("http", "0300.0250.00.01:0070",
                                                "/e/"));
  EXPECT_EQ("http://192.168.0.1/e/",
            QuicUrlUtilsImpl::GetPushPromiseUrl("http", "0xC0a80001", "/e/"));
  EXPECT_EQ("http://[::c0a8:1]/", QuicUrlUtilsImpl::GetPushPromiseUrl(
                                      "http", "[::192.168.0.1]", "/"));
  EXPECT_EQ("https://[::ffff:c0a8:1]/",
            QuicUrlUtilsImpl::GetPushPromiseUrl(
                "https", "[::ffff:0xC0.0Xa8.0x0.0x1]", "/"));
}

};  // namespace
};  // namespace test
};  // namespace net