// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_NTP_RECENT_TABS_PAGE_PREFS_H_
#define CHROME_BROWSER_ANDROID_NTP_RECENT_TABS_PAGE_PREFS_H_

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"

class RecentTabsPagePrefs {
 public:
  explicit RecentTabsPagePrefs(Profile* profile);
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  jboolean GetSnapshotDocumentCollapsed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void SetSnapshotDocumentCollapsed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jboolean is_collapsed);

  jboolean GetRecentlyClosedTabsCollapsed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void SetRecentlyClosedTabsCollapsed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jboolean is_collapsed);

  jboolean GetSyncPromoCollapsed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void SetSyncPromoCollapsed(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj,
                             jboolean is_collapsed);

  jboolean GetForeignSessionCollapsed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& session_tag);
  void SetForeignSessionCollapsed(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& session_tag,
      jboolean is_collapsed);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 private:
  virtual ~RecentTabsPagePrefs();

  Profile* profile_;  // weak
  DISALLOW_COPY_AND_ASSIGN(RecentTabsPagePrefs);
};

#endif  // CHROME_BROWSER_ANDROID_NTP_RECENT_TABS_PAGE_PREFS_H_
