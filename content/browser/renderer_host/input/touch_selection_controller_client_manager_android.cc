// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touch_selection_controller_client_manager_android.h"

#include "content/browser/renderer_host/render_widget_host_view_android.h"

namespace content {

TouchSelectionControllerClientManagerAndroid::
    TouchSelectionControllerClientManagerAndroid(
        RenderWidgetHostViewAndroid* rwhv)
    : rwhv_(rwhv), active_client_(rwhv), page_scale_factor_(1.f) {
  DCHECK(rwhv_);
}

TouchSelectionControllerClientManagerAndroid::
    ~TouchSelectionControllerClientManagerAndroid() {
  for (auto& observer : observers_)
    observer.OnManagerWillDestroy(this);
}

void TouchSelectionControllerClientManagerAndroid::SetPageScaleFactor(
    float page_scale_factor) {
  page_scale_factor_ = page_scale_factor;
}

namespace {

gfx::SelectionBound ScaleSelectionBound(const gfx::SelectionBound& bound,
                                        float scale) {
  gfx::SelectionBound scaled_bound;
  gfx::PointF scaled_top = bound.edge_top();
  scaled_top.Scale(scale);
  gfx::PointF scaled_bottom = bound.edge_bottom();
  scaled_bottom.Scale(scale);
  scaled_bound.SetEdge(scaled_top, scaled_bottom);
  scaled_bound.set_type(bound.type());
  scaled_bound.set_visible(bound.visible());
  return scaled_bound;
}
}  // namespace

// TouchSelectionControllerClientManager implementation.
void TouchSelectionControllerClientManagerAndroid::DidStopFlinging() {
  // TODO(wjmaclean): determine what, if anything, needs to happen here.
}

void TouchSelectionControllerClientManagerAndroid::UpdateClientSelectionBounds(
    const gfx::SelectionBound& start,
    const gfx::SelectionBound& end,
    ui::TouchSelectionControllerClient* client,
    ui::TouchSelectionMenuClient* menu_client) {
  if (client != active_client_ &&
      (start.type() == gfx::SelectionBound::EMPTY || !start.visible()) &&
      (end.type() == gfx::SelectionBound::EMPTY || !end.visible()) &&
      (manager_selection_start_.type() != gfx::SelectionBound::EMPTY ||
       manager_selection_end_.type() != gfx::SelectionBound::EMPTY)) {
    return;
  }

  active_client_ = client;
  if (active_client_ != rwhv_) {
    manager_selection_start_ = ScaleSelectionBound(start, page_scale_factor_);
    manager_selection_end_ = ScaleSelectionBound(end, page_scale_factor_);
  } else {
    manager_selection_start_ = start;
    manager_selection_end_ = end;
  }
  // Notify TouchSelectionController if anything should change here. Only
  // update if the client is different and not making a change to empty, or
  // is the same client.
  if (GetTouchSelectionController()) {
    GetTouchSelectionController()->OnSelectionBoundsChanged(
        manager_selection_start_, manager_selection_end_);
  }
}

void TouchSelectionControllerClientManagerAndroid::InvalidateClient(
    ui::TouchSelectionControllerClient* client) {
  if (active_client_ == client)
    active_client_ = rwhv_;
}

ui::TouchSelectionController*
TouchSelectionControllerClientManagerAndroid::GetTouchSelectionController() {
  return rwhv_->touch_selection_controller();
}

void TouchSelectionControllerClientManagerAndroid::AddObserver(
    Observer* observer) {
  observers_.AddObserver(observer);
}

void TouchSelectionControllerClientManagerAndroid::RemoveObserver(
    Observer* observer) {
  observers_.RemoveObserver(observer);
}

// TouchSelectionControllerClient implementation.
bool TouchSelectionControllerClientManagerAndroid::SupportsAnimation() const {
  return rwhv_->SupportsAnimation();
}

void TouchSelectionControllerClientManagerAndroid::SetNeedsAnimate() {
  rwhv_->SetNeedsAnimate();
}

void TouchSelectionControllerClientManagerAndroid::MoveCaret(
    const gfx::PointF& position) {
  gfx::PointF scaled_position = position;
  if (active_client_ != rwhv_)
    scaled_position.Scale(1 / page_scale_factor_);
  active_client_->MoveCaret(scaled_position);
}

void TouchSelectionControllerClientManagerAndroid::MoveRangeSelectionExtent(
    const gfx::PointF& extent) {
  gfx::PointF scaled_extent = extent;
  if (active_client_ != rwhv_)
    scaled_extent.Scale(1 / page_scale_factor_);
  active_client_->MoveRangeSelectionExtent(scaled_extent);
}

void TouchSelectionControllerClientManagerAndroid::SelectBetweenCoordinates(
    const gfx::PointF& base,
    const gfx::PointF& extent) {
  gfx::PointF scaled_extent = extent;
  gfx::PointF scaled_base = base;
  if (active_client_ != rwhv_) {
    scaled_extent.Scale(1 / page_scale_factor_);
    scaled_base.Scale(1 / page_scale_factor_);
  }
  active_client_->SelectBetweenCoordinates(scaled_base, scaled_extent);
}

void TouchSelectionControllerClientManagerAndroid::OnSelectionEvent(
    ui::SelectionEventType event) {
  // Always defer to the top-level RWHV TSC for this.
  rwhv_->OnSelectionEvent(event);
}

void TouchSelectionControllerClientManagerAndroid::OnDragUpdate(
    const gfx::PointF& position) {
  rwhv_->OnDragUpdate(position);
}

std::unique_ptr<ui::TouchHandleDrawable>
TouchSelectionControllerClientManagerAndroid::CreateDrawable() {
  return rwhv_->CreateDrawable();
}

void TouchSelectionControllerClientManagerAndroid::DidScroll() {
  // Nothing needs to be done here.
}

}  // namespace content
