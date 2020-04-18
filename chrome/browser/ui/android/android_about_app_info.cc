// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/android_about_app_info.h"

#include <jni.h>
#include <stdint.h>

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "jni/ChromeVersionInfo_jni.h"

std::string AndroidAboutAppInfo::GetGmsInfo() {
  JNIEnv* env = base::android::AttachCurrentThread();
  const base::android::ScopedJavaLocalRef<jstring> info =
      Java_ChromeVersionInfo_getGmsInfo(env);
  return base::android::ConvertJavaStringToUTF8(env, info);
}

std::string AndroidAboutAppInfo::GetOsInfo() {
  std::string android_info_str;

  // Append information about the OS version.
  int32_t os_major_version = 0;
  int32_t os_minor_version = 0;
  int32_t os_bugfix_version = 0;
  base::SysInfo::OperatingSystemVersionNumbers(&os_major_version,
                                               &os_minor_version,
                                               &os_bugfix_version);
  base::StringAppendF(&android_info_str, "%d.%d.%d", os_major_version,
                      os_minor_version, os_bugfix_version);

  // Append information about the device.
  bool semicolon_inserted = false;
  std::string android_build_codename = base::SysInfo::GetAndroidBuildCodename();
  std::string android_device_name = base::SysInfo::HardwareModelName();
  if ("REL" == android_build_codename && android_device_name.size() > 0) {
    android_info_str += "; " + android_device_name;
    semicolon_inserted = true;
  }

  // Append the build ID.
  std::string android_build_id = base::SysInfo::GetAndroidBuildID();
  if (android_build_id.size() > 0) {
    if (!semicolon_inserted) {
      android_info_str += ";";
    }
    android_info_str += " Build/" + android_build_id;
  }

  return android_info_str;
}
