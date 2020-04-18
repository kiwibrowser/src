// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_COMMON_CRASH_REPORTER_AW_MICRODUMP_CRASH_REPORTER_H_
#define ANDROID_WEBVIEW_COMMON_CRASH_REPORTER_AW_MICRODUMP_CRASH_REPORTER_H_

#include <string>

namespace base {
class FilePath;
}

namespace android_webview {
namespace crash_reporter {

void EnableCrashReporter(const std::string& process_type, int crash_signal_fd);
bool GetCrashDumpLocation(base::FilePath* crash_dir);
void AddGpuFingerprintToMicrodumpCrashHandler(
    const std::string& gpu_fingerprint);
bool DumpWithoutCrashingToFd(int fd);
bool IsCrashReporterEnabled();
void SuppressDumpGeneration();
}  // namespace crash_reporter
}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_COMMON_CRASH_REPORTER_AW_MICRODUMP_CRASH_REPORTER_H_
