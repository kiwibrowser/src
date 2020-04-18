// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_parsing_utils.h"

#include "device/fido/fido_test_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace fido_parsing_utils {

namespace {
constexpr uint8_t kOne[] = {0x01};
constexpr uint8_t kOneTwo[] = {0x01, 0x02};
constexpr uint8_t kTwo[] = {0x02};
constexpr uint8_t kTwoThree[] = {0x02, 0x03};
constexpr uint8_t kThree[] = {0x03};
constexpr uint8_t kOneTwoThree[] = {0x01, 0x02, 0x03};
}  // namespace

TEST(U2fParsingUtils, SpanLess) {
  const std::array<int, 4> kOneTwoThreeFour = {1, 2, 3, 4};

  EXPECT_FALSE(SpanLess()(kOne, kOne));
  EXPECT_TRUE(SpanLess()(kOne, kOneTwo));
  EXPECT_TRUE(SpanLess()(kOne, kTwo));
  EXPECT_TRUE(SpanLess()(kOne, kTwoThree));
  EXPECT_TRUE(SpanLess()(kOne, kThree));
  EXPECT_TRUE(SpanLess()(kOne, kOneTwoThree));
  EXPECT_TRUE(SpanLess()(kOne, kOneTwoThreeFour));

  EXPECT_FALSE(SpanLess()(kOneTwo, kOne));
  EXPECT_FALSE(SpanLess()(kOneTwo, kOneTwo));
  EXPECT_TRUE(SpanLess()(kOneTwo, kTwo));
  EXPECT_TRUE(SpanLess()(kOneTwo, kTwoThree));
  EXPECT_TRUE(SpanLess()(kOneTwo, kThree));
  EXPECT_TRUE(SpanLess()(kOneTwo, kOneTwoThree));
  EXPECT_TRUE(SpanLess()(kOneTwo, kOneTwoThreeFour));

  EXPECT_FALSE(SpanLess()(kTwo, kOne));
  EXPECT_FALSE(SpanLess()(kTwo, kOneTwo));
  EXPECT_FALSE(SpanLess()(kTwo, kTwo));
  EXPECT_TRUE(SpanLess()(kTwo, kTwoThree));
  EXPECT_TRUE(SpanLess()(kTwo, kThree));
  EXPECT_FALSE(SpanLess()(kTwo, kOneTwoThree));
  EXPECT_FALSE(SpanLess()(kTwo, kOneTwoThreeFour));

  EXPECT_FALSE(SpanLess()(kTwoThree, kOne));
  EXPECT_FALSE(SpanLess()(kTwoThree, kOneTwo));
  EXPECT_FALSE(SpanLess()(kTwoThree, kTwo));
  EXPECT_FALSE(SpanLess()(kTwoThree, kTwoThree));
  EXPECT_TRUE(SpanLess()(kTwoThree, kThree));
  EXPECT_FALSE(SpanLess()(kTwoThree, kOneTwoThree));
  EXPECT_FALSE(SpanLess()(kTwoThree, kOneTwoThreeFour));

  EXPECT_FALSE(SpanLess()(kThree, kOne));
  EXPECT_FALSE(SpanLess()(kThree, kOneTwo));
  EXPECT_FALSE(SpanLess()(kThree, kTwo));
  EXPECT_FALSE(SpanLess()(kThree, kTwoThree));
  EXPECT_FALSE(SpanLess()(kThree, kThree));
  EXPECT_FALSE(SpanLess()(kThree, kOneTwoThree));
  EXPECT_FALSE(SpanLess()(kThree, kOneTwoThreeFour));

  EXPECT_FALSE(SpanLess()(kOneTwoThree, kOne));
  EXPECT_FALSE(SpanLess()(kOneTwoThree, kOneTwo));
  EXPECT_TRUE(SpanLess()(kOneTwoThree, kTwo));
  EXPECT_TRUE(SpanLess()(kOneTwoThree, kTwoThree));
  EXPECT_TRUE(SpanLess()(kOneTwoThree, kThree));
  EXPECT_FALSE(SpanLess()(kOneTwoThree, kOneTwoThree));
  EXPECT_TRUE(SpanLess()(kOneTwoThree, kOneTwoThreeFour));

  EXPECT_FALSE(SpanLess()(kOneTwoThreeFour, kOne));
  EXPECT_FALSE(SpanLess()(kOneTwoThreeFour, kOneTwo));
  EXPECT_TRUE(SpanLess()(kOneTwoThreeFour, kTwo));
  EXPECT_TRUE(SpanLess()(kOneTwoThreeFour, kTwoThree));
  EXPECT_TRUE(SpanLess()(kOneTwoThreeFour, kThree));
  EXPECT_FALSE(SpanLess()(kOneTwoThreeFour, kOneTwoThree));
  EXPECT_FALSE(SpanLess()(kOneTwoThreeFour, kOneTwoThreeFour));
}

