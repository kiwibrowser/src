// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/wtf/experimental/container_type_operations.h"

#include <cstring>

#include "testing/gtest/include/gtest/gtest.h"

namespace WTF {
namespace experimental {

namespace {

struct Pod {
  int a;
  int* b;
  char c[7];
};

bool operator==(const Pod& left, const Pod& right) {
  return left.a == right.a && left.b == right.b &&
         memcmp(left.c, right.c, sizeof(left.c)) == 0;
}

}  // namespace

template <typename T>
class ContainerTypeOperationsTest : public testing::Test {};

TYPED_TEST_CASE_P(ContainerTypeOperationsTest);

TYPED_TEST_P(ContainerTypeOperationsTest, Completeness) {
  using Ops = CompletedContainerTypeOperations<TypeParam>;
  // Call all funcions in Ops and make sure they are callable.

  constexpr size_t SIZE = 4;
  // We need to initialize the content of |data| to let the compiler think
  // accessing this memory is safe. If the compiler detects undefined behavior,
  // it may "optimize" the code in an unexpected way and make the assertions
  // fail.
  alignas(TypeParam) char data[sizeof(TypeParam) * SIZE] = {0};
  TypeParam* storage = reinterpret_cast<TypeParam*>(data);

  Ops::DefaultInitialize(*storage);
  Ops::Destruct(*storage);

  Ops::DefaultInitializeRange(storage, storage + SIZE);
  Ops::DestructRange(storage, storage + SIZE);

  Ops::DefaultInitializeRange(storage, storage + SIZE);
  // We also need to zero-initialize this variable for the same reason as above.
  TypeParam lvalue{};
  Ops::Assign(*storage, lvalue);
  Ops::Assign(*storage, TypeParam());  // rvalue.
  Ops::CopyRange(storage, storage + 2, storage + 2);
  Ops::MoveRange(storage, storage + 2, storage + 2);
  Ops::CopyOverlappingRange(storage, storage + 3, storage + 1);
  Ops::MoveOverlappingRange(storage, storage + 3, storage + 1);

  Ops::DestructRange(storage, storage + 2);
  Ops::UninitializedCopy(storage + 2, storage + 4, storage);
  Ops::DestructRange(storage, storage + 2);
  Ops::UninitializedFill(lvalue, storage, storage + 2);

  // The first element may be moved. Let's make sure it is in some valid state.
  Ops::Assign(*storage, lvalue);

  EXPECT_TRUE(Ops::Equal(*storage, *storage));
  EXPECT_TRUE(Ops::EqualRange(storage, storage + SIZE, storage));

  Ops::DestructRange(storage, storage + SIZE);
}

REGISTER_TYPED_TEST_CASE_P(ContainerTypeOperationsTest, Completeness);

using PodTestTypes = testing::Types<int, char, int*, Pod>;

INSTANTIATE_TYPED_TEST_CASE_P(Pod, ContainerTypeOperationsTest, PodTestTypes);

}  // namespace experimental
}  // namespace WTF
