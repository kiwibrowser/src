// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/content_element.h"

#include "chrome/browser/vr/platform_ui_input_delegate.h"
#include "third_party/blink/public/platform/web_gesture_event.h"
#include "ui/gfx/geometry/rect_f.h"

namespace vr {

PlatformUiElement::PlatformUiElement() {
  set_scrollable(true);
}

PlatformUiElement::~PlatformUiElement() = default;

void PlatformUiElement::Render(UiElementRenderer* renderer,
                               const CameraModel& model) const {
  if (texture_id_) {
    renderer->DrawTexturedQuad(texture_id_, 0, texture_location_,
                               model.view_proj_matrix * world_space_transform(),
                               GetClipRect(), computed_opacity(), size(),
                               corner_radius(), true);
  }
}

void PlatformUiElement::OnHoverEnter(const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnHoverEnter(position);
}

void PlatformUiElement::OnHoverLeave() {
  if (delegate_)
    delegate_->OnHoverLeave();
}

void PlatformUiElement::OnMove(const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnMove(position);
}

void PlatformUiElement::OnButtonDown(const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnButtonDown(position);
}

void PlatformUiElement::OnButtonUp(const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnButtonUp(position);
}

void PlatformUiElement::OnFlingCancel(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnFlingCancel(std::move(gesture), position);
}

void PlatformUiElement::OnScrollBegin(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnScrollBegin(std::move(gesture), position);
}

void PlatformUiElement::OnScrollUpdate(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnScrollUpdate(std::move(gesture), position);
}

void PlatformUiElement::OnScrollEnd(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  if (delegate_)
    delegate_->OnScrollEnd(std::move(gesture), position);
}

void PlatformUiElement::SetTextureId(unsigned int texture_id) {
  texture_id_ = texture_id;
}

void PlatformUiElement::SetTextureLocation(
    UiElementRenderer::TextureLocation location) {
  texture_location_ = location;
}

void PlatformUiElement::SetDelegate(PlatformUiInputDelegate* delegate) {
  delegate_ = delegate;
}

}  // namespace vr
