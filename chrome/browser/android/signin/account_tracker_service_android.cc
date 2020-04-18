// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/android/jni_array.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "jni/AccountTrackerService_jni.h"

using base::android::JavaParamRef;

namespace signin {
namespace android {

void JNI_AccountTrackerService_SeedAccountsInfo(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jobjectArray>& gaiaIds,
    const JavaParamRef<jobjectArray>& accountNames) {
  std::vector<std::string> gaia_ids;
  std::vector<std::string> account_names;
  base::android::AppendJavaStringArrayToStringVector(env, gaiaIds, &gaia_ids);
  base::android::AppendJavaStringArrayToStringVector(env, accountNames,
                                                     &account_names);
  DCHECK_EQ(gaia_ids.size(), account_names.size());

  DVLOG(1) << "AccountTrackerServiceAndroid::SeedAccountsInfo: "
           << " number of accounts " << gaia_ids.size();
  Profile* profile = ProfileManager::GetActiveUserProfile();
  AccountTrackerService* account_tracker_service =
      AccountTrackerServiceFactory::GetForProfile(profile);

  for (size_t i = 0; i < gaia_ids.size(); i++) {
    account_tracker_service->SeedAccountInfo(gaia_ids[i], account_names[i]);
  }
}

jboolean JNI_AccountTrackerService_AreAccountsSeeded(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jobjectArray>& accountNames) {
  std::vector<std::string> account_names;
  base::android::AppendJavaStringArrayToStringVector(env, accountNames,
                                                     &account_names);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  AccountTrackerService* account_tracker_service =
      AccountTrackerServiceFactory::GetForProfile(profile);
  bool migrated =
      account_tracker_service->GetMigrationState() ==
              AccountTrackerService::AccountIdMigrationState::MIGRATION_DONE
          ? true
          : false;

  for (size_t i = 0; i < account_names.size(); i++) {
    AccountInfo info =
        account_tracker_service->FindAccountInfoByEmail(account_names[i]);
    if (info.account_id.empty()) {
      return false;
    }
    if (migrated && info.gaia.empty()) {
      return false;
    }
  }
  return true;
}

}  // namespace android
}  // namespace signin
