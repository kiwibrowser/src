// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/base/cast_sys_info_android.h"

#include <sys/system_properties.h>
#include <memory>
#include <string>

#include "base/android/build_info.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/string_number_conversions.h"
#include "base/sys_info.h"
#include "chromecast/base/cast_sys_info_util.h"
#include "chromecast/base/version.h"
#include "chromecast/chromecast_buildflags.h"
#include "jni/CastSysInfoAndroid_jni.h"
#if BUILDFLAG(IS_ANDROID_THINGS_NON_PUBLIC)
#include "jni/CastSysInfoAndroidThings_jni.h"
#endif

namespace chromecast {

namespace {
const char kBuildTypeUser[] = "user";

std::string GetAndroidProperty(const std::string& key,
                               const std::string& default_value) {
  char value[PROP_VALUE_MAX];
  int ret = __system_property_get(key.c_str(), value);
  if (ret <= 0) {
    VLOG(1) << "No value set for property: " << key;
    return default_value;
  }

  return std::string(value);
}

}  // namespace

// static
std::unique_ptr<CastSysInfo> CreateSysInfo() {
  return std::make_unique<CastSysInfoAndroid>();
}

CastSysInfoAndroid::CastSysInfoAndroid()
    : build_info_(base::android::BuildInfo::GetInstance()) {}

CastSysInfoAndroid::~CastSysInfoAndroid() {}

CastSysInfo::BuildType CastSysInfoAndroid::GetBuildType() {
  if (CAST_IS_DEBUG_BUILD())
    return BUILD_ENG;

  int build_number;
  if (!base::StringToInt(CAST_BUILD_INCREMENTAL, &build_number))
    build_number = 0;

  const std::string channel(GetSystemReleaseChannel());
  if (strcmp(build_info_->build_type(), kBuildTypeUser) == 0 &&
      build_number > 0 && (channel.empty() || channel == "stable-channel")) {
    return BUILD_PRODUCTION;
  }

  // Dogfooders without a user system build should all still have non-Debug
  // builds of the cast receiver APK, but with valid build numbers.
  if (build_number > 0)
    return BUILD_BETA;

  // Default to ENG build.
  return BUILD_ENG;
}

std::string CastSysInfoAndroid::GetSerialNumber() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::android::ConvertJavaStringToUTF8(
      Java_CastSysInfoAndroid_getSerialNumber(env));
}

std::string CastSysInfoAndroid::GetProductName() {
  return build_info_->device();
}

std::string CastSysInfoAndroid::GetDeviceModel() {
  return build_info_->model();
}

std::string CastSysInfoAndroid::GetManufacturer() {
  return build_info_->manufacturer();
}

std::string CastSysInfoAndroid::GetSystemBuildNumber() {
  return base::SysInfo::GetAndroidBuildID();
}

std::string CastSysInfoAndroid::GetSystemReleaseChannel() {
#if BUILDFLAG(IS_ANDROID_THINGS_NON_PUBLIC)
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::android::ConvertJavaStringToUTF8(
      Java_CastSysInfoAndroidThings_getReleaseChannel(env));
#else
  return "";
#endif
}

std::string CastSysInfoAndroid::GetBoardName() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::android::ConvertJavaStringToUTF8(
      Java_CastSysInfoAndroid_getBoard(env));
}

std::string CastSysInfoAndroid::GetBoardRevision() {
  return "";
}

std::string CastSysInfoAndroid::GetFactoryCountry() {
  return GetAndroidProperty("ro.boot.wificountrycode", "");
}

std::string CastSysInfoAndroid::GetFactoryLocale(std::string* second_locale) {
  // This duplicates the read-only property portion of
  // frameworks/base/core/jni/AndroidRuntime.cpp in the Android tree, which is
  // effectively the "factory locale", i.e. the locale chosen by Android
  // assuming the other persist.sys.* properties are not set.
  const std::string locale = GetAndroidProperty("ro.product.locale", "");
  if (!locale.empty()) {
    return locale;
  }

  const std::string language =
      GetAndroidProperty("ro.product.locale.language", "en");
  const std::string region =
      GetAndroidProperty("ro.product.locale.region", "US");
  return language + "-" + region;
}

std::string CastSysInfoAndroid::GetWifiInterface() {
  return "";
}

std::string CastSysInfoAndroid::GetApInterface() {
  return "";
}

std::string CastSysInfoAndroid::GetGlVendor() {
  NOTREACHED() << "GL information shouldn't be requested on Android.";
  return "";
}

std::string CastSysInfoAndroid::GetGlRenderer() {
  NOTREACHED() << "GL information shouldn't be requested on Android.";
  return "";
}

std::string CastSysInfoAndroid::GetGlVersion() {
  NOTREACHED() << "GL information shouldn't be requested on Android.";
  return "";
}

}  // namespace chromecast
