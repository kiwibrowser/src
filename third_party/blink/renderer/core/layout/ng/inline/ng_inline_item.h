// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGInlineItem_h
#define NGInlineItem_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping.h"
#include "third_party/blink/renderer/core/layout/ng/ng_style_variant.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"

#include <unicode/ubidi.h>
#include <unicode/uscript.h>

namespace blink {

class LayoutObject;

// Class representing a single text node or styled inline element with text
// content segmented by style, text direction, sideways rotation, font fallback
// priority (text, symbol, emoji, etc), and script (but not by font).
// In this representation TextNodes are merged up into their parent inline
// element where possible.
class CORE_EXPORT NGInlineItem {
 public:
  enum NGInlineItemType {
    kText,
    kControl,
    kAtomicInline,
    kOpenTag,
    kCloseTag,
    kFloating,
    kOutOfFlowPositioned,
    kListMarker,
    kBidiControl
    // When adding new values, make sure the bit size of |type_| is large
    // enough to store.
  };

  // Whether pre- and post-context should be used for shaping.
  enum NGLayoutInlineShapeOptions {
    kNoContext = 0,
    kPreContext = 1,
    kPostContext = 2
  };

  enum NGCollapseType {
    // No collapsible spaces.
    kNotCollapsible,
    // A collapsible space run that does not contain segment breaks.
    kCollapsibleSpace,
    // A collapsible space run that contains segment breaks.
    kCollapsibleNewline,
    // This item is opaque to whitespace collapsing.
    kOpaqueToCollapsing
  };

  // The constructor and destructor can't be implicit or inlined, because they
  // require full definition of ComputedStyle.
  NGInlineItem(NGInlineItemType type,
               unsigned start,
               unsigned end,
               const ComputedStyle* style = nullptr,
               LayoutObject* layout_object = nullptr,
               bool end_may_collapse = false);
  ~NGInlineItem();

  // Copy constructor adjusting start/end and shape results.
  NGInlineItem(const NGInlineItem&,
               unsigned adjusted_start,
               unsigned adjusted_end,
               scoped_refptr<const ShapeResult>);

  NGInlineItemType Type() const { return static_cast<NGInlineItemType>(type_); }
  const char* NGInlineItemTypeToString(int val) const;

  const ShapeResult* TextShapeResult() const { return shape_result_.get(); }
  NGLayoutInlineShapeOptions ShapeOptions() const {
    return static_cast<NGLayoutInlineShapeOptions>(shape_options_);
  }

  // If this item is "empty" for the purpose of empty block calculation.
  bool IsEmptyItem() const { return is_empty_item_; }

  // If this item should create a box fragment. Box fragments can be omitted for
  // optimization if this is false.
  bool ShouldCreateBoxFragment() const { return should_create_box_fragment_; }

  unsigned StartOffset() const { return start_offset_; }
  unsigned EndOffset() const { return end_offset_; }
  unsigned Length() const { return end_offset_ - start_offset_; }

  TextDirection Direction() const { return DirectionFromLevel(BidiLevel()); }
  UBiDiLevel BidiLevel() const { return static_cast<UBiDiLevel>(bidi_level_); }
  // Resolved bidi level for the reordering algorithm. Certain items have
  // artificial bidi level for the reordering algorithm without affecting its
  // direction.
  UBiDiLevel BidiLevelForReorder() const;

  UScriptCode GetScript() const { return script_; }
  const ComputedStyle* Style() const { return style_.get(); }
  LayoutObject* GetLayoutObject() const { return layout_object_; }

  void SetOffset(unsigned start, unsigned end);
  void SetEndOffset(unsigned);

  bool HasStartEdge() const;
  bool HasEndEdge() const;

  void SetStyleVariant(NGStyleVariant style_variant) {
    style_variant_ = static_cast<unsigned>(style_variant);
  }
  NGStyleVariant StyleVariant() const {
    return static_cast<NGStyleVariant>(style_variant_);
  }

  // Get or set the whitespace collapse type at the end of this item.
  NGCollapseType EndCollapseType() const {
    return static_cast<NGCollapseType>(end_collapse_type_);
  }
  void SetEndCollapseType(NGCollapseType type) { end_collapse_type_ = type; }

  // Whether the item may be affected by whitespace collapsing. Unlike the
  // EndCollapseType() method this returns true even if a trailing space has
  // been removed.
  bool EndMayCollapse() const { return end_may_collapse_; }

  static void Split(Vector<NGInlineItem>&, unsigned index, unsigned offset);
  void SetBidiLevel(UBiDiLevel);
  static unsigned SetBidiLevel(Vector<NGInlineItem>&,
                               unsigned index,
                               unsigned end_offset,
                               UBiDiLevel);

  void AssertOffset(unsigned offset) const;
  void AssertEndOffset(unsigned offset) const;

  String ToString() const;

 private:
  void ComputeBoxProperties();

  unsigned start_offset_;
  unsigned end_offset_;
  UScriptCode script_;
  scoped_refptr<const ShapeResult> shape_result_;
  scoped_refptr<const ComputedStyle> style_;
  LayoutObject* layout_object_;

  unsigned type_ : 4;
  unsigned bidi_level_ : 8;  // UBiDiLevel is defined as uint8_t.
  unsigned shape_options_ : 2;
  unsigned is_empty_item_ : 1;
  unsigned should_create_box_fragment_ : 1;
  unsigned style_variant_ : 2;
  unsigned end_collapse_type_ : 2;  // NGCollapseType
  unsigned end_may_collapse_ : 1;
  friend class NGInlineNode;
};

inline void NGInlineItem::AssertOffset(unsigned offset) const {
  DCHECK((offset >= start_offset_ && offset < end_offset_) ||
         (offset == start_offset_ && start_offset_ == end_offset_));
}

inline void NGInlineItem::AssertEndOffset(unsigned offset) const {
  DCHECK_GE(offset, start_offset_);
  DCHECK_LE(offset, end_offset_);
}

// Represents a text content with a list of NGInlineItem. A node may have an
// additional NGInlineItemsData for ::first-line pseudo element.
struct CORE_EXPORT NGInlineItemsData {
  // Text content for all inline items represented by a single NGInlineNode.
  // Encoded either as UTF-16 or latin-1 depending on the content.
  String text_content;
  Vector<NGInlineItem> items;

  // The DOM to text content offset mapping of this inline node.
  std::unique_ptr<NGOffsetMapping> offset_mapping;

  void AssertOffset(unsigned index, unsigned offset) const {
    items[index].AssertOffset(offset);
  }
  void AssertEndOffset(unsigned index, unsigned offset) const {
    items[index].AssertEndOffset(offset);
  }
};

}  // namespace blink

#endif  // NGInlineItem_h
