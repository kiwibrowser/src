// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_APP_ANDROID_CAST_CRASH_REPORTER_CLIENT_ANDROID_H_
#define CHROMECAST_APP_ANDROID_CAST_CRASH_REPORTER_CLIENT_ANDROID_H_

#include <stddef.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/crash/content/app/crash_reporter_client.h"

namespace chromecast {

class CastCrashReporterClientAndroid
    : public crash_reporter::CrashReporterClient {
 public:
  explicit CastCrashReporterClientAndroid(const std::string& process_type);
  ~CastCrashReporterClientAndroid() override;

  static bool GetCrashDumpLocation(const std::string& process_type,
                                   base::FilePath* crash_dir);

  // crash_reporter::CrashReporterClient implementation:
  void GetProductNameAndVersion(const char** product_name,
                                const char** version) override;
  base::FilePath GetReporterLogFilename() override;
  bool GetCrashDumpLocation(base::FilePath* crash_dir) override;
  int GetAndroidMinidumpDescriptor() override;
  bool GetCollectStatsConsent() override;
  bool EnableBreakpadForProcess(const std::string& process_type) override;

 private:
  std::string process_type_;

  DISALLOW_COPY_AND_ASSIGN(CastCrashReporterClientAndroid);
};

}  // namespace chromecast

#endif  // CHROMECAST_APP_ANDROID_CAST_CRASH_REPORTER_CLIENT_ANDROID_H_
