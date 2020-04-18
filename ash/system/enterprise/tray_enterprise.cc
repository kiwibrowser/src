// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/enterprise/tray_enterprise.h"

#include "ash/ash_view_ids.h"
#include "ash/login_status.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/model/enterprise_domain_model.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {
namespace {

base::string16 GetEnterpriseMessage() {
  EnterpriseDomainModel* model =
      Shell::Get()->system_tray_model()->enterprise_domain();

  // Active Directory devices do not show a domain name.
  if (model->active_directory_managed())
    return l10n_util::GetStringUTF16(IDS_ASH_ENTERPRISE_DEVICE_MANAGED);

  if (!model->enterprise_display_domain().empty()) {
    return l10n_util::GetStringFUTF16(
        IDS_ASH_ENTERPRISE_DEVICE_MANAGED_BY,
        base::UTF8ToUTF16(model->enterprise_display_domain()));
  }
  return base::string16();
}

}  // namespace

TrayEnterprise::TrayEnterprise(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_ENTERPRISE), tray_view_(nullptr) {
  Shell::Get()->system_tray_model()->enterprise_domain()->AddObserver(this);
}

TrayEnterprise::~TrayEnterprise() {
  Shell::Get()->system_tray_model()->enterprise_domain()->RemoveObserver(this);
}

void TrayEnterprise::UpdateEnterpriseMessage() {
  if (tray_view_)
    tray_view_->SetMessage(GetEnterpriseMessage());
}

views::View* TrayEnterprise::CreateDefaultView(LoginStatus status) {
  DCHECK(!tray_view_);
  // For public accounts, enterprise ownership is indicated in the user details
  // instead.
  if (status == LoginStatus::PUBLIC)
    return nullptr;
  tray_view_ = new LabelTrayView(this, kSystemMenuBusinessIcon);
  tray_view_->set_id(VIEW_ID_TRAY_ENTERPRISE);
  UpdateEnterpriseMessage();
  return tray_view_;
}

void TrayEnterprise::OnDefaultViewDestroyed() {
  tray_view_ = nullptr;
}

void TrayEnterprise::OnEnterpriseDomainChanged() {
  UpdateEnterpriseMessage();
}

void TrayEnterprise::OnViewClicked(views::View* sender) {
  Shell::Get()->system_tray_controller()->ShowEnterpriseInfo();
}

}  // namespace ash
