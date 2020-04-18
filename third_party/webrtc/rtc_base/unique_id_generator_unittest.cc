/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "algorithm"
#include "string"
#include "vector"

#include "api/array_view.h"
#include "rtc_base/gunit.h"
#include "rtc_base/helpers.h"
#include "rtc_base/unique_id_generator.h"
#include "test/gmock.h"

using ::testing::IsEmpty;
using ::testing::Test;

namespace rtc {

template <typename Generator>
class UniqueIdGeneratorTest : public Test {};

using test_types = ::testing::Types<UniqueNumberGenerator<uint8_t>,
                                    UniqueNumberGenerator<uint16_t>,
                                    UniqueNumberGenerator<uint32_t>,
                                    UniqueRandomIdGenerator,
                                    UniqueStringGenerator>;

TYPED_TEST_SUITE(UniqueIdGeneratorTest, test_types);

TYPED_TEST(UniqueIdGeneratorTest, ElementsDoNotRepeat) {
  typedef TypeParam Generator;
  const size_t num_elements = 255;
  Generator generator;
  std::vector<typename Generator::value_type> values;
  for (size_t i = 0; i < num_elements; i++) {
    values.push_back(generator());
  }

  EXPECT_EQ(num_elements, values.size());
  // Use a set to check uniqueness.
  std::set<typename Generator::value_type> set(values.begin(), values.end());
  EXPECT_EQ(values.size(), set.size()) << "Returned values were not unique.";
}

TYPED_TEST(UniqueIdGeneratorTest, KnownElementsAreNotGenerated) {
  typedef TypeParam Generator;
  const size_t num_elements = 100;
  rtc::InitRandom(0);
  Generator generator1;
  std::vector<typename Generator::value_type> known_values;
  for (size_t i = 0; i < num_elements; i++) {
    known_values.push_back(generator1());
  }
  EXPECT_EQ(num_elements, known_values.size());

  rtc::InitRandom(0);
  Generator generator2(known_values);

  std::vector<typename Generator::value_type> values;
  for (size_t i = 0; i < num_elements; i++) {
    values.push_back(generator2());
  }
  EXPECT_THAT(values, ::testing::SizeIs(num_elements));
  std::sort(values.begin(), values.end());
  std::sort(known_values.begin(), known_values.end());
  std::vector<typename Generator::value_type> intersection;
  std::set_intersection(values.begin(), values.end(), known_values.begin(),
                        known_values.end(), std::back_inserter(intersection));
  EXPECT_THAT(intersection, IsEmpty());
}

TYPED_TEST(UniqueIdGeneratorTest, AddedElementsAreNotGenerated) {
  typedef TypeParam Generator;
  const size_t num_elements = 100;
  rtc::InitRandom(0);
  Generator generator1;
  std::vector<typename Generator::value_type> known_values;
  for (size_t i = 0; i < num_elements; i++) {
    known_values.push_back(generator1());
  }
  EXPECT_EQ(num_elements, known_values.size());

  rtc::InitRandom(0);
  Generator generator2;

  for (const typename Generator::value_type& value : known_values) {
    generator2.AddKnownId(value);
  }

  std::vector<typename Generator::value_type> values;
  for (size_t i = 0; i < num_elements; i++) {
    values.push_back(generator2());
  }
  EXPECT_THAT(values, ::testing::SizeIs(num_elements));
  std::sort(values.begin(), values.end());
  std::sort(known_values.begin(), known_values.end());
  std::vector<typename Generator::value_type> intersection;
  std::set_intersection(values.begin(), values.end(), known_values.begin(),
                        known_values.end(), std::back_inserter(intersection));
  EXPECT_THAT(intersection, IsEmpty());
}

}  // namespace rtc
