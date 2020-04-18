// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_CONTENT_PROVIDER_ANDROID_H_
#define CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_CONTENT_PROVIDER_ANDROID_H_

#include <memory>
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/supervised_user/supervised_user_service_observer.h"
#include "chrome/browser/supervised_user/supervised_user_url_filter.h"
#include "components/supervised_user_error_page/supervised_user_error_page.h"

class Profile;

class SupervisedUserContentProvider {
 public:
  SupervisedUserContentProvider(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& caller);
  virtual ~SupervisedUserContentProvider();

  void ShouldProceed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& caller,
      const base::android::JavaParamRef<jobject>& query_result_jobj,
      const base::android::JavaParamRef<jstring>& url);
  void RequestInsert(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& caller,
      const base::android::JavaParamRef<jobject>& insert_result_jobj,
      const base::android::JavaParamRef<jstring>& url);

  void SetFilterForTesting(JNIEnv* env, jobject caller);

 private:
  class UrlFilterObserver : public SupervisedUserServiceObserver {
   public:
    UrlFilterObserver(JNIEnv* env,
                      const base::android::ScopedJavaGlobalRef<jobject>&
                          java_content_provider);

    ~UrlFilterObserver() override;

   private:
    void OnURLFilterChanged() override;
    base::android::ScopedJavaGlobalRef<jobject> java_content_provider_;
  };

  void OnQueryComplete(
      base::android::ScopedJavaGlobalRef<jobject> query_reply_jobj,
      SupervisedUserURLFilter::FilteringBehavior behavior,
      supervised_user_error_page::FilteringBehaviorReason reason,
      bool /* uncertain */);
  void OnInsertRequestSendComplete(
      base::android::ScopedJavaGlobalRef<jobject> insert_reply_jobj,
      bool sent_ok);
  Profile* profile_;
  base::android::ScopedJavaGlobalRef<jobject> java_content_provider_;
  std::unique_ptr<UrlFilterObserver> url_filter_observer_;

  base::WeakPtrFactory<SupervisedUserContentProvider> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserContentProvider);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_CONTENT_PROVIDER_ANDROID_H_
