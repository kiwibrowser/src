// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/app_list_controller_impl.h"

#include <utility>

#include "ash/public/interfaces/app_list.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "extensions/common/extension.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"

AppListControllerDelegateImpl::AppListControllerDelegateImpl() {}

AppListControllerDelegateImpl::~AppListControllerDelegateImpl() {}

void AppListControllerDelegateImpl::SetAppListController(
    ash::mojom::AppListController* app_list_controller) {
  app_list_controller_ = app_list_controller;
}

void AppListControllerDelegateImpl::DismissView() {
  if (!app_list_controller_)
    return;
  app_list_controller_->DismissAppList();
}

int64_t AppListControllerDelegateImpl::GetAppListDisplayId() {
  return display_id_;
}

void AppListControllerDelegateImpl::SetAppListDisplayId(int64_t display_id) {
  display_id_ = display_id;
}

void AppListControllerDelegateImpl::GetAppInfoDialogBounds(
    GetAppInfoDialogBoundsCallback callback) {
  if (!app_list_controller_) {
    LOG(ERROR) << "app_list_controller_ is null";
    std::move(callback).Run(gfx::Rect());
    return;
  }
  app_list_controller_->GetAppInfoDialogBounds(std::move(callback));
}

bool AppListControllerDelegateImpl::IsAppPinned(const std::string& app_id) {
  return ChromeLauncherController::instance()->IsAppPinned(app_id);
}

bool AppListControllerDelegateImpl::IsAppOpen(const std::string& app_id) const {
  return ChromeLauncherController::instance()->IsOpen(ash::ShelfID(app_id));
}

void AppListControllerDelegateImpl::PinApp(const std::string& app_id) {
  ChromeLauncherController::instance()->PinAppWithID(app_id);
}

void AppListControllerDelegateImpl::UnpinApp(const std::string& app_id) {
  ChromeLauncherController::instance()->UnpinAppWithID(app_id);
}

AppListControllerDelegate::Pinnable AppListControllerDelegateImpl::GetPinnable(
    const std::string& app_id) {
  return GetPinnableForAppID(app_id,
                             ChromeLauncherController::instance()->profile());
}

void AppListControllerDelegateImpl::OnShowChildDialog() {}

void AppListControllerDelegateImpl::OnCloseChildDialog() {}

void AppListControllerDelegateImpl::CreateNewWindow(Profile* profile,
                                                    bool incognito) {
  if (incognito)
    chrome::NewEmptyWindow(profile->GetOffTheRecordProfile());
  else
    chrome::NewEmptyWindow(profile);
}

void AppListControllerDelegateImpl::OpenURL(Profile* profile,
                                            const GURL& url,
                                            ui::PageTransition transition,
                                            WindowOpenDisposition disposition) {
  NavigateParams params(profile, url, transition);
  params.disposition = disposition;
  Navigate(&params);
}

void AppListControllerDelegateImpl::ActivateApp(
    Profile* profile,
    const extensions::Extension* extension,
    AppListSource source,
    int event_flags) {
  // Platform apps treat activations as a launch. The app can decide whether to
  // show a new window or focus an existing window as it sees fit.
  if (extension->is_platform_app()) {
    LaunchApp(profile, extension, source, event_flags,
              display::kInvalidDisplayId);
    return;
  }

  ChromeLauncherController::instance()->ActivateApp(
      extension->id(), AppListSourceToLaunchSource(source), event_flags);

  if (!IsHomeLauncherEnabledInTabletMode())
    DismissView();
}

void AppListControllerDelegateImpl::LaunchApp(
    Profile* profile,
    const extensions::Extension* extension,
    AppListSource source,
    int event_flags,
    int64_t display_id) {
  ChromeLauncherController::instance()->LaunchApp(
      ash::ShelfID(extension->id()), AppListSourceToLaunchSource(source),
      event_flags, display_id);

  if (!IsHomeLauncherEnabledInTabletMode())
    DismissView();
}

ash::ShelfLaunchSource
AppListControllerDelegateImpl::AppListSourceToLaunchSource(
    AppListSource source) {
  switch (source) {
    case LAUNCH_FROM_APP_LIST:
      return ash::LAUNCH_FROM_APP_LIST;
    case LAUNCH_FROM_APP_LIST_SEARCH:
      return ash::LAUNCH_FROM_APP_LIST_SEARCH;
    default:
      return ash::LAUNCH_FROM_UNKNOWN;
  }
}
