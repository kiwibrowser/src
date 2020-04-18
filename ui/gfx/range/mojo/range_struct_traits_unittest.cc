// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/range/mojo/range_traits_test_service.mojom.h"

namespace gfx {

namespace {

class RangeStructTraitsTest : public testing::Test,
                              public mojom::RangeTraitsTestService {
 public:
  RangeStructTraitsTest() {}

 protected:
  mojom::RangeTraitsTestServicePtr GetTraitsTestProxy() {
    mojom::RangeTraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // RangeTraitsTestService:
  void EchoRange(const Range& p, EchoRangeCallback callback) override {
    std::move(callback).Run(p);
  }

  void EchoRangeF(const RangeF& p, EchoRangeFCallback callback) override {
    std::move(callback).Run(p);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<RangeTraitsTestService> traits_test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(RangeStructTraitsTest);
};

}  // namespace

TEST_F(RangeStructTraitsTest, Range) {
  const uint32_t start = 1234;
  const uint32_t end = 5678;
  gfx::Range input(start, end);
  mojom::RangeTraitsTestServicePtr proxy = GetTraitsTestProxy();
  gfx::Range output;
  proxy->EchoRange(input, &output);
  EXPECT_EQ(start, output.start());
  EXPECT_EQ(end, output.end());
}

TEST_F(RangeStructTraitsTest, RangeF) {
  const float start = 1234.5f;
  const float end = 6789.6f;
  gfx::RangeF input(start, end);
  mojom::RangeTraitsTestServicePtr proxy = GetTraitsTestProxy();
  gfx::RangeF output;
  proxy->EchoRangeF(input, &output);
  EXPECT_EQ(start, output.start());
  EXPECT_EQ(end, output.end());
}

}  // namespace gfx