TEST(U2fParsingUtils, Materialize) {
  const std::vector<uint8_t> empty;
  EXPECT_THAT(Materialize(empty), ::testing::IsEmpty());
  EXPECT_THAT(Materialize(base::span<const uint8_t>()), ::testing::IsEmpty());

  EXPECT_THAT(Materialize(kOne), ::testing::ElementsAreArray(kOne));
  EXPECT_THAT(Materialize(kOneTwoThree),
              ::testing::ElementsAreArray(kOneTwoThree));
}

TEST(U2fParsingUtils, MaterializeOrNull) {
  auto result = MaterializeOrNull(kOneTwoThree);
  ASSERT_TRUE(result.has_value());
  EXPECT_THAT(*result, ::testing::ElementsAreArray(kOneTwoThree));

  EXPECT_EQ(MaterializeOrNull(base::nullopt), base::nullopt);
}

TEST(U2fParsingUtils, Append) {
  std::vector<uint8_t> target;

  Append(&target, base::span<const uint8_t>());
  EXPECT_THAT(target, ::testing::IsEmpty());

  // Should be idempotent, try twice for good measure.
  Append(&target, base::span<const uint8_t>());
  EXPECT_THAT(target, ::testing::IsEmpty());

  const std::vector<uint8_t> one(std::begin(kOne), std::end(kOne));
  Append(&target, one);
  EXPECT_THAT(target, ::testing::ElementsAreArray(kOne));

  Append(&target, kTwoThree);
  EXPECT_THAT(target, ::testing::ElementsAreArray(kOneTwoThree));
}

TEST(U2fParsingUtils, AppendSelfCrashes) {
  std::vector<uint8_t> target(std::begin(kOneTwoThree), std::end(kOneTwoThree));
  auto span = base::make_span(target);

  // Tests the case where |in_values| overlap with the beginning of |*target|.
  EXPECT_DEATH_IF_SUPPORTED(Append(&target, span.first(1)), "");

  // Tests the case where |in_values| overlap with the end of |*target|.
  EXPECT_DEATH_IF_SUPPORTED(Append(&target, span.last(1)), "");
}

// ExtractSpan and ExtractSuffixSpan are implicitly tested as they used by
// the Extract and ExtractSuffix implementations.

TEST(U2fParsingUtils, ExtractEmpty) {
  const std::vector<uint8_t> empty;
  EXPECT_THAT(Extract(empty, 0, 0), ::testing::IsEmpty());

  EXPECT_THAT(Extract(kOne, 0, 0), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOne, 1, 0), ::testing::IsEmpty());

  EXPECT_THAT(Extract(kOneTwoThree, 0, 0), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOneTwoThree, 1, 0), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOneTwoThree, 2, 0), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOneTwoThree, 3, 0), ::testing::IsEmpty());
}

TEST(U2fParsingUtils, ExtractInBounds) {
  EXPECT_THAT(Extract(kOne, 0, 1), ::testing::ElementsAreArray(kOne));
  EXPECT_THAT(Extract(kOneTwoThree, 0, 1), ::testing::ElementsAreArray(kOne));
  EXPECT_THAT(Extract(kOneTwoThree, 2, 1), ::testing::ElementsAreArray(kThree));
  EXPECT_THAT(Extract(kOneTwoThree, 1, 2),
              ::testing::ElementsAreArray(kTwoThree));
  EXPECT_THAT(Extract(kOneTwoThree, 0, 3),
              ::testing::ElementsAreArray(kOneTwoThree));
}

TEST(U2fParsingUtils, ExtractOutOfBounds) {
  const std::vector<uint8_t> empty;
  EXPECT_THAT(Extract(empty, 0, 1), ::testing::IsEmpty());
  EXPECT_THAT(Extract(empty, 1, 0), ::testing::IsEmpty());

  EXPECT_THAT(Extract(kOne, 0, 2), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOne, 1, 1), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOne, 2, 0), ::testing::IsEmpty());

  EXPECT_THAT(Extract(kOneTwoThree, 0, 4), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOneTwoThree, 1, 3), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOneTwoThree, 2, 2), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOneTwoThree, 3, 1), ::testing::IsEmpty());
  EXPECT_THAT(Extract(kOneTwoThree, 4, 0), ::testing::IsEmpty());
}

