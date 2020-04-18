// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NG_BASE_LAYOUT_ALGORITHM_TEST_H_
#define NG_BASE_LAYOUT_ALGORITHM_TEST_H_

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_test.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class Element;
class LayoutNGBlockFlow;
class NGPhysicalBoxFragment;

// Base class for all LayoutNG Algorithms unit test classes.
typedef bool TestParamLayoutNG;
class NGBaseLayoutAlgorithmTest
    : public testing::WithParamInterface<TestParamLayoutNG>,
      public NGLayoutTest {
 protected:
  void SetUp() override;

  std::pair<scoped_refptr<NGPhysicalBoxFragment>,
            scoped_refptr<NGConstraintSpace>>
  RunBlockLayoutAlgorithmForElement(Element* element);

  scoped_refptr<const NGPhysicalBoxFragment> GetBoxFragmentByElementId(
      const char*);

  static const NGPhysicalBoxFragment* CurrentFragmentFor(
      const LayoutNGBlockFlow*);
};

class FragmentChildIterator {
  STACK_ALLOCATED();

 public:
  explicit FragmentChildIterator(const NGPhysicalBoxFragment* parent) {
    SetParent(parent);
  }
  void SetParent(const NGPhysicalBoxFragment* parent) {
    parent_ = parent;
    index_ = 0;
  }

  const NGPhysicalBoxFragment* NextChild();

 private:
  const NGPhysicalBoxFragment* parent_;
  unsigned index_;
};

scoped_refptr<NGConstraintSpace> ConstructBlockLayoutTestConstraintSpace(
    WritingMode writing_mode,
    TextDirection direction,
    NGLogicalSize size,
    bool shrink_to_fit = false,
    bool is_new_formatting_context = false,
    LayoutUnit fragmentainer_space_available = LayoutUnit());

}  // namespace blink

#endif  // NG_BASE_LAYOUT_ALGORITHM_TEST_H_
