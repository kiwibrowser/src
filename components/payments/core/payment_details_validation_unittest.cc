// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_details_validation.h"

#include <ostream>
#include <utility>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "components/payments/core/payment_details.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

const bool REQUIRE_TOTAL = true;
const bool DO_NOT_REQUIRE_TOTAL = false;
const bool EXPECT_VALID = true;
const bool EXPECT_INVALID = false;

struct TestCase {
  TestCase(const char* details, bool require_total, bool expect_valid)
      : details(details),
        require_total(require_total),
        expect_valid(expect_valid) {}
  ~TestCase() {}

  const char* const details;
  const bool require_total;
  const bool expect_valid;
};

std::ostream& operator<<(std::ostream& out, const TestCase& test_case) {
  out << test_case.details;
  return out;
}

class PaymentDetailsValidationTest : public ::testing::TestWithParam<TestCase> {
};

TEST_P(PaymentDetailsValidationTest, Test) {
  auto value = base::JSONReader::Read(GetParam().details);
  ASSERT_NE(nullptr, value.get()) << "Should be in JSON format";
  auto dictionary = base::DictionaryValue::From(std::move(value));
  ASSERT_NE(nullptr, dictionary.get()) << "Should be a dictionary";
  PaymentDetails details;
  ASSERT_TRUE(
      details.FromDictionaryValue(*dictionary, GetParam().require_total));
  std::string unused;

  EXPECT_EQ(GetParam().expect_valid, ValidatePaymentDetails(details, &unused));
}

INSTANTIATE_TEST_CASE_P(TestCases,
                        PaymentDetailsValidationTest,
                        ::testing::Values(TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "1.00"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "total": {
        "label": "",
        "amount": {"currency": "USD", "value": "1.00"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "1"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "100"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": ""}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "-1"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "1/3"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "1,3"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "ABC"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "", "value": "1.00"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "usd", "value": "1.00"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "123", "value": "1.00"}
    }
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "1.00"}
    },
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "0.99"}
    }, {
        "label": "Tax",
        "amount": {"currency": "USD", "value": "0.01"}
    }]
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "total": {
        "label": "",
        "amount": {"currency": "USD", "value": "1.00"}
    },
    "displayItems": [{
        "label": "",
        "amount": {"currency": "USD", "value": "0.99"}
    }, {
        "label": "",
        "amount": {"currency": "USD", "value": "0.01"}
    }]
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "total": {
        "label": "Tots",
        "amount": {"currency": "USD", "value": "1.00"}
    },
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "$", "value": "0.99"}
    }, {
        "label": "Tax",
        "amount": {"currency": "USD", "value": "0.01"}
    }]
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "",
        "amount": {"currency": "USD", "value": "1.00"}
    },
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "0,99"}
    }, {
        "label": "Tax",
        "amount": {"currency": "USD", "value": "0.01"}
    }]
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "total": {
        "label": "",
        "amount": {"currency": "USD", "value": "1.00"}
    },
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }]
})",
                                                   REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }]
})",
                                                   DO_NOT_REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }],
    "modifiers": [{
      "supportedMethods": ["basic-card"],
      "data": {
        "supportedTypes": ["debit"]
      },
      "total": {
        "label": "Discounted total",
        "amount": {"currency": "USD", "value": "0.80"}
      },
      "additionalDisplayItems": [{
        "label": "Debit card discount",
        "amount": {"currency": "USD", "value": "-0.20"}
      }]
    }, {
      "supportedMethods": ["basic-card"],
      "data": {
        "supportedTypes": ["mastercard"]
      },
      "total": {
        "label": "MasterCard discounted total",
        "amount": {"currency": "USD", "value": "0.80"}
      },
      "additionalDisplayItems": [{
        "label": "MasterCard discount",
        "amount": {"currency": "USD", "value": "-0.20"}
      }]
    }]
})",
                                                   DO_NOT_REQUIRE_TOTAL,
                                                   EXPECT_VALID),
                                          TestCase(R"(
{
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }],
    "modifiers": [{
      "supportedMethods": ["basic-card"],
      "data": {
        "supportedTypes": ["debit"]
      },
      "total": {
        "label": "Discounted total",
        "amount": {"currency": "$", "value": "0.80"}
      },
      "additionalDisplayItems": [{
        "label": "Debit card discount",
        "amount": {"currency": "USD", "value": "-0.20"}
      }]
    }]
})",
                                                   DO_NOT_REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }],
    "modifiers": [{
      "supportedMethods": ["basic-card"],
      "data": {
        "supportedTypes": ["debit"]
      },
      "total": {
        "label": "Discounted total",
        "amount": {"currency": "USD", "value": "0.80"}
      },
      "additionalDisplayItems": [{
        "label": "Debit card discount",
        "amount": {"currency": "$", "value": "-0.20"}
      }]
    }]
})",
                                                   DO_NOT_REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }],
    "modifiers": [{
      "supportedMethods": ["basic-card"],
      "data": {
        "supportedTypes": ["debit"]
      },
      "total": {
        "label": "Discounted total",
        "amount": {"currency": "USD", "value": "-0.80"}
      },
      "additionalDisplayItems": [{
        "label": "Debit card discount",
        "amount": {"currency": "USD", "value": "-0.20"}
      }]
    }]
})",
                                                   DO_NOT_REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }],
    "modifiers": [{
      "supportedMethods": ["basic-card"],
      "data": {
        "supportedTypes": ["debit"]
      },
      "total": {
        "label": "Discounted total",
        "amount": {"currency": "USD", "value": "8,000"}
      },
      "additionalDisplayItems": [{
        "label": "Debit card discount",
        "amount": {"currency": "USD", "value": "-0.20"}
      }]
    }]
})",
                                                   DO_NOT_REQUIRE_TOTAL,
                                                   EXPECT_INVALID),
                                          TestCase(R"(
{
    "displayItems": [{
        "label": "Subtotal",
        "amount": {"currency": "USD", "value": "1.01"}
    }, {
        "label": "Discount",
        "amount": {"currency": "USD", "value": "-0.01"}
    }],
    "modifiers": [{
      "supportedMethods": ["basic-card"],
      "data": {
        "supportedTypes": ["debit"]
      },
      "total": {
        "label": "Discounted total",
        "amount": {"currency": "USD", "value": "0.80"}
      },
      "additionalDisplayItems": [{
        "label": "Debit card discount",
        "amount": {"currency": "USD", "value": "ABC"}
      }]
    }]
})",
                                                   DO_NOT_REQUIRE_TOTAL,
                                                   EXPECT_INVALID)));

}  // namespace
}  // namespace payments
