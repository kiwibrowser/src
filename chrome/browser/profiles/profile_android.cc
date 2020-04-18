// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_android.h"

#include "base/android/jni_android.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_destroyer.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "jni/Profile_jni.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace {
const char kProfileAndroidKey[] = "profile_android";
}  // namespace

// static
ProfileAndroid* ProfileAndroid::FromProfile(Profile* profile) {
  if (!profile)
    return NULL;

  ProfileAndroid* profile_android = static_cast<ProfileAndroid*>(
      profile->GetUserData(kProfileAndroidKey));
  if (!profile_android) {
    profile_android = new ProfileAndroid(profile);
    profile->SetUserData(kProfileAndroidKey, base::WrapUnique(profile_android));
  }
  return profile_android;
}

// static
Profile* ProfileAndroid::FromProfileAndroid(const JavaRef<jobject>& obj) {
  if (obj.is_null())
    return NULL;

  ProfileAndroid* profile_android = reinterpret_cast<ProfileAndroid*>(
      Java_Profile_getNativePointer(AttachCurrentThread(), obj));
  if (!profile_android)
    return NULL;
  return profile_android->profile_;
}

// static
ScopedJavaLocalRef<jobject> ProfileAndroid::GetLastUsedProfile(JNIEnv* env,
                                                               jclass clazz) {
  Profile* profile = ProfileManager::GetLastUsedProfile();
  if (profile == NULL) {
    NOTREACHED() << "Profile not found.";
    return ScopedJavaLocalRef<jobject>();
  }

  ProfileAndroid* profile_android = ProfileAndroid::FromProfile(profile);
  if (profile_android == NULL) {
    NOTREACHED() << "ProfileAndroid not found.";
    return ScopedJavaLocalRef<jobject>();
  }

  return ScopedJavaLocalRef<jobject>(profile_android->obj_);
}

void ProfileAndroid::DestroyWhenAppropriate(JNIEnv* env,
                                            const JavaParamRef<jobject>& obj) {
  // Don't delete the Profile directly because the corresponding
  // RenderViewHost might not be deleted yet.
  ProfileDestroyer::DestroyProfileWhenAppropriate(profile_);
}

base::android::ScopedJavaLocalRef<jobject> ProfileAndroid::GetOriginalProfile(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  ProfileAndroid* original_profile = ProfileAndroid::FromProfile(
      profile_->GetOriginalProfile());
  DCHECK(original_profile);
  return original_profile->GetJavaObject();
}

base::android::ScopedJavaLocalRef<jobject>
ProfileAndroid::GetOffTheRecordProfile(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  ProfileAndroid* otr_profile = ProfileAndroid::FromProfile(
      profile_->GetOffTheRecordProfile());
  DCHECK(otr_profile);
  return otr_profile->GetJavaObject();
}

jboolean ProfileAndroid::HasOffTheRecordProfile(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return profile_->HasOffTheRecordProfile();
}

jboolean ProfileAndroid::IsOffTheRecord(JNIEnv* env,
                                        const JavaParamRef<jobject>& obj) {
  return profile_->IsOffTheRecord();
}

jboolean ProfileAndroid::IsChild(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  return profile_->IsChild();
}

// static
ScopedJavaLocalRef<jobject> JNI_Profile_GetLastUsedProfile(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return ProfileAndroid::GetLastUsedProfile(env, clazz);
}

ProfileAndroid::ProfileAndroid(Profile* profile)
    : profile_(profile) {
  JNIEnv* env = AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> jprofile =
      Java_Profile_create(env, reinterpret_cast<intptr_t>(this));
  obj_.Reset(env, jprofile.obj());
}

ProfileAndroid::~ProfileAndroid() {
  Java_Profile_onNativeDestroyed(AttachCurrentThread(), obj_);
}

base::android::ScopedJavaLocalRef<jobject> ProfileAndroid::GetJavaObject() {
  return base::android::ScopedJavaLocalRef<jobject>(obj_);
}