TEST(U2fParsingUtils, ExtractSuffixEmpty) {
  const std::vector<uint8_t> empty;
  EXPECT_THAT(ExtractSuffix(empty, 0), ::testing::IsEmpty());
  EXPECT_THAT(ExtractSuffix(kOne, 1), ::testing::IsEmpty());
  EXPECT_THAT(ExtractSuffix(kOneTwoThree, 3), ::testing::IsEmpty());
}

TEST(U2fParsingUtils, ExtractSuffixInBounds) {
  EXPECT_THAT(ExtractSuffix(kOne, 0), ::testing::ElementsAreArray(kOne));
  EXPECT_THAT(ExtractSuffix(kOneTwoThree, 1),
              ::testing::ElementsAreArray(kTwoThree));
  EXPECT_THAT(ExtractSuffix(kOneTwoThree, 2),
              ::testing::ElementsAreArray(kThree));
}

TEST(U2fParsingUtils, ExtractSuffixOutOfBounds) {
  const std::vector<uint8_t> empty;
  EXPECT_THAT(ExtractSuffix(empty, 1), ::testing::IsEmpty());
  EXPECT_THAT(ExtractSuffix(kOne, 2), ::testing::IsEmpty());
  EXPECT_THAT(ExtractSuffix(kOneTwoThree, 4), ::testing::IsEmpty());
}

TEST(U2fParsingUtils, ExtractArray) {
  const std::vector<uint8_t> empty;
  std::array<uint8_t, 0> array_empty;
  EXPECT_TRUE(ExtractArray(empty, 0, &array_empty));

  std::array<uint8_t, 2> array_two_three;
  EXPECT_TRUE(ExtractArray(kTwoThree, 0, &array_two_three));
  EXPECT_THAT(array_two_three, ::testing::ElementsAreArray(kTwoThree));

  EXPECT_FALSE(ExtractArray(kOneTwoThree, 2, &array_two_three));

  std::array<uint8_t, 1> array_three;
  EXPECT_TRUE(ExtractArray(kOneTwoThree, 2, &array_three));
  EXPECT_THAT(array_three, ::testing::ElementsAreArray(kThree));
}

TEST(U2fParsingUtils, SplitSpan) {
  std::vector<uint8_t> empty;
  EXPECT_THAT(SplitSpan(empty, 1), ::testing::IsEmpty());
  EXPECT_THAT(SplitSpan(empty, 2), ::testing::IsEmpty());
  EXPECT_THAT(SplitSpan(empty, 3), ::testing::IsEmpty());

  EXPECT_THAT(SplitSpan(kOne, 1),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOne)));
  EXPECT_THAT(SplitSpan(kOne, 2),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOne)));
  EXPECT_THAT(SplitSpan(kOne, 3),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOne)));

  EXPECT_THAT(SplitSpan(kOneTwo, 1),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOne),
                                     ::testing::ElementsAreArray(kTwo)));
  EXPECT_THAT(SplitSpan(kOneTwo, 2),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOneTwo)));
  EXPECT_THAT(SplitSpan(kOneTwo, 3),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOneTwo)));

  EXPECT_THAT(SplitSpan(kOneTwoThree, 1),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOne),
                                     ::testing::ElementsAreArray(kTwo),
                                     ::testing::ElementsAreArray(kThree)));
  EXPECT_THAT(SplitSpan(kOneTwoThree, 2),
              ::testing::ElementsAre(::testing::ElementsAreArray(kOneTwo),
                                     ::testing::ElementsAreArray(kThree)));
  EXPECT_THAT(
      SplitSpan(kOneTwoThree, 3),
      ::testing::ElementsAre(::testing::ElementsAreArray(kOneTwoThree)));
  EXPECT_THAT(
      SplitSpan(kOneTwoThree, 4),
      ::testing::ElementsAre(::testing::ElementsAreArray(kOneTwoThree)));
  EXPECT_THAT(
      SplitSpan(kOneTwoThree, 5),
      ::testing::ElementsAre(::testing::ElementsAreArray(kOneTwoThree)));
  EXPECT_THAT(
      SplitSpan(kOneTwoThree, 6),
      ::testing::ElementsAre(::testing::ElementsAreArray(kOneTwoThree)));
}

TEST(U2fParsingUtils, CreateSHA256Hash) {
  EXPECT_THAT(CreateSHA256Hash("acme.com"),
              ::testing::ElementsAreArray(test_data::kApplicationParameter));
}

}  // namespace fido_parsing_utils
}  // namespace device
