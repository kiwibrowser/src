// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/devtools/render_widget_screen_metrics_emulator.h"

#include "content/common/visual_properties.h"
#include "content/public/common/context_menu_params.h"
#include "content/renderer/devtools/render_widget_screen_metrics_emulator_delegate.h"

namespace content {

RenderWidgetScreenMetricsEmulator::RenderWidgetScreenMetricsEmulator(
    RenderWidgetScreenMetricsEmulatorDelegate* delegate,
    const blink::WebDeviceEmulationParams& params,
    const VisualProperties& visual_properties,
    const gfx::Rect& view_screen_rect,
    const gfx::Rect& window_screen_rect)
    : delegate_(delegate),
      emulation_params_(params),
      scale_(1.f),
      original_visual_properties_(visual_properties),
      original_view_screen_rect_(view_screen_rect),
      original_window_screen_rect_(window_screen_rect) {}

RenderWidgetScreenMetricsEmulator::~RenderWidgetScreenMetricsEmulator() {
  delegate_->SynchronizeVisualProperties(original_visual_properties_);
  delegate_->SetScreenMetricsEmulationParameters(false, emulation_params_);
  delegate_->SetScreenRects(original_view_screen_rect_,
                            original_window_screen_rect_);
}

void RenderWidgetScreenMetricsEmulator::ChangeEmulationParams(
    const blink::WebDeviceEmulationParams& params) {
  emulation_params_ = params;
  Apply();
}

void RenderWidgetScreenMetricsEmulator::Apply() {
  VisualProperties modified_visual_properties = original_visual_properties_;
  applied_widget_rect_.set_size(gfx::Size(emulation_params_.view_size));

  if (!applied_widget_rect_.width())
    applied_widget_rect_.set_width(original_size().width());

  if (!applied_widget_rect_.height())
    applied_widget_rect_.set_height(original_size().height());

  scale_ = emulation_params_.scale;
  if (!emulation_params_.view_size.width &&
      !emulation_params_.view_size.height && scale_) {
    applied_widget_rect_.set_size(
        gfx::ScaleToRoundedSize(original_size(), 1.f / scale_));
  }

  gfx::Rect window_screen_rect;
  if (emulation_params_.screen_position ==
      blink::WebDeviceEmulationParams::kDesktop) {
    modified_visual_properties.screen_info.rect = original_screen_info().rect;
    modified_visual_properties.screen_info.available_rect =
        original_screen_info().available_rect;
    window_screen_rect = original_window_screen_rect_;
    if (emulation_params_.view_position) {
      applied_widget_rect_.set_origin(*emulation_params_.view_position);
      window_screen_rect.set_origin(*emulation_params_.view_position);
    } else {
      applied_widget_rect_.set_origin(original_view_screen_rect_.origin());
    }
  } else {
    applied_widget_rect_.set_origin(
        emulation_params_.view_position.value_or(blink::WebPoint()));
    modified_visual_properties.screen_info.rect = applied_widget_rect_;
    modified_visual_properties.screen_info.available_rect =
        applied_widget_rect_;
    window_screen_rect = applied_widget_rect_;
  }

  if (!emulation_params_.screen_size.IsEmpty()) {
    gfx::Rect screen_rect = gfx::Rect(0, 0, emulation_params_.screen_size.width,
                                      emulation_params_.screen_size.height);
    modified_visual_properties.screen_info.rect = screen_rect;
    modified_visual_properties.screen_info.available_rect = screen_rect;
  }

  modified_visual_properties.screen_info.device_scale_factor =
      emulation_params_.device_scale_factor
          ? emulation_params_.device_scale_factor
          : original_screen_info().device_scale_factor;

  if (emulation_params_.screen_orientation_type !=
      blink::kWebScreenOrientationUndefined) {
    switch (emulation_params_.screen_orientation_type) {
      case blink::kWebScreenOrientationPortraitPrimary:
        modified_visual_properties.screen_info.orientation_type =
            SCREEN_ORIENTATION_VALUES_PORTRAIT_PRIMARY;
        break;
      case blink::kWebScreenOrientationPortraitSecondary:
        modified_visual_properties.screen_info.orientation_type =
            SCREEN_ORIENTATION_VALUES_PORTRAIT_SECONDARY;
        break;
      case blink::kWebScreenOrientationLandscapePrimary:
        modified_visual_properties.screen_info.orientation_type =
            SCREEN_ORIENTATION_VALUES_LANDSCAPE_PRIMARY;
        break;
      case blink::kWebScreenOrientationLandscapeSecondary:
        modified_visual_properties.screen_info.orientation_type =
            SCREEN_ORIENTATION_VALUES_LANDSCAPE_SECONDARY;
        break;
      default:
        modified_visual_properties.screen_info.orientation_type =
            SCREEN_ORIENTATION_VALUES_DEFAULT;
        break;
    }
    modified_visual_properties.screen_info.orientation_angle =
        emulation_params_.screen_orientation_angle;
  }

  // Pass three emulation parameters to the blink side:
  // - we keep the real device scale factor in compositor to produce sharp image
  //   even when emulating different scale factor;
  // - in order to fit into view, WebView applies offset and scale to the
  //   root layer.
  blink::WebDeviceEmulationParams modified_emulation_params = emulation_params_;
  modified_emulation_params.device_scale_factor =
      original_screen_info().device_scale_factor;
  modified_emulation_params.scale = scale_;
  delegate_->SetScreenMetricsEmulationParameters(true,
                                                 modified_emulation_params);

  delegate_->SetScreenRects(applied_widget_rect_, window_screen_rect);

  modified_visual_properties.new_size = applied_widget_rect_.size();
  modified_visual_properties.visible_viewport_size =
      applied_widget_rect_.size();
  delegate_->SynchronizeVisualProperties(modified_visual_properties);
}

void RenderWidgetScreenMetricsEmulator::OnSynchronizeVisualProperties(
    const VisualProperties& params) {
  original_visual_properties_ = params;
  Apply();

  delegate_->Redraw();
}

void RenderWidgetScreenMetricsEmulator::OnUpdateWindowScreenRect(
    const gfx::Rect& window_screen_rect) {
  original_window_screen_rect_ = window_screen_rect;
  if (emulation_params_.screen_position ==
      blink::WebDeviceEmulationParams::kDesktop)
    Apply();
}

void RenderWidgetScreenMetricsEmulator::OnUpdateScreenRects(
    const gfx::Rect& view_screen_rect,
    const gfx::Rect& window_screen_rect) {
  original_view_screen_rect_ = view_screen_rect;
  original_window_screen_rect_ = window_screen_rect;
  if (emulation_params_.screen_position ==
      blink::WebDeviceEmulationParams::kDesktop) {
    Apply();
  }
}

void RenderWidgetScreenMetricsEmulator::OnShowContextMenu(
    ContextMenuParams* params) {
  params->x *= scale_;
  params->y *= scale_;
}

gfx::Rect RenderWidgetScreenMetricsEmulator::AdjustValidationMessageAnchor(
    const gfx::Rect& anchor) {
  return gfx::ScaleToEnclosedRect(anchor, scale_);
}

}  // namespace content
