// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/internal_app/internal_app_model_builder.h"

#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/internal_app/internal_app_item.h"
#include "chrome/browser/ui/app_list/internal_app/internal_app_metadata.h"

InternalAppModelBuilder::InternalAppModelBuilder(
    AppListControllerDelegate* controller)
    : AppListModelBuilder(controller, InternalAppItem::kItemType) {}

void InternalAppModelBuilder::BuildModel() {
  for (const auto& internal_app : app_list::GetInternalAppList()) {
    if (!internal_app.show_in_launcher)
      continue;

    InsertApp(std::make_unique<InternalAppItem>(
        profile(), GetSyncItem(internal_app.app_id), internal_app));
  }
}
