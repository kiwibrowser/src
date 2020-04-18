// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/can_make_payment_query.h"

#include "base/message_loop/message_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace payments {
namespace {

struct TestCase {
  TestCase(const char* first_origin,
           const char* second_origin,
           bool expected_second_origin_different_query_allowed)
      : first_origin(first_origin),
        second_origin(second_origin),
        expected_second_origin_different_query_allowed(
            expected_second_origin_different_query_allowed) {}

  ~TestCase() {}

  const char* const first_origin;
  const char* const second_origin;
  const bool expected_second_origin_different_query_allowed;
};

class CanMakePaymentQueryTest : public ::testing::TestWithParam<TestCase> {
 private:
  base::MessageLoop message_loop_;
};

TEST_P(CanMakePaymentQueryTest, SecondOriginDifferentQuery) {
  std::map<std::string, std::set<std::string>> query1;
  query1["amex"] = std::set<std::string>();
  std::map<std::string, std::set<std::string>> query2;
  query2["visa"] = std::set<std::string>();
  CanMakePaymentQuery guard;
  EXPECT_TRUE(guard.CanQuery(GURL(GetParam().first_origin),
                             GURL(GetParam().first_origin), query1));

  EXPECT_EQ(GetParam().expected_second_origin_different_query_allowed,
            guard.CanQuery(GURL(GetParam().second_origin),
                           GURL(GetParam().second_origin), query2));
}

INSTANTIATE_TEST_CASE_P(
    Denied,
    CanMakePaymentQueryTest,
    testing::Values(
        TestCase("https://example.com", "https://example.com", false),
        TestCase("http://localhost", "http://localhost", false),
        TestCase("file:///tmp/test.html", "file:///tmp/test.html", false)));

INSTANTIATE_TEST_CASE_P(
    Allowed,
    CanMakePaymentQueryTest,
    testing::Values(
        TestCase("https://example.com", "https://not-example.com", true),
        TestCase("http://localhost", "http://not-localhost", true),
        TestCase("file:///tmp/test.html", "file:///tmp/not-test.html", true)));

}  // namespace
}  // namespace payments
