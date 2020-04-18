// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/crash_upload_list/crash_upload_list.h"

#include "build/build_config.h"

#if defined(OS_MACOSX) || defined(OS_WIN)
#include "chrome/browser/crash_upload_list/crash_upload_list_crashpad.h"
#else
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "components/upload_list/crash_upload_list.h"
#include "components/upload_list/text_log_upload_list.h"
#endif

#if defined(OS_ANDROID)
#include "chrome/browser/crash_upload_list/crash_upload_list_android.h"
#endif

scoped_refptr<UploadList> CreateCrashUploadList() {
#if defined(OS_MACOSX) || defined(OS_WIN)
  return new CrashUploadListCrashpad();
#else
  base::FilePath crash_dir_path;
  base::PathService::Get(chrome::DIR_CRASH_DUMPS, &crash_dir_path);
  base::FilePath upload_log_path =
      crash_dir_path.AppendASCII(CrashUploadList::kReporterLogFilename);
#if defined(OS_ANDROID)
  return new CrashUploadListAndroid(upload_log_path);
#else
  return new TextLogUploadList(upload_log_path);
#endif  // defined(OS_ANDROID)
#endif  // defined(OS_MACOSX) || defined(OS_WIN)
}
