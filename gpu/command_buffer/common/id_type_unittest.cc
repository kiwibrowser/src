// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "gpu/command_buffer/common/id_type.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

namespace {

class Foo;
using FooId = IdType<Foo, int, 0>;

class Bar;
using BarId = IdType<Bar, int, 0>;

class AnotherIdMarker;
class DerivedId : public IdType<AnotherIdMarker, int, 0> {
 public:
  explicit DerivedId(int unsafe_value)
      : IdType<AnotherIdMarker, int, 0>(unsafe_value) {}
};

}  // namespace

TEST(IdType, DefaultValueIsInvalid) {
  FooId foo_id;
  EXPECT_TRUE(foo_id.is_null());
}

TEST(IdType, NormalValueIsValid) {
  FooId foo_id = FooId::FromUnsafeValue(123);
  EXPECT_FALSE(foo_id.is_null());
}

TEST(IdType, OutputStreamTest) {
  FooId foo_id = FooId::FromUnsafeValue(123);

  std::ostringstream ss;
  ss << foo_id;
  EXPECT_EQ("123", ss.str());
}

TEST(IdType, IdType32) {
  IdType32<Foo> id;

  EXPECT_EQ(0, id.GetUnsafeValue());
  static_assert(sizeof(int32_t) == sizeof(id), "");
}

TEST(IdType, IdTypeU32) {
  IdTypeU32<Foo> id;

  EXPECT_EQ(0u, id.GetUnsafeValue());
  static_assert(sizeof(uint32_t) == sizeof(id), "");
}

TEST(IdType, IdType64) {
  IdType64<Foo> id;

  EXPECT_EQ(0, id.GetUnsafeValue());
  static_assert(sizeof(int64_t) == sizeof(id), "");
}

TEST(IdType, IdTypeU64) {
  IdTypeU64<Foo> id;

  EXPECT_EQ(0u, id.GetUnsafeValue());
  static_assert(sizeof(uint64_t) == sizeof(id), "");
}

TEST(IdType, DerivedClasses) {
  DerivedId derived_id(456);

  std::ostringstream ss;
  ss << derived_id;
  EXPECT_EQ("456", ss.str());

  std::map<DerivedId, std::string> ordered_map;
  ordered_map[derived_id] = "blah";
  EXPECT_EQ(ordered_map[derived_id], "blah");

  std::unordered_map<DerivedId, std::string, DerivedId::Hasher> unordered_map;
  unordered_map[derived_id] = "blah2";
  EXPECT_EQ(unordered_map[derived_id], "blah2");
}

TEST(IdType, StaticAsserts) {
  static_assert(!std::is_constructible<FooId, int>::value,
                "Should be impossible to construct FooId from a raw integer.");
  static_assert(!std::is_convertible<int, FooId>::value,
                "Should be impossible to convert a raw integer into FooId.");

  static_assert(!std::is_constructible<FooId, BarId>::value,
                "Should be impossible to construct FooId from a BarId.");
  static_assert(!std::is_convertible<BarId, FooId>::value,
                "Should be impossible to convert a BarId into FooId.");

  // The presence of a custom default constructor means that FooId is not a
  // "trivial" class and therefore is not a POD type (unlike an int32_t).
  // At the same time FooId has almost all of the properties of a POD type:
  // - is "trivially copyable" (i.e. is memcpy-able),
  // - has "standard layout" (i.e. interops with things expecting C layout).
  // See http://stackoverflow.com/a/7189821 for more info about these
  // concepts.
  static_assert(std::is_standard_layout<FooId>::value,
                "FooId should have standard layout. "
                "See http://stackoverflow.com/a/7189821 for more info.");
  static_assert(sizeof(FooId) == sizeof(int),
                "FooId should be the same size as the raw integer it wraps.");
  // TODO(lukasza): Enable these once <type_traits> supports all the standard
  // C++11 equivalents (i.e. std::is_trivially_copyable instead of the
  // non-standard std::has_trivial_copy_assign).
  // static_assert(std::has_trivial_copy_constructor<FooId>::value,
  //               "FooId should have a trivial copy constructor.");
  // static_assert(std::has_trivial_copy_assign<FooId>::value,
  //               "FooId should have a trivial copy assignment operator.");
  // static_assert(std::has_trivial_destructor<FooId>::value,
  //               "FooId should have a trivial destructor.");
}

class IdTypeSpecificValueTest : public ::testing::TestWithParam<int> {
 protected:
  FooId test_id() { return FooId::FromUnsafeValue(GetParam()); }

  FooId other_id() {
    if (GetParam() != std::numeric_limits<int>::max())
      return FooId::FromUnsafeValue(GetParam() + 1);
    else
      return FooId::FromUnsafeValue(std::numeric_limits<int>::min());
  }
};

TEST_P(IdTypeSpecificValueTest, ComparisonToSelf) {
  EXPECT_TRUE(test_id() == test_id());
  EXPECT_FALSE(test_id() != test_id());
  EXPECT_FALSE(test_id() < test_id());
}

TEST_P(IdTypeSpecificValueTest, ComparisonToOther) {
  EXPECT_FALSE(test_id() == other_id());
  EXPECT_TRUE(test_id() != other_id());
}

TEST_P(IdTypeSpecificValueTest, UnsafeValueRoundtrips) {
  int original_value = GetParam();
  FooId id = FooId::FromUnsafeValue(original_value);
  int final_value = id.GetUnsafeValue();
  EXPECT_EQ(original_value, final_value);
}

TEST_P(IdTypeSpecificValueTest, Copying) {
  FooId original = test_id();

  FooId copy_via_constructor(original);
  EXPECT_EQ(original, copy_via_constructor);

  FooId copy_via_assignment;
  copy_via_assignment = original;
  EXPECT_EQ(original, copy_via_assignment);
}

TEST_P(IdTypeSpecificValueTest, StdUnorderedMap) {
  std::unordered_map<FooId, std::string, FooId::Hasher> map;

  map[test_id()] = "test_id";
  map[other_id()] = "other_id";

  EXPECT_EQ(map[test_id()], "test_id");
  EXPECT_EQ(map[other_id()], "other_id");
}

TEST_P(IdTypeSpecificValueTest, StdMap) {
  std::map<FooId, std::string> map;

  map[test_id()] = "test_id";
  map[other_id()] = "other_id";

  EXPECT_EQ(map[test_id()], "test_id");
  EXPECT_EQ(map[other_id()], "other_id");
}

INSTANTIATE_TEST_CASE_P(,
                        IdTypeSpecificValueTest,
                        ::testing::Values(std::numeric_limits<int>::min(),
                                          -1,
                                          0,
                                          1,
                                          123,
                                          std::numeric_limits<int>::max()));

}  // namespace gpu
