// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLineBreaker_h
#define NGLineBreaker_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/exclusions/ng_line_layout_opportunity.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item_result.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"
#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_shaper.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_spacing.h"
#include "third_party/blink/renderer/platform/text/text_break_iterator.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class Hyphenation;
class NGContainerFragmentBuilder;
class NGInlineBreakToken;
class NGInlineItem;
class NGInlineLayoutStateStack;
struct NGPositionedFloat;
struct NGUnpositionedFloat;

// The line breaker needs to know which mode its in to properly handle floats.
enum class NGLineBreakerMode { kContent, kMinContent, kMaxContent };

// Represents a line breaker.
//
// This class measures each NGInlineItem and determines items to form a line,
// so that NGInlineLayoutAlgorithm can build a line box from the output.
class CORE_EXPORT NGLineBreaker {
  STACK_ALLOCATED();

 public:
  NGLineBreaker(NGInlineNode,
                NGLineBreakerMode,
                const NGConstraintSpace&,
                Vector<NGPositionedFloat>*,
                Vector<scoped_refptr<NGUnpositionedFloat>>*,
                NGContainerFragmentBuilder* container_builder,
                NGExclusionSpace*,
                unsigned handled_float_index,
                const NGInlineBreakToken* = nullptr);
  ~NGLineBreaker();

  // Compute the next line break point and produces NGInlineItemResults for
  // the line.
  bool NextLine(const NGLineLayoutOpportunity& line_opportunity, NGLineInfo*);

  // Create an NGInlineBreakToken for the last line returned by NextLine().
  scoped_refptr<NGInlineBreakToken> CreateBreakToken(
      const NGLineInfo&,
      std::unique_ptr<const NGInlineLayoutStateStack>) const;

  // Compute NGInlineItemResult for an open tag item.
  // Returns true if this item has edge and may have non-zero inline size.
  static bool ComputeOpenTagResult(const NGInlineItem&,
                                   const NGConstraintSpace&,
                                   NGInlineItemResult*);

 private:
  // This struct holds information for the current line.
  struct LineData {
    STACK_ALLOCATED();

    LineData(NGInlineNode node, const NGInlineBreakToken* break_token);

    // The current position from inline_start. Unlike NGInlineLayoutAlgorithm
    // that computes position in visual order, this position in logical order.
    LayoutUnit position;

    NGLineLayoutOpportunity line_opportunity;

    // True if this line is the "first formatted line".
    // https://www.w3.org/TR/CSS22/selector.html#first-formatted-line
    bool is_first_formatted_line;

    bool use_first_line_style;

    // We don't create "certain zero-height line boxes".
    // https://drafts.csswg.org/css2/visuren.html#phantom-line-box
    // Such line boxes do not prevent two margins being "adjoining", and thus
    // collapsing.
    // https://drafts.csswg.org/css2/box.html#collapsing-margins
    bool should_create_line_box = false;

    // Set when the line ended with a forced break. Used to setup the states for
    // the next line.
    bool is_after_forced_break = false;

    LayoutUnit AvailableWidth() const {
      return line_opportunity.AvailableInlineSize();
    }
    bool CanFit() const { return position <= AvailableWidth(); }
    bool CanFit(LayoutUnit extra) const {
      return position + extra <= AvailableWidth();
    }
    bool CanFloatFit(LayoutUnit extra) const {
      return position + extra <= line_opportunity.AvailableFloatInlineSize();
    }
  };

  const String& Text() const { return items_data_.text_content; }
  const Vector<NGInlineItem>& Items() const { return items_data_.items; }

  NGInlineItemResult* AddItem(const NGInlineItem&,
                              unsigned end_offset,
                              NGInlineItemResults*);
  NGInlineItemResult* AddItem(const NGInlineItem&, NGInlineItemResults*);
  void SetLineEndFragment(scoped_refptr<NGPhysicalTextFragment>, NGLineInfo*);
  void ComputeCanBreakAfter(NGInlineItemResult*) const;

  void BreakLine(NGLineInfo*);

  void PrepareNextLine(const NGLineLayoutOpportunity&, NGLineInfo*);

