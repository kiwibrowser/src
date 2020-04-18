// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/style/cached_ua_style.h"

#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

bool CachedUAStyle::BorderColorEquals(const ComputedStyle& other) const {
  return (border_left_color == other.BorderLeftColorInternal() &&
          border_right_color == other.BorderRightColorInternal() &&
          border_top_color == other.BorderTopColorInternal() &&
          border_bottom_color == other.BorderBottomColorInternal()) &&
         (border_left_color_is_current_color ==
              other.BorderLeftColorIsCurrentColor() &&
          border_right_color_is_current_color ==
              other.BorderRightColorIsCurrentColor() &&
          border_top_color_is_current_color ==
              other.BorderTopColorIsCurrentColor() &&
          border_bottom_color_is_current_color ==
              other.BorderBottomColorIsCurrentColor());
}

bool CachedUAStyle::BorderWidthEquals(const ComputedStyle& other) const {
  return (border_left_width == other.BorderLeftWidth() &&
          border_right_width == other.BorderRightWidth() &&
          border_top_width == other.BorderTopWidth() &&
          border_bottom_width == other.BorderBottomWidth());
}

bool CachedUAStyle::BorderRadiiEquals(const ComputedStyle& other) const {
  return top_left_ == other.BorderTopLeftRadius() &&
         top_right_ == other.BorderTopRightRadius() &&
         bottom_left_ == other.BorderBottomLeftRadius() &&
         bottom_right_ == other.BorderBottomRightRadius();
}

bool CachedUAStyle::BorderStyleEquals(const ComputedStyle& other) const {
  return (
      border_left_style == static_cast<unsigned>(other.BorderLeftStyle()) &&
      border_right_style == static_cast<unsigned>(other.BorderRightStyle()) &&
      border_top_style == static_cast<unsigned>(other.BorderTopStyle()) &&
      border_bottom_style == static_cast<unsigned>(other.BorderBottomStyle()));
}

CachedUAStyle::CachedUAStyle(const ComputedStyle* style)
    : top_left_(style->BorderTopLeftRadius()),
      top_right_(style->BorderTopRightRadius()),
      bottom_left_(style->BorderBottomLeftRadius()),
      bottom_right_(style->BorderBottomRightRadius()),
      border_left_color(style->BorderLeftColorInternal()),
      border_right_color(style->BorderRightColorInternal()),
      border_top_color(style->BorderTopColorInternal()),
      border_bottom_color(style->BorderBottomColorInternal()),
      border_left_color_is_current_color(
          style->BorderLeftColorIsCurrentColor()),
      border_right_color_is_current_color(
          style->BorderRightColorIsCurrentColor()),
      border_top_color_is_current_color(style->BorderTopColorIsCurrentColor()),
      border_bottom_color_is_current_color(
          style->BorderBottomColorIsCurrentColor()),
      border_left_style(static_cast<unsigned>(style->BorderLeftStyle())),
      border_right_style(static_cast<unsigned>(style->BorderRightStyle())),
      border_top_style(static_cast<unsigned>(style->BorderTopStyle())),
      border_bottom_style(static_cast<unsigned>(style->BorderBottomStyle())),
      border_left_width(style->BorderLeftWidth()),
      border_right_width(style->BorderRightWidth()),
      border_top_width(style->BorderTopWidth()),
      border_bottom_width(style->BorderBottomWidth()),
      border_image(style->BorderImage()),
      background_layers(style->BackgroundLayers()),
      background_color(style->BackgroundColor()) {}

}  // namespace blink
