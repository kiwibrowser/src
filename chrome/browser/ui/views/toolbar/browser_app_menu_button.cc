// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"

#include "base/location.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "cc/paint/paint_flags.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_otr_state.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/views/extensions/browser_action_drag_data.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/chromium_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/metrics.h"

namespace {

constexpr base::TimeDelta kDelayTime = base::TimeDelta::FromMilliseconds(1500);

}  // namespace

// static
bool BrowserAppMenuButton::g_open_app_immediately_for_testing = false;

BrowserAppMenuButton::BrowserAppMenuButton(ToolbarView* toolbar_view)
    : AppMenuButton(toolbar_view),
      toolbar_view_(toolbar_view),
      animation_delay_timer_(
          FROM_HERE,
          kDelayTime,
          base::BindRepeating(&BrowserAppMenuButton::AnimateIconIfPossible,
                              base::Unretained(this),
                              false),
          false) {
  SetInkDropMode(InkDropMode::ON);
  SetFocusPainter(nullptr);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);

  if (base::FeatureList::IsEnabled(features::kAnimatedAppMenuIcon)) {
    toolbar_view_->browser()->tab_strip_model()->AddObserver(this);
    should_use_new_icon_ = true;
    should_delay_animation_ = base::GetFieldTrialParamByFeatureAsBool(
        features::kAnimatedAppMenuIcon, "HasDelay", false);
  }

  if (ui::MaterialDesignController::IsTouchOptimizedUiEnabled())
    set_ink_drop_visible_opacity(kTouchToolbarInkDropVisibleOpacity);

  const int size = GetLayoutConstant(LOCATION_BAR_HEIGHT);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::EMPHASIS_HIGH, gfx::Size(size, size));
  set_ink_drop_corner_radii(radii, radii);
}

BrowserAppMenuButton::~BrowserAppMenuButton() {}

void BrowserAppMenuButton::SetSeverity(AppMenuIconController::IconType type,
                                       AppMenuIconController::Severity severity,
                                       bool animate) {
  type_ = type;
  severity_ = severity;

  SetTooltipText(
      severity_ == AppMenuIconController::Severity::NONE
          ? l10n_util::GetStringUTF16(IDS_APPMENU_TOOLTIP)
          : l10n_util::GetStringUTF16(IDS_APPMENU_TOOLTIP_UPDATE_AVAILABLE));
  UpdateIcon(animate);
}

void BrowserAppMenuButton::SetIsProminent(bool is_prominent) {
  if (is_prominent) {
    SetBackground(views::CreateSolidBackground(GetNativeTheme()->GetSystemColor(
        ui::NativeTheme::kColorId_ProminentButtonColor)));
  } else {
    SetBackground(nullptr);
  }
  SchedulePaint();
}

void BrowserAppMenuButton::ShowMenu(bool for_drop) {
  if (IsMenuShowing())
    return;

#if defined(USE_AURA)
  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  if (keyboard_controller && keyboard_controller->keyboard_visible()) {
    keyboard_controller->HideKeyboard(
        keyboard::KeyboardController::HIDE_REASON_AUTOMATIC);
  }
#endif

  Browser* browser = toolbar_view_->browser();

  InitMenu(std::make_unique<AppMenuModel>(toolbar_view_, browser), browser,
           for_drop ? AppMenu::FOR_DROP : AppMenu::NO_FLAGS);

  base::TimeTicks menu_open_time = base::TimeTicks::Now();
  menu()->RunMenu(this);

  if (!for_drop) {
    // Record the time-to-action for the menu. We don't record in the case of a
    // drag-and-drop command because menus opened for drag-and-drop don't block
    // the message loop.
    UMA_HISTOGRAM_TIMES("Toolbar.AppMenuTimeToAction",
                        base::TimeTicks::Now() - menu_open_time);
  }

  AnimateIconIfPossible(false);
}

gfx::Size BrowserAppMenuButton::CalculatePreferredSize() const {
  const int icon_size =
      ui::MaterialDesignController::IsTouchOptimizedUiEnabled() ? 24 : 16;
  gfx::Rect rect(gfx::Size(icon_size, icon_size));
  rect.Inset(-GetLayoutInsets(TOOLBAR_BUTTON));

  return rect.size();
}

void BrowserAppMenuButton::Layout() {
  if (new_icon_) {
    new_icon_->SetBoundsRect(GetContentsBounds());
    ink_drop_container()->SetBoundsRect(GetLocalBounds());
    image()->SetBoundsRect(GetContentsBounds());
    return;
  }

  views::MenuButton::Layout();
}

void BrowserAppMenuButton::OnThemeChanged() {
  UpdateIcon(false);
}

void BrowserAppMenuButton::TabInsertedAt(TabStripModel* tab_strip_model,
                                         content::WebContents* contents,
                                         int index,
                                         bool foreground) {
  AnimateIconIfPossible(true);
}