  void UpdatePosition(const NGInlineItemResults&);
  void ComputeLineLocation(NGLineInfo*) const;

  enum class LineBreakState {
    // The line breaking is complete.
    kDone,

    // Should complete the line at the earliest possible point.
    // Trailing spaces, <br>, or close tags should be included to the line even
    // when it is overflowing.
    kTrailing,

    // The initial state. Looking for items to break the line.
    kContinue,
  };

  LineBreakState HandleText(const NGInlineItem&, LineBreakState, NGLineInfo*);
  void BreakText(NGInlineItemResult*,
                 const NGInlineItem&,
                 LayoutUnit available_width,
                 NGLineInfo*);
  LineBreakState HandleTrailingSpaces(const NGInlineItem&, NGLineInfo*);
  void RemoveTrailingCollapsibleSpace(NGLineInfo*);
  void AppendHyphen(const NGInlineItem& item, NGLineInfo*);

  LineBreakState HandleControlItem(const NGInlineItem&,
                                   LineBreakState,
                                   NGLineInfo*);
  LineBreakState HandleBidiControlItem(const NGInlineItem&,
                                       LineBreakState,
                                       NGLineInfo*);
  void HandleAtomicInline(const NGInlineItem&, NGLineInfo*);
  void HandleFloat(const NGInlineItem&, NGLineInfo*, NGInlineItemResult*);

  void HandleOpenTag(const NGInlineItem&, NGInlineItemResult*);
  void HandleCloseTag(const NGInlineItem&, NGInlineItemResults*);

  LineBreakState HandleOverflow(NGLineInfo*);
  LineBreakState HandleOverflow(NGLineInfo*, LayoutUnit available_width);
  void Rewind(NGLineInfo*, unsigned new_end);

  LayoutObject* CurrentLayoutObject(const NGLineInfo&) const;

  void SetCurrentStyle(const ComputedStyle&);

  void MoveToNextOf(const NGInlineItem&);
  void MoveToNextOf(const NGInlineItemResult&);

  void ComputeBaseDirection(const NGLineInfo&);
  bool IsTrailing(const NGInlineItem&, const NGLineInfo&) const;

  LineData line_;
  NGInlineNode node_;
  const NGInlineItemsData& items_data_;

  NGLineBreakerMode mode_;
  const NGConstraintSpace& constraint_space_;
  Vector<NGPositionedFloat>* positioned_floats_;
  Vector<scoped_refptr<NGUnpositionedFloat>>* unpositioned_floats_;
  NGContainerFragmentBuilder* container_builder_; /* May be nullptr */
  NGExclusionSpace* exclusion_space_;
  scoped_refptr<const ComputedStyle> current_style_;

  unsigned item_index_ = 0;
  unsigned offset_ = 0;
  LazyLineBreakIterator break_iterator_;
  HarfBuzzShaper shaper_;
  ShapeResultSpacing<String> spacing_;
  bool previous_line_had_forced_break_ = false;
  const Hyphenation* hyphenation_ = nullptr;

  // Keep track of handled float items. See HandleFloat().
  unsigned handled_floats_end_item_index_;

  // The current base direction for the bidi algorithm.
  // This is copied from NGInlineNode, then updated after each forced line break
  // if 'unicode-bidi: plaintext'.
  TextDirection base_direction_;

  // True when current box allows line wrapping.
  bool auto_wrap_ = false;

  // True when current box has 'word-break/word-wrap: break-word'.
  bool break_anywhere_if_overflow_ = false;

  // Force LineBreakType::kBreakCharacter by ignoring the current style.
  // Set to find grapheme cluster boundaries for 'break-word' after overflow.
  bool override_break_anywhere_ = false;

  // True when breaking at soft hyphens (U+00AD) is allowed.
  bool enable_soft_hyphen_ = true;

  // True in quirks mode or limited-quirks mode, which require line-height
  // quirks.
  // https://quirks.spec.whatwg.org/#the-line-height-calculation-quirk
  bool in_line_height_quirks_mode_ = false;

  bool ignore_floats_ = false;
};

}  // namespace blink

#endif  // NGLineBreaker_h
