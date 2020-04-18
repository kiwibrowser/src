// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_scroll_into_view_params.h"

#include "third_party/blink/renderer/platform/wtf/size_assertions.h"

namespace blink {
namespace {
using AlignmentBehavior = WebScrollIntoViewParams::AlignmentBehavior;
using Alignment = WebScrollIntoViewParams::Alignment;
using Type = WebScrollIntoViewParams::Type;
using Behavior = WebScrollIntoViewParams::Behavior;

// Make sure we keep the public enums in sync with the internal ones.
ASSERT_SIZE(AlignmentBehavior, ScrollAlignmentBehavior)
STATIC_ASSERT_ENUM(AlignmentBehavior::kNoScroll,
                   ScrollAlignmentBehavior::kScrollAlignmentNoScroll);
STATIC_ASSERT_ENUM(AlignmentBehavior::kCenter,
                   ScrollAlignmentBehavior::kScrollAlignmentCenter);
STATIC_ASSERT_ENUM(AlignmentBehavior::kTop,
                   ScrollAlignmentBehavior::kScrollAlignmentTop);
STATIC_ASSERT_ENUM(AlignmentBehavior::kBottom,
                   ScrollAlignmentBehavior::kScrollAlignmentBottom);
STATIC_ASSERT_ENUM(AlignmentBehavior::kLeft,
                   ScrollAlignmentBehavior::kScrollAlignmentLeft);
STATIC_ASSERT_ENUM(AlignmentBehavior::kRight,
                   ScrollAlignmentBehavior::kScrollAlignmentRight);
STATIC_ASSERT_ENUM(AlignmentBehavior::kClosestEdge,
                   ScrollAlignmentBehavior::kScrollAlignmentClosestEdge);

ASSERT_SIZE(Type, ScrollType)
STATIC_ASSERT_ENUM(Type::kUser, ScrollType::kUserScroll);
STATIC_ASSERT_ENUM(Type::kProgrammatic, ScrollType::kProgrammaticScroll);
STATIC_ASSERT_ENUM(Type::kClamping, ScrollType::kClampingScroll);
STATIC_ASSERT_ENUM(Type::kAnchoring, ScrollType::kAnchoringScroll);
STATIC_ASSERT_ENUM(Type::kSequenced, ScrollType::kSequencedScroll);

ASSERT_SIZE(Behavior, ScrollBehavior)
STATIC_ASSERT_ENUM(Behavior::kAuto, ScrollBehavior::kScrollBehaviorAuto);
STATIC_ASSERT_ENUM(Behavior::kInstant, ScrollBehavior::kScrollBehaviorInstant);
STATIC_ASSERT_ENUM(Behavior::kSmooth, ScrollBehavior::kScrollBehaviorSmooth);

AlignmentBehavior ToAlignmentBehavior(
    ScrollAlignmentBehavior scroll_alignment_behavior) {
  return static_cast<AlignmentBehavior>(
      static_cast<int>(scroll_alignment_behavior));
}

ScrollAlignmentBehavior ToScrollAlignmentBehavior(
    AlignmentBehavior alignment_behavior) {
  return static_cast<ScrollAlignmentBehavior>(
      static_cast<int>(alignment_behavior));
}

ScrollAlignment ToScrollAlignment(Alignment alignment) {
  return {ToScrollAlignmentBehavior(alignment.rect_visible),
          ToScrollAlignmentBehavior(alignment.rect_hidden),
          ToScrollAlignmentBehavior(alignment.rect_partial)};
}

Type FromScrollType(ScrollType type) {
  return static_cast<Type>(static_cast<int>(type));
}

ScrollType ToScrollType(Type type) {
  return static_cast<ScrollType>(static_cast<int>(type));
}

Behavior FromScrollBehavior(ScrollBehavior behavior) {
  return static_cast<Behavior>(static_cast<int>(behavior));
}

ScrollBehavior ToScrollBehavior(Behavior behavior) {
  return static_cast<ScrollBehavior>(static_cast<int>(behavior));
}

}  // namespace

WebScrollIntoViewParams::Alignment::Alignment(
    const ScrollAlignment& scroll_alignment)
    : rect_visible(ToAlignmentBehavior(
          ScrollAlignment::GetVisibleBehavior(scroll_alignment))),
      rect_hidden(ToAlignmentBehavior(
          ScrollAlignment::GetHiddenBehavior(scroll_alignment))),
      rect_partial(ToAlignmentBehavior(
          ScrollAlignment::GetPartialBehavior(scroll_alignment))) {}

WebScrollIntoViewParams::WebScrollIntoViewParams(
    ScrollAlignment scroll_alignment_x,
    ScrollAlignment scroll_alignment_y,
    ScrollType scroll_type,
    bool make_visible_in_visual_viewport,
    ScrollBehavior scroll_behavior,
    bool is_for_scroll_sequence,
    bool zoom_into_rect)
    : align_x(scroll_alignment_x),
      align_y(scroll_alignment_y),
      type(FromScrollType(scroll_type)),
      make_visible_in_visual_viewport(make_visible_in_visual_viewport),
      behavior(FromScrollBehavior(scroll_behavior)),
      is_for_scroll_sequence(is_for_scroll_sequence),
      zoom_into_rect(zoom_into_rect) {}

ScrollAlignment WebScrollIntoViewParams::GetScrollAlignmentX() const {
  return ToScrollAlignment(align_x);
}

ScrollAlignment WebScrollIntoViewParams::GetScrollAlignmentY() const {
  return ToScrollAlignment(align_y);
}

ScrollType WebScrollIntoViewParams::GetScrollType() const {
  return ToScrollType(type);
}

ScrollBehavior WebScrollIntoViewParams::GetScrollBehavior() const {
  return ToScrollBehavior(behavior);
}

}  // namespace blink
