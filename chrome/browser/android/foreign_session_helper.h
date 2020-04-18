// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_FOREIGN_SESSION_HELPER_H_
#define CHROME_BROWSER_ANDROID_FOREIGN_SESSION_HELPER_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sync/driver/sync_service_observer.h"

using base::android::ScopedJavaLocalRef;

namespace syncer {
class SyncService;
}  // namespace syncer

class ForeignSessionHelper : public syncer::SyncServiceObserver {
 public:
  explicit ForeignSessionHelper(Profile* profile);
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  jboolean IsTabSyncEnabled(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);
  void TriggerSessionSync(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj);
  void SetOnForeignSessionCallback(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& callback);
  jboolean GetForeignSessions(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& result);
  jboolean OpenForeignSessionTab(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_tab,
      const base::android::JavaParamRef<jstring>& session_tag,
      jint tab_id,
      jint disposition);
  void DeleteForeignSession(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& session_tag);

  // syncer::SyncServiceObserver implementation
  void OnSyncConfigurationCompleted(syncer::SyncService* sync) override;
  void OnForeignSessionUpdated(syncer::SyncService* sync) override;

 private:
  ~ForeignSessionHelper() override;

  // Fires |callback_| if it is not null.
  void FireForeignSessionCallback();

  Profile* profile_;  // weak
  base::android::ScopedJavaGlobalRef<jobject> callback_;
  ScopedObserver<syncer::SyncService, syncer::SyncServiceObserver>
      scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(ForeignSessionHelper);
};

#endif  // CHROME_BROWSER_ANDROID_FOREIGN_SESSION_HELPER_H_
