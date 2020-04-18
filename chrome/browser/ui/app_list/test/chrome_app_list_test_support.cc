// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/test/chrome_app_list_test_support.h"

#include <string>

#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_list/app_list_client_impl.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service_factory.h"

namespace test {

namespace {

class CreateProfileHelper {
 public:
  CreateProfileHelper() : profile_(NULL) {}

  Profile* CreateAsync() {
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    base::FilePath temp_profile_dir =
        profile_manager->user_data_dir().AppendASCII("Profile 1");
    profile_manager->CreateProfileAsync(
        temp_profile_dir,
        base::Bind(&CreateProfileHelper::OnProfileCreated,
                   base::Unretained(this)),
        base::string16(),
        std::string(),
        std::string());
    run_loop_.Run();
    return profile_;
  }

 private:
  void OnProfileCreated(Profile* profile, Profile::CreateStatus status) {
    if (status == Profile::CREATE_STATUS_INITIALIZED) {
      profile_ = profile;
      run_loop_.Quit();
    }
  }

  base::RunLoop run_loop_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(CreateProfileHelper);
};

}  // namespace

AppListModelUpdater* GetModelUpdater(AppListClientImpl* client) {
  return app_list::AppListSyncableServiceFactory::GetForProfile(
             client->GetCurrentAppListProfile())
      ->GetModelUpdater();
}

AppListClientImpl* GetAppListClient() {
  AppListClientImpl* client = AppListClientImpl::GetInstance();
  client->UpdateProfile();
  return client;
}

Profile* CreateSecondProfileAsync() {
  CreateProfileHelper helper;
  return helper.CreateAsync();
}

}  // namespace test
