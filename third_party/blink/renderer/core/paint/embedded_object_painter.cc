// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/embedded_object_painter.h"

#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_object.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/fonts/font_selector.h"
#include "third_party/blink/renderer/platform/fonts/text_run_paint_info.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/path.h"
#include "third_party/blink/renderer/platform/text/text_run.h"

namespace blink {

static const float kReplacementTextRoundedRectHeight = 18;
static const float kReplacementTextRoundedRectLeftRightTextMargin = 6;
static const float kReplacementTextRoundedRectOpacity = 0.20f;
static const float kReplacementTextRoundedRectRadius = 5;
static const float kReplacementTextTextOpacity = 0.55f;

static Font ReplacementTextFont() {
  FontDescription font_description;
  LayoutTheme::GetTheme().SystemFont(CSSValueWebkitSmallControl,
                                     font_description);
  font_description.SetWeight(BoldWeightValue());
  font_description.SetComputedSize(font_description.SpecifiedSize());
  Font font(font_description);
  font.Update(nullptr);
  return font;
}

void EmbeddedObjectPainter::PaintReplaced(const PaintInfo& paint_info,
                                          const LayoutPoint& paint_offset) {
  if (!layout_embedded_object_.ShowsUnavailablePluginIndicator())
    return;

  if (paint_info.phase == PaintPhase::kSelection)
    return;

  GraphicsContext& context = paint_info.context;
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, layout_embedded_object_, paint_info.phase))
    return;

  LayoutRect content_rect(layout_embedded_object_.ContentBoxRect());
  content_rect.MoveBy(paint_offset);
  DrawingRecorder recorder(context, layout_embedded_object_, paint_info.phase);
  GraphicsContextStateSaver state_saver(context);
  context.Clip(PixelSnappedIntRect(content_rect));

  Font font = ReplacementTextFont();
  const SimpleFontData* font_data = font.PrimaryFont();
  DCHECK(font_data);
  if (!font_data)
    return;

  TextRun text_run(layout_embedded_object_.UnavailablePluginReplacementText());
  FloatSize text_geometry(font.Width(text_run),
                          font_data->GetFontMetrics().Height());

  LayoutRect background_rect(
      0, 0,
      text_geometry.Width() +
          2 * kReplacementTextRoundedRectLeftRightTextMargin,
      kReplacementTextRoundedRectHeight);
  background_rect.Move(content_rect.Center() - background_rect.Center());
  background_rect = LayoutRect(PixelSnappedIntRect(background_rect));
  Path rounded_background_rect;
  FloatRect float_background_rect(background_rect);
  rounded_background_rect.AddRoundedRect(
      float_background_rect, FloatSize(kReplacementTextRoundedRectRadius,
                                       kReplacementTextRoundedRectRadius));
  context.SetFillColor(
      ScaleAlpha(Color::kWhite, kReplacementTextRoundedRectOpacity));
  context.FillPath(rounded_background_rect);

  FloatRect text_rect(FloatPoint(), text_geometry);
  text_rect.Move(FloatPoint(content_rect.Center()) - text_rect.Center());
  TextRunPaintInfo run_info(text_run);
  run_info.bounds = float_background_rect;
  context.SetFillColor(ScaleAlpha(Color::kBlack, kReplacementTextTextOpacity));
  context.DrawBidiText(font, run_info,
                       text_rect.Location() +
                           FloatSize(0, font_data->GetFontMetrics().Ascent()));
}

}  // namespace blink
