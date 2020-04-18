// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/test_system_tray_item.h"

#include "ash/test/ash_test_base.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace ash {

TestSystemTrayItem::TestSystemTrayItem() : TestSystemTrayItem(UMA_TEST) {}

TestSystemTrayItem::TestSystemTrayItem(SystemTrayItem::UmaType uma_type)
    : SystemTrayItem(AshTestBase::GetPrimarySystemTray(), uma_type),
      has_views_(true),
      views_are_visible_(true),
      tray_view_(nullptr),
      default_view_(nullptr),
      detailed_view_(nullptr) {}

TestSystemTrayItem::~TestSystemTrayItem() = default;

views::View* TestSystemTrayItem::CreateTrayView(LoginStatus status) {
  if (!has_views_) {
    tray_view_ = nullptr;
    return tray_view_;
  }
  tray_view_ = new views::View;
  // Add a label so it has non-zero width.
  tray_view_->SetLayoutManager(std::make_unique<views::FillLayout>());
  tray_view_->AddChildView(new views::Label(base::UTF8ToUTF16("Tray")));
  tray_view_->SetVisible(views_are_visible_);
  return tray_view_;
}

views::View* TestSystemTrayItem::CreateDefaultView(LoginStatus status) {
  if (!has_views_) {
    default_view_ = nullptr;
    return default_view_;
  }
  default_view_ = new views::View;
  default_view_->SetLayoutManager(std::make_unique<views::FillLayout>());
  default_view_->AddChildView(new views::Label(base::UTF8ToUTF16("Default")));
  default_view_->SetVisible(views_are_visible_);
  return default_view_;
}

views::View* TestSystemTrayItem::CreateDetailedView(LoginStatus status) {
  if (!has_views_) {
    detailed_view_ = nullptr;
    return detailed_view_;
  }
  detailed_view_ = new views::View;
  detailed_view_->SetLayoutManager(std::make_unique<views::FillLayout>());
  detailed_view_->AddChildView(new views::Label(base::UTF8ToUTF16("Detailed")));
  detailed_view_->SetVisible(views_are_visible_);
  return detailed_view_;
}

void TestSystemTrayItem::OnTrayViewDestroyed() {
  tray_view_ = nullptr;
}

void TestSystemTrayItem::OnDefaultViewDestroyed() {
  default_view_ = nullptr;
}

void TestSystemTrayItem::OnDetailedViewDestroyed() {
  detailed_view_ = nullptr;
}

void TestSystemTrayItem::UpdateAfterLoginStatusChange(LoginStatus status) {}

}  // namespace ash
