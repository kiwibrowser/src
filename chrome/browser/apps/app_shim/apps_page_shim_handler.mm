// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_shim/apps_page_shim_handler.h"

#import "base/mac/foundation_util.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/user_manager.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "ui/base/page_transition_types.h"

namespace {

bool IsProfileSignedOut(Profile* profile) {
  ProfileAttributesEntry* entry;
  bool has_entry =
      g_browser_process->profile_manager()
          ->GetProfileAttributesStorage()
          .GetProfileAttributesWithPath(profile->GetPath(), &entry);
  return has_entry && entry->IsSigninRequired();
}

// Opens a Chrome browser tab at chrome://apps.
void OpenAppsPage(Profile* fallback_profile) {
  Browser* browser = chrome::FindLastActive();
  Profile* app_list_profile = browser ? browser->profile() : fallback_profile;
  app_list_profile = app_list_profile->GetOriginalProfile();

  if (IsProfileSignedOut(app_list_profile) ||
      app_list_profile->IsSystemProfile() ||
      app_list_profile->IsGuestSession()) {
    UserManager::Show(base::FilePath(),
                      profiles::USER_MANAGER_SELECT_PROFILE_NO_ACTION);
    return;
  }

  NavigateParams params(app_list_profile, GURL(chrome::kChromeUIAppsURL),
                        ui::PAGE_TRANSITION_AUTO_BOOKMARK);
  Navigate(&params);
}

}  // namespace

void AppsPageShimHandler::OnShimLaunch(
    apps::AppShimHandler::Host* host,
    apps::AppShimLaunchType launch_type,
    const std::vector<base::FilePath>& files) {
  AppController* controller =
      base::mac::ObjCCastStrict<AppController>([NSApp delegate]);
  OpenAppsPage([controller lastProfile]);

  // Always close the shim process immediately.
  host->OnAppLaunchComplete(apps::APP_SHIM_LAUNCH_DUPLICATE_HOST);
}

void AppsPageShimHandler::OnShimClose(apps::AppShimHandler::Host* host) {}

void AppsPageShimHandler::OnShimFocus(
    apps::AppShimHandler::Host* host,
    apps::AppShimFocusType focus_type,
    const std::vector<base::FilePath>& files) {}

void AppsPageShimHandler::OnShimSetHidden(apps::AppShimHandler::Host* host,
                                          bool hidden) {}

void AppsPageShimHandler::OnShimQuit(apps::AppShimHandler::Host* host) {}
