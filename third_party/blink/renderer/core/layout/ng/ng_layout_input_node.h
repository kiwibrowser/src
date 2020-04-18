// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLayoutInputNode_h
#define NGLayoutInputNode_h

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"

namespace blink {

class ComputedStyle;
class Document;
class LayoutObject;
class LayoutBox;
class NGBreakToken;
class NGConstraintSpace;
class NGLayoutResult;
struct MinMaxSize;
struct NGLogicalSize;
struct NGPhysicalSize;

// Input to the min/max inline size calculation algorithm for child nodes. Child
// nodes within the same formatting context need to know which floats are beside
// them.
struct MinMaxSizeInput {
  LayoutUnit float_left_inline_size;
  LayoutUnit float_right_inline_size;
};

// Represents the input to a layout algorithm for a given node. The layout
// engine should use the style, node type to determine which type of layout
// algorithm to use to produce fragments for this node.
class CORE_EXPORT NGLayoutInputNode {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  enum NGLayoutInputNodeType {
    kBlock,
    kInline
    // When adding new values, ensure type_ below has enough bits.
  };

  NGLayoutInputNode(std::nullptr_t) : box_(nullptr), type_(kBlock) {}

  bool IsInline() const;
  bool IsBlock() const;

  bool IsColumnSpanAll() const;
  bool IsFloating() const;
  bool IsOutOfFlowPositioned() const;
  bool IsReplaced() const;
  bool IsAbsoluteContainer() const;
  bool IsFixedContainer() const;
  bool IsBody() const;
  bool IsDocumentElement() const;
  bool ShouldBeConsideredAsReplaced() const;
  bool IsListItem() const;
  bool IsListMarker() const;
  bool IsAnonymous() const;

  // If the node is a quirky container for margin collapsing, see:
  // https://html.spec.whatwg.org/#margin-collapsing-quirks
  // NOTE: The spec appears to only somewhat match reality.
  bool IsQuirkyContainer() const;

  bool CreatesNewFormattingContext() const;

  // Performs layout on this input node, will return the layout result.
  scoped_refptr<NGLayoutResult> Layout(const NGConstraintSpace&, NGBreakToken*);

  MinMaxSize ComputeMinMaxSize(WritingMode,
                               const MinMaxSizeInput&,
                               const NGConstraintSpace* = nullptr);

  // Returns intrinsic sizing information for replaced elements.
  // ComputeReplacedSize can use it to compute actual replaced size.
  // The function arguments return values from LegacyLayout intrinsic size
  // computations: LayoutReplaced::IntrinsicSizingInfo,
  // and LayoutReplaced::IntrinsicSize.
  void IntrinsicSize(NGLogicalSize* default_intrinsic_size,
                     base::Optional<LayoutUnit>* computed_inline_size,
                     base::Optional<LayoutUnit>* computed_block_size,
                     NGLogicalSize* aspect_ratio) const;

  // Returns the next sibling.
  NGLayoutInputNode NextSibling();

  Document& GetDocument() const;

  NGPhysicalSize InitialContainingBlockSize() const;

  // Returns the LayoutObject which is associated with this node.
  LayoutObject* GetLayoutObject() const;

  const ComputedStyle& Style() const;

  String ToString() const;

  explicit operator bool() const { return box_ != nullptr; }

  bool operator==(const NGLayoutInputNode& other) const {
    return box_ == other.box_;
  }

  bool operator!=(const NGLayoutInputNode& other) const {
    return !(*this == other);
  }

#ifndef NDEBUG
  void ShowNodeTree() const;
#endif

 protected:
  NGLayoutInputNode(LayoutBox* box, NGLayoutInputNodeType type)
      : box_(box), type_(type) {}

  LayoutBox* box_;

  unsigned type_ : 1;  // NGLayoutInputNodeType
};

}  // namespace blink

#endif  // NGLayoutInputNode_h
