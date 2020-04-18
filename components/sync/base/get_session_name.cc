// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/get_session_name.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"

#if defined(OS_CHROMEOS)
#include "chromeos/system/devicetype.h"
#elif defined(OS_LINUX)
#include "components/sync/base/get_session_name_linux.h"
#elif defined(OS_IOS)
#include "components/sync/base/get_session_name_ios.h"
#elif defined(OS_MACOSX)
#include "components/sync/base/get_session_name_mac.h"
#elif defined(OS_WIN)
#include "components/sync/base/get_session_name_win.h"
#elif defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

namespace syncer {

namespace {

std::string GetSessionNameSynchronously() {
  base::AssertBlockingAllowed();
  std::string session_name;
#if defined(OS_CHROMEOS)
  switch (chromeos::GetDeviceType()) {
    case chromeos::DeviceType::kChromebase:
      session_name = "Chromebase";
      break;
    case chromeos::DeviceType::kChromebit:
      session_name = "Chromebit";
      break;
    case chromeos::DeviceType::kChromebook:
      session_name = "Chromebook";
      break;
    case chromeos::DeviceType::kChromebox:
      session_name = "Chromebox";
      break;
    case chromeos::DeviceType::kUnknown:
      session_name = "Chromebook";
      break;
  }
#elif defined(OS_LINUX)
  session_name = internal::GetHostname();
#elif defined(OS_IOS)
  session_name = internal::GetComputerName();
#elif defined(OS_MACOSX)
  session_name = internal::GetHardwareModelName();
#elif defined(OS_WIN)
  session_name = internal::GetComputerName();
#elif defined(OS_ANDROID)
  base::android::BuildInfo* android_build_info =
      base::android::BuildInfo::GetInstance();
  session_name = android_build_info->model();
#endif

  if (session_name == "Unknown" || session_name.empty())
    session_name = base::SysInfo::OperatingSystemName();

  DCHECK(base::IsStringUTF8(session_name));
  return session_name;
}

}  // namespace

void GetSessionName(
    const scoped_refptr<base::TaskRunner>& task_runner,
    const base::Callback<void(const std::string&)>& done_callback) {
  base::PostTaskAndReplyWithResult(task_runner.get(), FROM_HERE,
                                   base::Bind(&GetSessionNameSynchronously),
                                   done_callback);
}

std::string GetSessionNameSynchronouslyForTesting() {
  return GetSessionNameSynchronously();
}

}  // namespace syncer
