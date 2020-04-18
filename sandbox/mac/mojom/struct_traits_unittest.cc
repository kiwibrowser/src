// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "sandbox/mac/mojom/seatbelt_extension_token_struct_traits.h"
#include "sandbox/mac/mojom/traits_test_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {
namespace {

class StructTraitsTest : public testing::Test,
                         public sandbox::mac::mojom::TraitsTestService {
 public:
  StructTraitsTest()
      : interface_ptr_(), binding_(this, mojo::MakeRequest(&interface_ptr_)) {}

  sandbox::mac::mojom::TraitsTestService* interface() {
    return interface_ptr_.get();
  }

 private:
  // TraitsTestService:
  void EchoSeatbeltExtensionToken(
      sandbox::SeatbeltExtensionToken token,
      EchoSeatbeltExtensionTokenCallback callback) override {
    std::move(callback).Run(std::move(token));
  }

  base::test::ScopedTaskEnvironment task_environment_;

  sandbox::mac::mojom::TraitsTestServicePtr interface_ptr_;
  mojo::Binding<sandbox::mac::mojom::TraitsTestService> binding_;
};

TEST_F(StructTraitsTest, SeatbeltExtensionToken) {
  auto input = sandbox::SeatbeltExtensionToken::CreateForTesting("hello world");
  sandbox::SeatbeltExtensionToken output;

  interface()->EchoSeatbeltExtensionToken(std::move(input), &output);
  EXPECT_EQ("hello world", output.token());
  EXPECT_TRUE(input.token().empty());
}

}  // namespace
}  // namespace sandbox
