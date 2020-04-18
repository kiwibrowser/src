// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/canvas/text_metrics.h"

namespace blink {

constexpr int kHangingAsPercentOfAscent = 80;

float TextMetrics::GetFontBaseline(const TextBaseline& text_baseline,
                                   const FontMetrics& font_metrics) {
  // If the font is so tiny that the lroundf operations result in two
  // different types of text baselines to return the same baseline, use
  // floating point metrics (crbug.com/338908).
  // If you changed the heuristic here, for consistency please also change it
  // in SimpleFontData::platformInit().
  // TODO(fserb): revisit this.
  bool use_float_ascent_descent =
      font_metrics.Ascent() < 3 || font_metrics.Height() < 2;
  switch (text_baseline) {
    case kTopTextBaseline:
      return use_float_ascent_descent ? font_metrics.FloatAscent()
                                      : font_metrics.Ascent();
    case kHangingTextBaseline:
      // According to
      // http://wiki.apache.org/xmlgraphics-fop/LineLayout/AlignmentHandling
      // "FOP (Formatting Objects Processor) puts the hanging baseline at 80% of
      // the ascender height"
      return use_float_ascent_descent
                 ? font_metrics.FloatAscent() * kHangingAsPercentOfAscent / 100
                 : font_metrics.Ascent() * kHangingAsPercentOfAscent / 100;
    case kBottomTextBaseline:
    case kIdeographicTextBaseline:
      return use_float_ascent_descent ? -font_metrics.FloatDescent()
                                      : -font_metrics.Descent();
    case kMiddleTextBaseline:
      return use_float_ascent_descent
                 ? -font_metrics.FloatDescent() +
                       font_metrics.FloatHeight() / 2.0f
                 : -font_metrics.Descent() + font_metrics.Height() / 2;
    case kAlphabeticTextBaseline:
    default:
      // Do nothing.
      break;
  }
  return 0;
}

void TextMetrics::Update(const Font& font,
                         const TextDirection& direction,
                         const TextBaseline& baseline,
                         const TextAlign& align,
                         const String& text) {
  const SimpleFontData* font_data = font.PrimaryFont();
  if (!font_data)
    return;

  TextRun text_run(
      text, /* xpos */ 0, /* expansion */ 0,
      TextRun::kAllowTrailingExpansion | TextRun::kForbidLeadingExpansion,
      direction, false);
  text_run.SetNormalizeSpace(true);
  FloatRect bbox = font.BoundingBox(text_run);
  const FontMetrics& font_metrics = font_data->GetFontMetrics();

  // x direction
  width_ = bbox.Width();

  float dx = 0.0f;
  if (align == kCenterTextAlign)
    dx = -width_ / 2.0f;
  else if (align == kRightTextAlign ||
           (align == kStartTextAlign && direction == TextDirection::kRtl) ||
           (align == kEndTextAlign && direction != TextDirection::kRtl))
    dx = -width_;
  actual_bounding_box_left_ = -bbox.X() - dx;
  actual_bounding_box_right_ = bbox.MaxX() + dx;

  // y direction
  const float ascent = font_metrics.FloatAscent();
  const float descent = font_metrics.FloatDescent();
  const float baseline_y = GetFontBaseline(baseline, font_metrics);

  font_bounding_box_ascent_ = ascent - baseline_y;
  font_bounding_box_descent_ = descent + baseline_y;
  actual_bounding_box_ascent_ = -bbox.Y() - baseline_y;
  actual_bounding_box_descent_ = bbox.MaxY() + baseline_y;

  // it's not clear where the baseline for the em rect is.
  // We could try to render a letter that has 1em height and try to figure out.
  // But for now, just ignore descent for em.
  em_height_ascent_ = font_metrics.Height() - baseline_y;
  em_height_descent_ = baseline_y;

  // TODO(fserb): hanging/ideographic baselines are broken.
  hanging_baseline_ = ascent * kHangingAsPercentOfAscent / 100.0f - baseline_y;
  ideographic_baseline_ = -descent - baseline_y;
  alphabetic_baseline_ = -baseline_y;
}

}  // namespace blink
