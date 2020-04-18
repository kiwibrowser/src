/**
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/paint/theme_painter.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/forms/html_data_list_element.h"
#include "third_party/blink/renderer/core/html/forms/html_data_list_options_collection.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/forms/html_option_element.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html/shadow/shadow_element_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/fallback_theme.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/theme.h"
#include "ui/native_theme/native_theme.h"

// The methods in this file are shared by all themes on every platform.

namespace blink {

namespace {

ui::NativeTheme::State GetFallbackThemeState(const Node* node) {
  if (!LayoutTheme::IsEnabled(node))
    return ui::NativeTheme::kDisabled;
  if (LayoutTheme::IsPressed(node))
    return ui::NativeTheme::kPressed;
  if (LayoutTheme::IsHovered(node))
    return ui::NativeTheme::kHovered;

  return ui::NativeTheme::kNormal;
}

}  // anonymous namespace

ThemePainter::ThemePainter() = default;

bool ThemePainter::Paint(const LayoutObject& o,
                         const PaintInfo& paint_info,
                         const IntRect& r) {
  const Node* node = o.GetNode();
  const ComputedStyle& style = o.StyleRef();
  ControlPart part = o.StyleRef().Appearance();

  if (LayoutTheme::GetTheme().ShouldUseFallbackTheme(style))
    return PaintUsingFallbackTheme(node, style, paint_info, r);

  if (part == kButtonPart && node) {
    const Document& doc = node->GetDocument();
    UseCounter::Count(doc, WebFeature::kCSSValueAppearanceButtonRendered);
    if (IsHTMLAnchorElement(node)) {
      UseCounter::Count(doc, WebFeature::kCSSValueAppearanceButtonForAnchor);
    } else if (IsHTMLButtonElement(node)) {
      UseCounter::Count(doc, WebFeature::kCSSValueAppearanceButtonForButton);
    } else if (IsHTMLInputElement(node) &&
               ToHTMLInputElement(node)->IsTextButton()) {
      // Text buttons (type=button, reset, submit) has
      // -webkit-appearance:push-button by default.
      UseCounter::Count(doc,
                        WebFeature::kCSSValueAppearanceButtonForOtherButtons);
    }
  }

  // Call the appropriate paint method based off the appearance value.
  switch (part) {
    case kCheckboxPart:
      return PaintCheckbox(node, o.GetDocument(), style, paint_info, r);
    case kRadioPart:
      return PaintRadio(node, o.GetDocument(), style, paint_info, r);
    case kPushButtonPart:
    case kSquareButtonPart:
    case kButtonPart:
      return PaintButton(node, o.GetDocument(), style, paint_info, r);
    case kInnerSpinButtonPart:
      return PaintInnerSpinButton(node, style, paint_info, r);
    case kMenulistPart:
      return PaintMenuList(node, o.GetDocument(), style, paint_info, r);
    case kMeterPart:
      return true;
    case kProgressBarPart:
      return PaintProgressBar(o, paint_info, r);
    case kSliderHorizontalPart:
    case kSliderVerticalPart:
      return PaintSliderTrack(o, paint_info, r);
    case kSliderThumbHorizontalPart:
    case kSliderThumbVerticalPart:
      return PaintSliderThumb(node, style, paint_info, r);
    case kMediaEnterFullscreenButtonPart:
    case kMediaExitFullscreenButtonPart:
    case kMediaPlayButtonPart:
    case kMediaOverlayPlayButtonPart:
    case kMediaMuteButtonPart:
    case kMediaToggleClosedCaptionsButtonPart:
    case kMediaSliderPart:
    case kMediaSliderThumbPart:
    case kMediaVolumeSliderContainerPart:
    case kMediaVolumeSliderPart:
    case kMediaVolumeSliderThumbPart:
    case kMediaTimeRemainingPart:
    case kMediaCurrentTimePart:
    case kMediaControlsBackgroundPart:
    case kMediaCastOffButtonPart:
    case kMediaOverlayCastOffButtonPart:
    case kMediaTrackSelectionCheckmarkPart:
    case kMediaClosedCaptionsIconPart:
    case kMediaSubtitlesIconPart:
    case kMediaOverflowMenuButtonPart:
    case kMediaRemotingCastIconPart:
    case kMediaDownloadIconPart:
      return true;
    case kMenulistButtonPart:
    case kTextFieldPart:
    case kTextAreaPart:
      return true;
    case kSearchFieldPart:
      return PaintSearchField(node, style, paint_info, r);
    case kSearchFieldCancelButtonPart:
      return PaintSearchFieldCancelButton(o, paint_info, r);
    default:
      break;
  }

  // We don't support the appearance, so let the normal background/border paint.
  return true;
}

bool ThemePainter::PaintBorderOnly(const Node* node,
                                   const ComputedStyle& style,
                                   const PaintInfo& paint_info,
                                   const IntRect& r) {
  // Call the appropriate paint method based off the appearance value.
  switch (style.Appearance()) {
    case kTextFieldPart:
      if (node) {
        UseCounter::Count(node->GetDocument(),
                          WebFeature::kCSSValueAppearanceTextFieldRendered);
        if (auto* input = ToHTMLInputElementOrNull(node)) {
          if (input->type() == InputTypeNames::search) {
            UseCounter::Count(
                node->GetDocument(),
                WebFeature::kCSSValueAppearanceTextFieldForSearch);
          } else if (input->IsTextField()) {
            UseCounter::Count(
                node->GetDocument(),
                WebFeature::kCSSValueAppearanceTextFieldForTextField);
          }
        }
      }
      return PaintTextField(node, style, paint_info, r);
    case kTextAreaPart:
      return PaintTextArea(node, style, paint_info, r);
    case kMenulistButtonPart:
    case kSearchFieldPart:
    case kListboxPart:
      return true;
    case kCheckboxPart:
    case kRadioPart:
    case kPushButtonPart:
    case kSquareButtonPart:
    case kButtonPart:
    case kMenulistPart:
    case kMeterPart:
    case kProgressBarPart:
    case kSliderHorizontalPart:
    case kSliderVerticalPart:
    case kSliderThumbHorizontalPart:
    case kSliderThumbVerticalPart:
    case kSearchFieldCancelButtonPart:
    default:
      break;
  }

  return false;
}

bool ThemePainter::PaintDecorations(const Node* node,
                                    const Document& document,
                                    const ComputedStyle& style,
                                    const PaintInfo& paint_info,
                                    const IntRect& r) {
  // Call the appropriate paint method based off the appearance value.
  switch (style.Appearance()) {
    case kMenulistButtonPart:
      return PaintMenuListButton(node, document, style, paint_info, r);
    case kTextFieldPart:
    case kTextAreaPart:
    case kCheckboxPart:
    case kRadioPart:
    case kPushButtonPart:
    case kSquareButtonPart:
    case kButtonPart:
    case kMenulistPart:
    case kMeterPart:
    case kProgressBarPart:
    case kSliderHorizontalPart:
    case kSliderVerticalPart:
    case kSliderThumbHorizontalPart:
    case kSliderThumbVerticalPart:
    case kSearchFieldPart:
    case kSearchFieldCancelButtonPart:
    default:
      break;
  }

  return false;
}

void ThemePainter::PaintSliderTicks(const LayoutObject& o,
                                    const PaintInfo& paint_info,
                                    const IntRect& rect) {
  Node* node = o.GetNode();
  if (!IsHTMLInputElement(node))
    return;

  HTMLInputElement* input = ToHTMLInputElement(node);
  if (input->type() != InputTypeNames::range ||
      !input->UserAgentShadowRoot()->HasChildren())
    return;

  HTMLDataListElement* data_list = input->DataList();
  if (!data_list)
    return;

  double min = input->Minimum();
  double max = input->Maximum();
  ControlPart part = o.StyleRef().Appearance();
  // We don't support ticks on alternate sliders like MediaVolumeSliders.
  if (part != kSliderHorizontalPart && part != kSliderVerticalPart)
    return;
  bool is_horizontal = part == kSliderHorizontalPart;

  IntSize thumb_size;
  LayoutObject* thumb_layout_object =
      input->UserAgentShadowRoot()
          ->getElementById(ShadowElementNames::SliderThumb())
          ->GetLayoutObject();
  if (thumb_layout_object) {
    const ComputedStyle& thumb_style = thumb_layout_object->StyleRef();
    int thumb_width = thumb_style.Width().IntValue();
    int thumb_height = thumb_style.Height().IntValue();
    thumb_size.SetWidth(is_horizontal ? thumb_width : thumb_height);
    thumb_size.SetHeight(is_horizontal ? thumb_height : thumb_width);
  }

  IntSize tick_size = LayoutTheme::GetTheme().SliderTickSize();
  float zoom_factor = o.StyleRef().EffectiveZoom();
  FloatRect tick_rect;
  int tick_region_side_margin = 0;
  int tick_region_width = 0;
  IntRect track_bounds;
  LayoutObject* track_layout_object =
      input->UserAgentShadowRoot()
          ->getElementById(ShadowElementNames::SliderTrack())
          ->GetLayoutObject();
  // We can ignoring transforms because transform is handled by the graphics
  // context.
  if (track_layout_object)
    track_bounds =
        track_layout_object->AbsoluteBoundingBoxRectIgnoringTransforms();
  IntRect slider_bounds = o.AbsoluteBoundingBoxRectIgnoringTransforms();

  // Make position relative to the transformed ancestor element.
  track_bounds.SetX(track_bounds.X() - slider_bounds.X() + rect.X());
  track_bounds.SetY(track_bounds.Y() - slider_bounds.Y() + rect.Y());

  if (is_horizontal) {
    tick_rect.SetWidth(floor(tick_size.Width() * zoom_factor));
    tick_rect.SetHeight(floor(tick_size.Height() * zoom_factor));
    tick_rect.SetY(
        floor(rect.Y() + rect.Height() / 2.0 +
              LayoutTheme::GetTheme().SliderTickOffsetFromTrackCenter() *
                  zoom_factor));
    tick_region_side_margin =
        track_bounds.X() +
        (thumb_size.Width() - tick_size.Width() * zoom_factor) / 2.0;
    tick_region_width = track_bounds.Width() - thumb_size.Width();
  } else {
    tick_rect.SetWidth(floor(tick_size.Height() * zoom_factor));
    tick_rect.SetHeight(floor(tick_size.Width() * zoom_factor));
    tick_rect.SetX(
        floor(rect.X() + rect.Width() / 2.0 +
              LayoutTheme::GetTheme().SliderTickOffsetFromTrackCenter() *
                  zoom_factor));
    tick_region_side_margin =
        track_bounds.Y() +
        (thumb_size.Width() - tick_size.Width() * zoom_factor) / 2.0;
    tick_region_width = track_bounds.Height() - thumb_size.Width();
  }
  HTMLDataListOptionsCollection* options = data_list->options();
  for (unsigned i = 0; HTMLOptionElement* option_element = options->Item(i);
       i++) {
    String value = option_element->value();
    if (option_element->IsDisabledFormControl() || value.IsEmpty())
      continue;
    if (!input->IsValidValue(value))
      continue;
    double parsed_value =
        ParseToDoubleForNumberType(input->SanitizeValue(value));
    double tick_fraction = (parsed_value - min) / (max - min);
    double tick_ratio = is_horizontal && o.StyleRef().IsLeftToRightDirection()
                            ? tick_fraction
                            : 1.0 - tick_fraction;
    double tick_position =
        round(tick_region_side_margin + tick_region_width * tick_ratio);
    if (is_horizontal)
      tick_rect.SetX(tick_position);
    else
      tick_rect.SetY(tick_position);
    paint_info.context.FillRect(tick_rect,
                                o.ResolveColor(GetCSSPropertyColor()));
  }
}

bool ThemePainter::PaintUsingFallbackTheme(const Node* node,
                                           const ComputedStyle& style,
                                           const PaintInfo& paint_info,
                                           const IntRect& paint_rect) {
  ControlPart part = style.Appearance();
  switch (part) {
    case kCheckboxPart:
      return PaintCheckboxUsingFallbackTheme(node, style, paint_info,
                                             paint_rect);
    case kRadioPart:
      return PaintRadioUsingFallbackTheme(node, style, paint_info, paint_rect);
    default:
      break;
  }
  return true;
}

bool ThemePainter::PaintCheckboxUsingFallbackTheme(const Node* node,
                                                   const ComputedStyle& style,
                                                   const PaintInfo& paint_info,
                                                   const IntRect& paint_rect) {
  ui::NativeTheme::ExtraParams extra_params;
  extra_params.button.checked = LayoutTheme::IsChecked(node);
  extra_params.button.indeterminate = LayoutTheme::IsIndeterminate(node);

  float zoom_level = style.EffectiveZoom();
  GraphicsContextStateSaver state_saver(paint_info.context);
  IntRect unzoomed_rect = paint_rect;
  if (zoom_level != 1) {
    unzoomed_rect.SetWidth(unzoomed_rect.Width() / zoom_level);
    unzoomed_rect.SetHeight(unzoomed_rect.Height() / zoom_level);
    paint_info.context.Translate(unzoomed_rect.X(), unzoomed_rect.Y());
    paint_info.context.Scale(zoom_level, zoom_level);
    paint_info.context.Translate(-unzoomed_rect.X(), -unzoomed_rect.Y());
  }

  GetFallbackTheme().Paint(
      paint_info.context.Canvas(), ui::NativeTheme::kCheckbox,
      GetFallbackThemeState(node), unzoomed_rect, extra_params);
  return false;
}

bool ThemePainter::PaintRadioUsingFallbackTheme(const Node* node,
                                                const ComputedStyle& style,
                                                const PaintInfo& paint_info,
                                                const IntRect& paint_rect) {
  ui::NativeTheme::ExtraParams extra_params;
  extra_params.button.checked = LayoutTheme::IsChecked(node);

  float zoom_level = style.EffectiveZoom();
  GraphicsContextStateSaver state_saver(paint_info.context);
  IntRect unzoomed_rect = paint_rect;
  if (zoom_level != 1) {
    unzoomed_rect.SetWidth(unzoomed_rect.Width() / zoom_level);
    unzoomed_rect.SetHeight(unzoomed_rect.Height() / zoom_level);
    paint_info.context.Translate(unzoomed_rect.X(), unzoomed_rect.Y());
    paint_info.context.Scale(zoom_level, zoom_level);
    paint_info.context.Translate(-unzoomed_rect.X(), -unzoomed_rect.Y());
  }

  GetFallbackTheme().Paint(paint_info.context.Canvas(), ui::NativeTheme::kRadio,
                           GetFallbackThemeState(node), unzoomed_rect,
                           extra_params);
  return false;
}

}  // namespace blink