void BrowserAppMenuButton::UpdateIcon(bool should_animate) {
  SkColor severity_color = gfx::kPlaceholderColor;
  SkColor toolbar_icon_color =
      GetThemeProvider()->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  const ui::NativeTheme* native_theme = GetNativeTheme();
  switch (severity_) {
    case AppMenuIconController::Severity::NONE:
      severity_color = toolbar_icon_color;
      break;
    case AppMenuIconController::Severity::LOW:
      severity_color = native_theme->GetSystemColor(
          ui::NativeTheme::kColorId_AlertSeverityLow);
      break;
    case AppMenuIconController::Severity::MEDIUM:
      severity_color = native_theme->GetSystemColor(
          ui::NativeTheme::kColorId_AlertSeverityMedium);
      break;
    case AppMenuIconController::Severity::HIGH:
      severity_color = native_theme->GetSystemColor(
          ui::NativeTheme::kColorId_AlertSeverityHigh);
      break;
  }

  if (should_use_new_icon_) {
    if (!new_icon_) {
      new_icon_ = new views::AnimatedIconView(kBrowserToolsAnimatedIcon);
      new_icon_->set_can_process_events_within_subtree(false);
      AddChildView(new_icon_);
    }

    // Only show a special color for severity when using the classic Chrome
    // theme. Otherwise, we can't be sure that it contrasts with the toolbar
    // background.
    ThemeService* theme_service =
        ThemeServiceFactory::GetForProfile(toolbar_view_->browser()->profile());
    new_icon_->SetColor(theme_service->UsingSystemTheme() ||
                                theme_service->UsingDefaultTheme()
                            ? severity_color
                            : toolbar_icon_color);

    if (should_animate)
      AnimateIconIfPossible(true);

    return;
  }

  const bool is_touch =
      ui::MaterialDesignController::IsTouchOptimizedUiEnabled();
  const gfx::VectorIcon* icon_id = nullptr;
  switch (type_) {
    case AppMenuIconController::IconType::NONE:
      icon_id = is_touch ? &kBrowserToolsTouchIcon : &kBrowserToolsIcon;
      DCHECK_EQ(AppMenuIconController::Severity::NONE, severity_);
      break;
    case AppMenuIconController::IconType::UPGRADE_NOTIFICATION:
      icon_id =
          is_touch ? &kBrowserToolsUpdateTouchIcon : &kBrowserToolsUpdateIcon;
      break;
    case AppMenuIconController::IconType::GLOBAL_ERROR:
    case AppMenuIconController::IconType::INCOMPATIBILITY_WARNING:
      icon_id =
          is_touch ? &kBrowserToolsErrorTouchIcon : &kBrowserToolsErrorIcon;
      break;
  }

  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(*icon_id, severity_color));
}

void BrowserAppMenuButton::SetTrailingMargin(int margin) {
  margin_trailing_ = margin;
  UpdateThemedBorder();
  InvalidateLayout();
}

void BrowserAppMenuButton::AnimateIconIfPossible(bool with_delay) {
  if (!new_icon_ || !should_use_new_icon_ ||
      severity_ == AppMenuIconController::Severity::NONE) {
    return;
  }

  if (!should_delay_animation_ || !with_delay || new_icon_->IsAnimating()) {
    animation_delay_timer_.Stop();
    new_icon_->Animate(views::AnimatedIconView::END);
    return;
  }

  if (!animation_delay_timer_.IsRunning())
    animation_delay_timer_.Reset();
}

const char* BrowserAppMenuButton::GetClassName() const {
  return "BrowserAppMenuButton";
}

std::unique_ptr<views::LabelButtonBorder>
BrowserAppMenuButton::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      MenuButton::CreateDefaultBorder();

  // Adjust border insets to follow the margin change,
  // which will be reflected in where the border is painted
  // through GetThemePaintRect().
  gfx::Insets insets(border->GetInsets());
  insets += gfx::Insets(0, 0, 0, margin_trailing_);
  border->set_insets(insets);

  return border;
}

gfx::Rect BrowserAppMenuButton::GetThemePaintRect() const {
  gfx::Rect rect(MenuButton::GetThemePaintRect());
  rect.Inset(0, 0, margin_trailing_, 0);
  return rect;
}

bool BrowserAppMenuButton::GetDropFormats(
    int* formats,
    std::set<ui::Clipboard::FormatType>* format_types) {
  return BrowserActionDragData::GetDropFormats(format_types);
}

bool BrowserAppMenuButton::AreDropTypesRequired() {
  return BrowserActionDragData::AreDropTypesRequired();
}

bool BrowserAppMenuButton::CanDrop(const ui::OSExchangeData& data) {
  return BrowserActionDragData::CanDrop(data,
                                        toolbar_view_->browser()->profile());
}

void BrowserAppMenuButton::OnDragEntered(const ui::DropTargetEvent& event) {
  DCHECK(!weak_factory_.HasWeakPtrs());
  if (!g_open_app_immediately_for_testing) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&BrowserAppMenuButton::ShowMenu,
                       weak_factory_.GetWeakPtr(), true),
        base::TimeDelta::FromMilliseconds(views::GetMenuShowDelay()));
  } else {
    ShowMenu(true);
  }
}

int BrowserAppMenuButton::OnDragUpdated(const ui::DropTargetEvent& event) {
  return ui::DragDropTypes::DRAG_MOVE;
}

void BrowserAppMenuButton::OnDragExited() {
  weak_factory_.InvalidateWeakPtrs();
}

int BrowserAppMenuButton::OnPerformDrop(const ui::DropTargetEvent& event) {
  return ui::DragDropTypes::DRAG_MOVE;
}

std::unique_ptr<views::InkDrop> BrowserAppMenuButton::CreateInkDrop() {
  return CreateToolbarInkDrop<MenuButton>(this);
}

std::unique_ptr<views::InkDropRipple>
BrowserAppMenuButton::CreateInkDropRipple() const {
  // FIXME: GetInkDropCenterBasedOnLastEvent() will always return the center
  // of this view. https://crbug.com/819878.
  return CreateToolbarInkDropRipple<MenuButton>(
      this, GetInkDropCenterBasedOnLastEvent());
}

std::unique_ptr<views::InkDropHighlight>
BrowserAppMenuButton::CreateInkDropHighlight() const {
  return CreateToolbarInkDropHighlight<MenuButton>(
      this, GetMirroredRect(GetContentsBounds()).CenterPoint());
}

std::unique_ptr<views::InkDropMask> BrowserAppMenuButton::CreateInkDropMask()
    const {
  return CreateToolbarInkDropMask<MenuButton>(this);
}
