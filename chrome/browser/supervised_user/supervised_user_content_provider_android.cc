// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_content_provider_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/supervised_user/supervised_user_interstitial.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "jni/SupervisedUserContentProvider_jni.h"

using base::android::JavaRef;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::AttachCurrentThread;

SupervisedUserContentProvider::UrlFilterObserver::UrlFilterObserver(
    JNIEnv* env,
    const ScopedJavaGlobalRef<jobject>& java_content_provider)
    : java_content_provider_(java_content_provider) {}

SupervisedUserContentProvider::UrlFilterObserver::~UrlFilterObserver() {}

void SupervisedUserContentProvider::UrlFilterObserver::OnURLFilterChanged() {
  Java_SupervisedUserContentProvider_onSupervisedUserFilterUpdated(
      AttachCurrentThread(), java_content_provider_);
}

static jlong
JNI_SupervisedUserContentProvider_CreateSupervisedUserContentProvider(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller) {
  return reinterpret_cast<intptr_t>(
      new SupervisedUserContentProvider(env, caller));
}

SupervisedUserContentProvider::SupervisedUserContentProvider(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller)
    : profile_(ProfileManager::GetLastUsedProfile()),
      java_content_provider_(env, caller),
      url_filter_observer_(new UrlFilterObserver(env, java_content_provider_)),
      weak_factory_(this) {
  if (profile_->IsSupervised()) {
    SupervisedUserService* supervised_user_service =
        SupervisedUserServiceFactory::GetForProfile(profile_);
    supervised_user_service->AddObserver(url_filter_observer_.get());
  }
}

SupervisedUserContentProvider::~SupervisedUserContentProvider() {
  if (profile_->IsSupervised()) {
    SupervisedUserService* supervised_user_service =
        SupervisedUserServiceFactory::GetForProfile(profile_);
    supervised_user_service->RemoveObserver(url_filter_observer_.get());
  }
}

void SupervisedUserContentProvider::ShouldProceed(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller,
    const JavaParamRef<jobject>& query_result_jobj,
    const JavaParamRef<jstring>& url) {
  if (!profile_->IsSupervised()) {
    // User isn't supervised, this can only happen if Chrome isn't signed in,
    // in which case all requests should be rejected
    Java_SupervisedUserQueryReply_onQueryFailed(
        AttachCurrentThread(), query_result_jobj,
        supervised_user_error_page::NOT_SIGNED_IN, false, true, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr);
    return;
  }
  SupervisedUserService* supervised_user_service =
      SupervisedUserServiceFactory::GetForProfile(profile_);
  SupervisedUserURLFilter* url_filter = supervised_user_service->GetURLFilter();
  url_filter->GetFilteringBehaviorForURLWithAsyncChecks(
      GURL(base::android::ConvertJavaStringToUTF16(env, url)),
      base::Bind(&SupervisedUserContentProvider::OnQueryComplete,
                 weak_factory_.GetWeakPtr(),
                 ScopedJavaGlobalRef<jobject>(env, query_result_jobj.obj())));
}

void SupervisedUserContentProvider::RequestInsert(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller,
    const JavaParamRef<jobject>& insert_result_jobj,
    const JavaParamRef<jstring>& url) {
  if (!profile_->IsSupervised())
    return;
  SupervisedUserService* supervised_user_service =
      SupervisedUserServiceFactory::GetForProfile(profile_);
  supervised_user_service->AddURLAccessRequest(
      GURL(base::android::ConvertJavaStringToUTF16(env, url)),
      base::Bind(&SupervisedUserContentProvider::OnInsertRequestSendComplete,
                 weak_factory_.GetWeakPtr(),
                 ScopedJavaGlobalRef<jobject>(env, insert_result_jobj.obj())));
}

void SupervisedUserContentProvider::OnQueryComplete(
    ScopedJavaGlobalRef<jobject> query_reply_jobj,
    SupervisedUserURLFilter::FilteringBehavior behavior,
    supervised_user_error_page::FilteringBehaviorReason reason,
    bool /* uncertain */) {
  if (behavior != SupervisedUserURLFilter::BLOCK) {
    Java_SupervisedUserQueryReply_onQueryComplete(AttachCurrentThread(),
                                                  query_reply_jobj);
  } else {
    JNIEnv* env = AttachCurrentThread();
    SupervisedUserService* service =
        SupervisedUserServiceFactory::GetForProfile(profile_);
    Java_SupervisedUserQueryReply_onQueryFailed(
        env, query_reply_jobj, reason, service->AccessRequestsEnabled(),
        profile_->IsChild(),
        base::android::ConvertUTF8ToJavaString(
            env, profile_->GetPrefs()->GetString(
                     prefs::kSupervisedUserCustodianProfileImageURL)),
        base::android::ConvertUTF8ToJavaString(
            env, profile_->GetPrefs()->GetString(
                     prefs::kSupervisedUserSecondCustodianProfileImageURL)),
        base::android::ConvertUTF8ToJavaString(env,
                                               service->GetCustodianName()),
        base::android::ConvertUTF8ToJavaString(
            env, service->GetCustodianEmailAddress()),
        base::android::ConvertUTF8ToJavaString(
            env, service->GetSecondCustodianName()),
        base::android::ConvertUTF8ToJavaString(
            env, service->GetSecondCustodianEmailAddress()));
  }
}

void SupervisedUserContentProvider::SetFilterForTesting(JNIEnv* env,
                                                        jobject caller) {
  if (!profile_->IsSupervised())
    return;
  SupervisedUserService* supervised_user_service =
      SupervisedUserServiceFactory::GetForProfile(profile_);
  SupervisedUserURLFilter* url_filter = supervised_user_service->GetURLFilter();
  url_filter->SetDefaultFilteringBehavior(SupervisedUserURLFilter::BLOCK);
}

void SupervisedUserContentProvider::OnInsertRequestSendComplete(
    ScopedJavaGlobalRef<jobject> insert_reply_jobj,
    bool sent_ok) {
  Java_SupervisedUserInsertReply_onInsertRequestSendComplete(
      AttachCurrentThread(), insert_reply_jobj, sent_ok);
}
