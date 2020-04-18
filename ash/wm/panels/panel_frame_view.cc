// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/panels/panel_frame_view.h"

#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/frame/default_frame_header.h"
#include "ash/frame/frame_border_hit_test.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/shell.h"
#include "ash/wm/resize_handle_window_targeter.h"
#include "ash/wm/window_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

// static
const char PanelFrameView::kViewClassName[] = "PanelFrameView";

PanelFrameView::PanelFrameView(views::Widget* frame, FrameType frame_type)
    : frame_(frame), caption_button_container_(nullptr), window_icon_(nullptr) {
  wm::InstallResizeHandleWindowTargeterForWindow(GetWidgetWindow(), nullptr);
  DCHECK(!frame_->widget_delegate()->CanMaximize());
  if (frame_type != FRAME_NONE)
    InitFrameHeader();
  Shell::Get()->AddShellObserver(this);
}

PanelFrameView::~PanelFrameView() {
  Shell::Get()->RemoveShellObserver(this);
}

void PanelFrameView::SetFrameColors(SkColor active_frame_color,
                                    SkColor inactive_frame_color) {
  frame_header_->SetFrameColors(active_frame_color, inactive_frame_color);
  GetWidgetWindow()->SetProperty(aura::client::kTopViewColor,
                                 inactive_frame_color);
}

const char* PanelFrameView::GetClassName() const {
  return kViewClassName;
}

void PanelFrameView::InitFrameHeader() {
  caption_button_container_ = new FrameCaptionButtonContainerView(frame_);
  AddChildView(caption_button_container_);

  frame_header_ = std::make_unique<DefaultFrameHeader>(
      frame_, this, caption_button_container_);
  GetWidgetWindow()->SetProperty(aura::client::kTopViewColor,
                                 kDefaultFrameColor);

  if (frame_->widget_delegate()->ShouldShowWindowIcon()) {
    window_icon_ = new views::ImageView();
    AddChildView(window_icon_);
    frame_header_->SetLeftHeaderView(window_icon_);
  }
}

aura::Window* PanelFrameView::GetWidgetWindow() {
  return frame_->GetNativeWindow();
}

int PanelFrameView::NonClientTopBorderHeight() const {
  if (!frame_header_)
    return 0;
  return frame_header_->GetHeaderHeightForPainting();
}

gfx::Size PanelFrameView::GetMinimumSize() const {
  if (!frame_header_)
    return gfx::Size();
  gfx::Size min_client_view_size(frame_->client_view()->GetMinimumSize());
  return gfx::Size(std::max(frame_header_->GetMinimumHeaderWidth(),
                            min_client_view_size.width()),
                   NonClientTopBorderHeight() + min_client_view_size.height());
}

void PanelFrameView::Layout() {
  if (!frame_header_)
    return;
  frame_header_->LayoutHeader();
  GetWidgetWindow()->SetProperty(aura::client::kTopViewInset,
                                 NonClientTopBorderHeight());
}

void PanelFrameView::GetWindowMask(const gfx::Size&, gfx::Path*) {
  // Nothing.
}

void PanelFrameView::ResetWindowControls() {
  NOTIMPLEMENTED();
}

void PanelFrameView::UpdateWindowIcon() {
  if (!window_icon_)
    return;
  views::WidgetDelegate* delegate = frame_->widget_delegate();
  if (delegate)
    window_icon_->SetImage(delegate->GetWindowIcon());
  window_icon_->SchedulePaint();
}

void PanelFrameView::UpdateWindowTitle() {
  if (!frame_header_)
    return;
  frame_header_->SchedulePaintForTitle();
}

void PanelFrameView::SizeConstraintsChanged() {}

gfx::Rect PanelFrameView::GetBoundsForClientView() const {
  gfx::Rect client_bounds = bounds();
  client_bounds.Inset(0, NonClientTopBorderHeight(), 0, 0);
  return client_bounds;
}

gfx::Rect PanelFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  gfx::Rect window_bounds = client_bounds;
  window_bounds.Inset(0, -NonClientTopBorderHeight(), 0, 0);
  return window_bounds;
}

int PanelFrameView::NonClientHitTest(const gfx::Point& point) {
  if (!frame_header_)
    return HTNOWHERE;
  return FrameBorderNonClientHitTest(this, nullptr, caption_button_container_,
                                     point);
}

void PanelFrameView::OnPaint(gfx::Canvas* canvas) {
  if (!frame_header_)
    return;
  bool paint_as_active = ShouldPaintAsActive();
  caption_button_container_->SetPaintAsActive(paint_as_active);

  FrameHeader::Mode header_mode =
      paint_as_active ? FrameHeader::MODE_ACTIVE : FrameHeader::MODE_INACTIVE;
  frame_header_->PaintHeader(canvas, header_mode);
}

///////////////////////////////////////////////////////////////////////////////
// PanelFrameView, ShellObserver overrides:

void PanelFrameView::OnOverviewModeStarting() {
  caption_button_container_->SetVisible(false);
}

void PanelFrameView::OnOverviewModeEnded() {
  caption_button_container_->SetVisible(true);
}

}  // namespace ash
