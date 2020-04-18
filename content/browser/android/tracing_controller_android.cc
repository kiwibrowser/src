// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/tracing_controller_android.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/tracing_controller.h"
#include "jni/TracingControllerAndroid_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace content {

static jlong JNI_TracingControllerAndroid_Init(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  TracingControllerAndroid* profiler = new TracingControllerAndroid(env, obj);
  return reinterpret_cast<intptr_t>(profiler);
}

TracingControllerAndroid::TracingControllerAndroid(JNIEnv* env, jobject obj)
    : weak_java_object_(env, obj),
      weak_factory_(this) {}

TracingControllerAndroid::~TracingControllerAndroid() {}

void TracingControllerAndroid::Destroy(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  delete this;
}

bool TracingControllerAndroid::StartTracing(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jcategories,
    const JavaParamRef<jstring>& jtraceoptions) {
  std::string categories =
      base::android::ConvertJavaStringToUTF8(env, jcategories);
  std::string options =
      base::android::ConvertJavaStringToUTF8(env, jtraceoptions);

  // This log is required by adb_profile_chrome.py.
  LOG(WARNING) << "Logging performance trace to file";

  return TracingController::GetInstance()->StartTracing(
      base::trace_event::TraceConfig(categories, options),
      TracingController::StartTracingDoneCallback());
}

void TracingControllerAndroid::StopTracing(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jfilepath) {
  base::FilePath file_path(
      base::android::ConvertJavaStringToUTF8(env, jfilepath));
  if (!TracingController::GetInstance()->StopTracing(
          TracingController::CreateFileEndpoint(
              file_path, base::Bind(&TracingControllerAndroid::OnTracingStopped,
                                    weak_factory_.GetWeakPtr())))) {
    LOG(ERROR) << "EndTracingAsync failed, forcing an immediate stop";
    OnTracingStopped();
  }
}

void TracingControllerAndroid::GenerateTracingFilePath(
    base::FilePath* file_path) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jfilename =
      Java_TracingControllerAndroid_generateTracingFilePath(env);
  *file_path = base::FilePath(
      base::android::ConvertJavaStringToUTF8(env, jfilename.obj()));
}

void TracingControllerAndroid::OnTracingStopped() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj())
    Java_TracingControllerAndroid_onTracingStopped(env, obj);
}

bool TracingControllerAndroid::GetKnownCategoryGroupsAsync(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!TracingController::GetInstance()->GetCategories(
          base::Bind(&TracingControllerAndroid::OnKnownCategoriesReceived,
                     weak_factory_.GetWeakPtr()))) {
    return false;
  }
  return true;
}

void TracingControllerAndroid::OnKnownCategoriesReceived(
    const std::set<std::string>& categories_received) {
  base::ListValue category_list;
  for (std::set<std::string>::const_iterator it = categories_received.begin();
       it != categories_received.end();
       ++it) {
    category_list.AppendString(*it);
  }
  std::string received_category_list;
  base::JSONWriter::Write(category_list, &received_category_list);

  // This log is required by adb_profile_chrome.py.
  LOG(WARNING) << "{\"traceCategoriesList\": " << received_category_list << "}";
}

static ScopedJavaLocalRef<jstring>
JNI_TracingControllerAndroid_GetDefaultCategories(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  base::trace_event::TraceConfig trace_config;
  return base::android::ConvertUTF8ToJavaString(
      env, trace_config.ToCategoryFilterString());
}

}  // namespace content
