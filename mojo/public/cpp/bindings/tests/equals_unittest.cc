// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {

namespace {

RectPtr CreateRect() {
  return Rect::New(1, 2, 3, 4);
}

using EqualsTest = testing::Test;

}  // namespace

TEST_F(EqualsTest, NullStruct) {
  RectPtr r1;
  RectPtr r2;
  EXPECT_TRUE(r1.Equals(r2));
  EXPECT_TRUE(r2.Equals(r1));

  r1 = CreateRect();
  EXPECT_FALSE(r1.Equals(r2));
  EXPECT_FALSE(r2.Equals(r1));
}

TEST_F(EqualsTest, Struct) {
  RectPtr r1(CreateRect());
  RectPtr r2(r1.Clone());
  EXPECT_TRUE(r1.Equals(r2));
  r2->y = 1;
  EXPECT_FALSE(r1.Equals(r2));
  r2.reset();
  EXPECT_FALSE(r1.Equals(r2));
}

TEST_F(EqualsTest, StructNested) {
  RectPairPtr p1(RectPair::New(CreateRect(), CreateRect()));
  RectPairPtr p2(p1.Clone());
  EXPECT_TRUE(p1.Equals(p2));
  p2->second->width = 0;
  EXPECT_FALSE(p1.Equals(p2));
  p2->second.reset();
  EXPECT_FALSE(p1.Equals(p2));
}

TEST_F(EqualsTest, Array) {
  std::vector<RectPtr> rects;
  rects.push_back(CreateRect());
  NamedRegionPtr n1(NamedRegion::New(std::string("n1"), std::move(rects)));
  NamedRegionPtr n2(n1.Clone());
  EXPECT_TRUE(n1.Equals(n2));

  n2->rects = base::nullopt;
  EXPECT_FALSE(n1.Equals(n2));
  n2->rects.emplace();
  EXPECT_FALSE(n1.Equals(n2));

  n2->rects->push_back(CreateRect());
  n2->rects->push_back(CreateRect());
  EXPECT_FALSE(n1.Equals(n2));

  n2->rects->resize(1);
  (*n2->rects)[0]->width = 0;
  EXPECT_FALSE(n1.Equals(n2));

  (*n2->rects)[0] = CreateRect();
  EXPECT_TRUE(n1.Equals(n2));
}

TEST_F(EqualsTest, InterfacePtr) {
  base::MessageLoop message_loop;

  SomeInterfacePtr inf1;
  SomeInterfacePtr inf2;

  EXPECT_TRUE(inf1.Equals(inf1));
  EXPECT_TRUE(inf1.Equals(inf2));

  auto inf1_request = MakeRequest(&inf1);
  ALLOW_UNUSED_LOCAL(inf1_request);

  EXPECT_TRUE(inf1.Equals(inf1));
  EXPECT_FALSE(inf1.Equals(inf2));

  auto inf2_request = MakeRequest(&inf2);
  ALLOW_UNUSED_LOCAL(inf2_request);

  EXPECT_FALSE(inf1.Equals(inf2));
}

TEST_F(EqualsTest, InterfaceRequest) {
  base::MessageLoop message_loop;

  InterfaceRequest<SomeInterface> req1;
  InterfaceRequest<SomeInterface> req2;

  EXPECT_TRUE(req1.Equals(req1));
  EXPECT_TRUE(req1.Equals(req2));

  SomeInterfacePtr inf1;
  req1 = MakeRequest(&inf1);

  EXPECT_TRUE(req1.Equals(req1));
  EXPECT_FALSE(req1.Equals(req2));

  SomeInterfacePtr inf2;
  req2 = MakeRequest(&inf2);

  EXPECT_FALSE(req1.Equals(req2));
}

}  // test
}  // mojo
