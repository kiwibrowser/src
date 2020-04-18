// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/screen_security/screen_tray_item.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_item_style.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {
namespace tray {

// ScreenTrayView implementations.
ScreenTrayView::ScreenTrayView(ScreenTrayItem* screen_tray_item)
    : TrayItemView(screen_tray_item), screen_tray_item_(screen_tray_item) {
  CreateImageView();
  image_view()->SetImage(
      gfx::CreateVectorIcon(kSystemTrayScreenShareIcon, kTrayIconColor));
  Update();
}

ScreenTrayView::~ScreenTrayView() = default;

void ScreenTrayView::Update() {
  SetVisible(screen_tray_item_->is_started());
}

// ScreenStatusView implementations.
ScreenStatusView::ScreenStatusView(ScreenTrayItem* screen_tray_item,
                                   const base::string16& label_text,
                                   const base::string16& stop_button_text)
    : screen_tray_item_(screen_tray_item),
      icon_(nullptr),
      label_(nullptr),
      stop_button_(nullptr),
      label_text_(label_text),
      stop_button_text_(stop_button_text) {
  CreateItems();
  TriView* tri_view(TrayPopupUtils::CreateDefaultRowView());
  SetLayoutManager(std::make_unique<views::FillLayout>());
  AddChildView(tri_view);
  tri_view->AddView(TriView::Container::START, icon_);
  tri_view->AddView(TriView::Container::CENTER, label_);
  tri_view->AddView(TriView::Container::END, stop_button_);
  // There should be |kTrayPopupButtonEndMargin| padding on both sides of the
  // stop button. There is already |kTrayPopupLabelHorizontalPadding| padding on
  // the right of the label.
  tri_view->SetContainerBorder(
      TriView::Container::END,
      views::CreateEmptyBorder(
          0, kTrayPopupButtonEndMargin - kTrayPopupLabelHorizontalPadding, 0,
          kTrayPopupButtonEndMargin));
  if (screen_tray_item_)
    UpdateFromScreenTrayItem();
}

ScreenStatusView::~ScreenStatusView() = default;

void ScreenStatusView::ButtonPressed(views::Button* sender,
                                     const ui::Event& event) {
  DCHECK(sender == stop_button_);
  screen_tray_item_->Stop();
  screen_tray_item_->RecordStoppedFromDefaultViewMetric();
}

void ScreenStatusView::CreateItems() {
  icon_ = TrayPopupUtils::CreateMainImageView();
  icon_->SetImage(gfx::CreateVectorIcon(
      kSystemMenuScreenShareIcon, TrayPopupItemStyle::GetIconColor(
                                      TrayPopupItemStyle::ColorStyle::ACTIVE)));
  label_ = TrayPopupUtils::CreateDefaultLabel();
  label_->SetMultiLine(true);
  label_->SetText(label_text_);
  TrayPopupItemStyle style(TrayPopupItemStyle::FontStyle::DEFAULT_VIEW_LABEL);
  style.SetupLabel(label_);

  stop_button_ = TrayPopupUtils::CreateTrayPopupButton(this, stop_button_text_);
}

void ScreenStatusView::UpdateFromScreenTrayItem() {
  SetVisible(screen_tray_item_->is_started());
}

}  // namespace tray

ScreenTrayItem::ScreenTrayItem(SystemTray* system_tray, UmaType uma_type)
    : SystemTrayItem(system_tray, uma_type),
      tray_view_(nullptr),
      default_view_(nullptr),
      is_started_(false) {}

ScreenTrayItem::~ScreenTrayItem() = default;

void ScreenTrayItem::SetIsStarted(bool is_started) {
  if (!is_started)
    stop_callback_.Reset();
  is_started_ = is_started;
}

void ScreenTrayItem::Update() {
  if (tray_view_)
    tray_view_->Update();
  if (default_view_)
    default_view_->UpdateFromScreenTrayItem();
}

void ScreenTrayItem::Start(base::OnceClosure stop_callback) {
  stop_callback_ = std::move(stop_callback);
  is_started_ = true;

  if (tray_view_)
    tray_view_->Update();

  if (default_view_)
    default_view_->UpdateFromScreenTrayItem();
}

void ScreenTrayItem::Stop() {
  is_started_ = false;
  Update();

  if (stop_callback_.is_null())
    return;

  std::move(stop_callback_).Run();
}

views::View* ScreenTrayItem::CreateTrayView(LoginStatus status) {
  tray_view_ = new tray::ScreenTrayView(this);
  return tray_view_;
}

void ScreenTrayItem::OnTrayViewDestroyed() {
  tray_view_ = nullptr;
}

void ScreenTrayItem::OnDefaultViewDestroyed() {
  default_view_ = nullptr;
}

}  // namespace ash
