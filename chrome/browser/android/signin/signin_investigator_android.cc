// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/signin/signin_investigator_android.h"

#include "base/android/jni_string.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/investigator_dependency_provider.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "jni/SigninInvestigator_jni.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;

// static
jint JNI_SigninInvestigator_Investigate(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jstring>& current_email) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  DCHECK(profile);
  InvestigatorDependencyProvider provider(profile);
  const std::string email = ConvertJavaStringToUTF8(env, current_email);

  // It may be possible that the account tracker is not aware of this account
  // yet. If this happens we'll get an empty account_id back, and the
  // investigator should fallback to an email comparison.
  const std::string account_id =
      AccountTrackerServiceFactory::GetForProfile(profile)
          ->FindAccountInfoByEmail(email)
          .account_id;

  return static_cast<int>(
      SigninInvestigator(email, account_id, &provider).Investigate());
}
