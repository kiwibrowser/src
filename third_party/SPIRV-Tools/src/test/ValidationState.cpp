// Copyright (c) 2015-2016 The Khronos Group Inc.
// Copyright (c) 2016 Google
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

// Unit tests for ValidationState_t.

#include <gtest/gtest.h>
#include <vector>

#include "spirv/spirv.h"

#include "source/validate.h"

namespace {
using libspirv::ValidationState_t;
using std::vector;

// A test with a ValidationState_t member transparently.
class ValidationStateTest : public testing::Test {
 public:
  ValidationStateTest()
      : context_(spvContextCreate(SPV_ENV_UNIVERSAL_1_0)),
        state_(&diag_, context_) {}

 protected:
  spv_diagnostic diag_;
  spv_context context_;
  ValidationState_t state_;
};

// A test of ValidationState_t::HasAnyOf().
class ValidationState_HasAnyOfTest : public ValidationStateTest {
 protected:
  spv_capability_mask_t mask(const vector<SpvCapability>& capabilities) {
    spv_capability_mask_t m = 0;
    for (auto c : capabilities) m |= SPV_CAPABILITY_AS_MASK(c);
    return m;
  }
};

TEST_F(ValidationState_HasAnyOfTest, EmptyMask) {
  EXPECT_TRUE(state_.HasAnyOf(0));
  state_.registerCapability(SpvCapabilityMatrix);
  EXPECT_TRUE(state_.HasAnyOf(0));
  state_.registerCapability(SpvCapabilityImageMipmap);
  EXPECT_TRUE(state_.HasAnyOf(0));
  state_.registerCapability(SpvCapabilityPipes);
  EXPECT_TRUE(state_.HasAnyOf(0));
  state_.registerCapability(SpvCapabilityStorageImageArrayDynamicIndexing);
  EXPECT_TRUE(state_.HasAnyOf(0));
  state_.registerCapability(SpvCapabilityClipDistance);
  EXPECT_TRUE(state_.HasAnyOf(0));
  state_.registerCapability(SpvCapabilityStorageImageWriteWithoutFormat);
  EXPECT_TRUE(state_.HasAnyOf(0));
}

TEST_F(ValidationState_HasAnyOfTest, SingleCapMask) {
  EXPECT_FALSE(state_.HasAnyOf(mask({SpvCapabilityMatrix})));
  EXPECT_FALSE(state_.HasAnyOf(mask({SpvCapabilityImageMipmap})));
  state_.registerCapability(SpvCapabilityMatrix);
  EXPECT_TRUE(state_.HasAnyOf(mask({SpvCapabilityMatrix})));
  EXPECT_FALSE(state_.HasAnyOf(mask({SpvCapabilityImageMipmap})));
  state_.registerCapability(SpvCapabilityImageMipmap);
  EXPECT_TRUE(state_.HasAnyOf(mask({SpvCapabilityMatrix})));
  EXPECT_TRUE(state_.HasAnyOf(mask({SpvCapabilityImageMipmap})));
}

TEST_F(ValidationState_HasAnyOfTest, MultiCapMask) {
  const auto mask1 = mask({SpvCapabilitySampledRect, SpvCapabilityImageBuffer});
  const auto mask2 = mask({SpvCapabilityStorageImageWriteWithoutFormat,
                           SpvCapabilityStorageImageReadWithoutFormat,
                           SpvCapabilityGeometryStreams});
  EXPECT_FALSE(state_.HasAnyOf(mask1));
  EXPECT_FALSE(state_.HasAnyOf(mask2));
  state_.registerCapability(SpvCapabilityImageBuffer);
  EXPECT_TRUE(state_.HasAnyOf(mask1));
  EXPECT_FALSE(state_.HasAnyOf(mask2));
}
}
