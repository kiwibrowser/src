// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_launch_for_metro_restart_win.h"

#include "apps/launcher.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/api/app_runtime/app_runtime_api.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/api/app_runtime.h"
#include "extensions/common/constants.h"

using extensions::AppRuntimeEventRouter;
using extensions::Extension;
using extensions::ExtensionSystem;

namespace app_metro_launch {

namespace {

void LaunchAppWithId(Profile* profile,
                     const std::string& extension_id) {
  ExtensionService* extension_service =
      ExtensionSystem::Get(profile)->extension_service();
  if (!extension_service)
    return;

  const Extension* extension =
      extension_service->GetExtensionById(extension_id, false);
  if (!extension)
    return;

  AppRuntimeEventRouter::DispatchOnLaunchedEvent(
      profile, extension, extensions::SOURCE_RESTART, nullptr);
}

}  // namespace

void HandleAppLaunchForMetroRestart(Profile* profile) {
  PrefService* prefs = g_browser_process->local_state();
  if (!prefs->HasPrefPath(prefs::kAppLaunchForMetroRestartProfile))
    return;

  // This will be called for each profile that had a browser window open before
  // relaunch.  After checking that the preference is set, check that the
  // profile that is starting up matches the profile that initially wanted to
  // launch the app.
  base::FilePath profile_dir = base::FilePath::FromUTF8Unsafe(
      prefs->GetString(prefs::kAppLaunchForMetroRestartProfile));
  if (profile_dir.empty() || profile->GetPath().BaseName() != profile_dir)
    return;

  prefs->ClearPref(prefs::kAppLaunchForMetroRestartProfile);

  if (!prefs->HasPrefPath(prefs::kAppLaunchForMetroRestart))
    return;

  std::string extension_id = prefs->GetString(prefs::kAppLaunchForMetroRestart);
  if (extension_id.empty())
    return;

  prefs->ClearPref(prefs::kAppLaunchForMetroRestart);

  const int kRestartAppLaunchDelayMs = 1000;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&LaunchAppWithId, profile, extension_id),
      base::TimeDelta::FromMilliseconds(kRestartAppLaunchDelayMs));
}

void SetAppLaunchForMetroRestart(Profile* profile,
                                 const std::string& extension_id) {
  PrefService* prefs = g_browser_process->local_state();
  prefs->SetString(prefs::kAppLaunchForMetroRestartProfile,
                   profile->GetPath().BaseName().MaybeAsASCII());
  prefs->SetString(prefs::kAppLaunchForMetroRestart, extension_id);
}

void RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kAppLaunchForMetroRestart, "");
  registry->RegisterStringPref(prefs::kAppLaunchForMetroRestartProfile, "");
}

}  // namespace app_metro_launch
