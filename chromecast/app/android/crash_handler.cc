// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/app/android/crash_handler.h"

#include <jni.h>
#include <stdlib.h>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "chromecast/app/android/cast_crash_reporter_client_android.h"
#include "chromecast/base/version.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "components/crash/content/app/crash_reporter_client.h"
#include "content/public/common/content_switches.h"
#include "jni/CastCrashHandler_jni.h"
#include "services/service_manager/embedder/switches.h"
#include "third_party/breakpad/breakpad/src/client/linux/handler/exception_handler.h"
#include "third_party/breakpad/breakpad/src/client/linux/handler/minidump_descriptor.h"

namespace {

chromecast::CrashHandler* g_crash_handler = NULL;

// Debug builds: always to crash-staging
// Release builds: only to crash-staging for local/invalid build numbers
bool UploadCrashToStaging() {
#if CAST_IS_DEBUG_BUILD()
  return true;
#else
  int build_number;
  if (base::StringToInt(CAST_BUILD_INCREMENTAL, &build_number))
    return build_number == 0;
  return true;
#endif
}

}  // namespace

namespace chromecast {

// static
void CrashHandler::Initialize(const std::string& process_type,
                              const base::FilePath& log_file_path) {
  DCHECK(!g_crash_handler);
  g_crash_handler = new CrashHandler(process_type, log_file_path);
  g_crash_handler->Initialize();
}

// static
bool CrashHandler::GetCrashDumpLocation(base::FilePath* crash_dir) {
  DCHECK(g_crash_handler);
  return g_crash_handler->crash_reporter_client_->GetCrashDumpLocation(
      crash_dir);
}

CrashHandler::CrashHandler(const std::string& process_type,
                           const base::FilePath& log_file_path)
    : log_file_path_(log_file_path),
      process_type_(process_type),
      crash_reporter_client_(new CastCrashReporterClientAndroid(process_type)) {
  if (!crash_reporter_client_->GetCrashDumpLocation(&crash_dump_path_)) {
    LOG(ERROR) << "Could not get crash dump location";
  }
  SetCrashReporterClient(crash_reporter_client_.get());
}

CrashHandler::~CrashHandler() {
  DCHECK(g_crash_handler);
  g_crash_handler = NULL;
}

void CrashHandler::Initialize() {
  if (process_type_.empty()) {
    InitializeUploader();
    breakpad::InitCrashReporter(process_type_);
    return;
  }

  if (process_type_ != service_manager::switches::kZygoteProcess) {
    breakpad::InitNonBrowserCrashReporterForAndroid(process_type_);
  }
}

void CrashHandler::InitializeUploader() {
  CrashHandler::UploadDumps(crash_dump_path_, "", "", true);
}

// static
void CrashHandler::UploadDumps(const base::FilePath& crash_dump_path,
                               const std::string& uuid,
                               const std::string& application_feedback,
                               bool periodic_upload) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> crash_dump_path_java =
      base::android::ConvertUTF8ToJavaString(env, crash_dump_path.value());
  base::android::ScopedJavaLocalRef<jstring> uuid_java =
      base::android::ConvertUTF8ToJavaString(env, uuid);
  base::android::ScopedJavaLocalRef<jstring> application_feedback_java =
      base::android::ConvertUTF8ToJavaString(env, application_feedback);
  Java_CastCrashHandler_initializeUploader(
      env, crash_dump_path_java, uuid_java, application_feedback_java,
      UploadCrashToStaging(), periodic_upload);
}

}  // namespace chromecast
