// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGPageLayoutAlgorithm_h
#define NGPageLayoutAlgorithm_h

#include "third_party/blink/renderer/core/layout/ng/ng_layout_algorithm.h"

#include "third_party/blink/renderer/core/layout/ng/ng_fragment_builder.h"

namespace blink {

class NGBlockNode;
class NGBlockBreakToken;
class NGBreakToken;
class NGConstraintSpace;
struct NGLogicalSize;

class CORE_EXPORT NGPageLayoutAlgorithm
    : public NGLayoutAlgorithm<NGBlockNode,
                               NGFragmentBuilder,
                               NGBlockBreakToken> {
 public:
  NGPageLayoutAlgorithm(NGBlockNode node,
                        const NGConstraintSpace& space,
                        NGBreakToken* break_token = nullptr);

  scoped_refptr<NGLayoutResult> Layout() override;

  base::Optional<MinMaxSize> ComputeMinMaxSize(
      const MinMaxSizeInput&) const override;

 private:
  scoped_refptr<NGConstraintSpace> CreateConstraintSpaceForPages(
      const NGLogicalSize& size) const;
};

}  // namespace blink

#endif  // NGPageLayoutAlgorithm_h
