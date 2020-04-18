// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/sidebar/sidebar_widget.h"

#include "ash/message_center/message_center_view.h"
#include "ash/public/cpp/app_list/app_list_features.h"
#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/system/message_center/notification_tray.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/tray_constants.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_style.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

namespace ash {

namespace {

display::Display GetCurrentDisplay(aura::Window* window) {
  return display::Screen::GetScreen()->GetDisplayNearestWindow(window);
}

const SkColor kLightShadingColor = SkColorSetARGB(200, 0, 0, 0);

// The blur radius of the background.
constexpr int kSidebarBackgroundBlurRadius = 30;

gfx::Rect CalculateBounds(const gfx::Rect& display_bounds) {
  return gfx::Rect(display_bounds.x() + display_bounds.width() -
                       message_center::kNotificationWidth,
                   display_bounds.y(), message_center::kNotificationWidth,
                   display_bounds.height());
}

}  // namespace

class SidebarWidget::DelegateView : public views::WidgetDelegateView,
                                    public views::ButtonListener {
 public:
  DelegateView(Shelf* shelf, SidebarInitMode mode)
      : container_(new views::View()), background_(new views::View) {
    bool is_background_blur_enabled =
        app_list::features::IsBackgroundBlurEnabled();

    background_->SetPaintToLayer(ui::LAYER_SOLID_COLOR);
    background_->layer()->SetFillsBoundsOpaquely(false);
    background_->layer()->SetColor(kLightShadingColor);
    background_->layer()->SetOpacity(
        is_background_blur_enabled
            ? app_list::AppListView::kAppListOpacityWithBlur
            : app_list::AppListView::kAppListOpacity);
    if (is_background_blur_enabled)
      background_->layer()->SetBackgroundBlur(kSidebarBackgroundBlurRadius);

    auto* message_center = message_center::MessageCenter::Get();
    gfx::Rect display_bounds = shelf->GetUserWorkAreaBounds();
    bool initially_message_center_settings_visible =
        (mode == SidebarInitMode::MESSAGE_CENTER_SETTINGS);
    message_center_view_ =
        new MessageCenterView(message_center, display_bounds.height(),
                              initially_message_center_settings_visible);
    message_center_view_->SetNotifications(
        message_center->GetVisibleNotifications());

    auto container_layout =
        std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical);
    container_->SetPaintToLayer();
    container_->layer()->SetFillsBoundsOpaquely(false);
    container_->AddChildView(message_center_view_);
    container_layout->SetFlexForView(message_center_view_, 1);
    container_->SetLayoutManager(std::move(container_layout));

    AddChildView(background_);
    AddChildView(container_);
  }

  // Overridden from View:
  void Layout() override {
    DCHECK(message_center_view_);

    background_->SetBoundsRect(GetLocalBounds());
    container_->SetBoundsRect(GetLocalBounds());

    int message_center_height = height();
    message_center_view_->SetBounds(0, 0, width(), message_center_height);
  }

  // Overridden from ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {}

  ash::MessageCenterView* message_center_view() { return message_center_view_; }

 private:
  views::View* container_;
  views::View* background_;
  ash::MessageCenterView* message_center_view_;

  DISALLOW_COPY_AND_ASSIGN(DelegateView);
};

SidebarWidget::SidebarWidget(aura::Window* sidebar_container,
                             Sidebar* sidebar,
                             Shelf* shelf,
                             SidebarInitMode mode)
    : sidebar_(sidebar),
      shelf_(shelf),
      delegate_view_(new DelegateView(shelf, mode)) {
  DCHECK(sidebar_container);
  DCHECK(sidebar_);
  DCHECK(shelf_);

  display::Screen::GetScreen()->AddObserver(this);

  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.name = "SidebarWidget";
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.shadow_type = views::Widget::InitParams::SHADOW_TYPE_DROP;
  params.delegate = delegate_view_;
  params.parent = sidebar_container;
  Init(params);

  SetContentsView(delegate_view_);

  gfx::Rect display_bounds = shelf->GetUserWorkAreaBounds();
  SetBounds(CalculateBounds(display_bounds));
}

SidebarWidget::~SidebarWidget() {
  display::Screen::GetScreen()->RemoveObserver(this);
}

void SidebarWidget::Reinitialize(SidebarInitMode mode) {
  switch (mode) {
    case SidebarInitMode::NORMAL:
      delegate_view_->message_center_view()->SetSettingsVisible(false);
      break;
    case SidebarInitMode::MESSAGE_CENTER_SETTINGS:
      delegate_view_->message_center_view()->SetSettingsVisible(true);
      break;
  }
}

void SidebarWidget::OnDisplayAdded(const display::Display& new_display) {}

void SidebarWidget::OnDisplayRemoved(const display::Display& old_display) {}

void SidebarWidget::OnDisplayMetricsChanged(const display::Display& display,
                                            uint32_t metrics) {
  if (GetCurrentDisplay(GetNativeView()).id() == display.id()) {
    gfx::Rect display_bounds = shelf_->GetUserWorkAreaBounds();
    SetBounds(CalculateBounds(display_bounds));
  }
}

}  // namespace ash
